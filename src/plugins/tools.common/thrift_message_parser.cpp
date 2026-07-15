/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Microsoft Corporation
 * 
 * -=- Robust Distributed System Nucleus (rDSN) -=- 
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/*
 * Description:
 *     What is this file about?
 *
 * Revision history:
 *     Jun. 2016, Zuoyan Qin, first version
 *     xxxx-xx-xx, author, fix bug about xxx
 */

# if defined(_WIN32)
# include <winsock2.h>
# endif

# include "thrift_message_parser.h"
# include <dsn/service_api_c.h>
# include <dsn/cpp/serialization_helper/thrift_helper.h>
# include <dsn/utility/ports.h>
# include <cstring>
# include <utility>
# include <limits>

# ifdef __TITLE__
# undef __TITLE__
# endif
# define __TITLE__ "thrift.message.parser"

namespace dsn
{
    void thrift_message_parser::reset()
    {
        _header_parsed = false;
    }

    message_ex* thrift_message_parser::get_message_on_receive(message_reader* reader, /*out*/ int& read_next)
    {
        read_next = 4096;

        dsn::blob& buf = reader->_buffer;
        char* buf_ptr = (char*)buf.data();
        unsigned int buf_len = reader->_buffer_occupied;

        if (buf_len >= sizeof(thrift_message_header))
        {
            if (!_header_parsed)
            {
                read_thrift_header(buf_ptr, _thrift_header);

                if (!check_thrift_header(_thrift_header))
                {
                    derror("header check failed");
                    read_next = -1;
                    return nullptr;
                }
                else
                {
                    _header_parsed = true;
                }
            }

            // body_length comes from the wire; compute the total size in 64-bit and reject a
            // value that would overflow the 32-bit msg_sz and wrap to a small size (which would
            // desynchronize the stream and cause the rest of the message to be misparsed).
            uint64_t msg_sz64 = (uint64_t)sizeof(thrift_message_header) + _thrift_header.body_length;
            if (msg_sz64 > UINT32_MAX)
            {
                derror("thrift message body length is too large: %u", _thrift_header.body_length);
                read_next = -1;
                return nullptr;
            }
            unsigned int msg_sz = (unsigned int)msg_sz64;

            // msg done
            if (buf_len >= msg_sz)
            {
                dsn::blob msg_bb = buf.range(0, msg_sz);
                message_ex* msg = parse_message(_thrift_header, msg_bb);

                // parse_message returns null when the (untrusted) thrift body is malformed
                // (bad message-begin envelope, over-long rpc name, or a non-request message).
                // The receive loop only treats read_next == -1 as a hard failure, so we must
                // signal it here instead of falling through and dereferencing the null msg.
                if (msg == nullptr)
                {
                    derror("thrift message body check failed");
                    read_next = -1;
                    return nullptr;
                }

                reader->_buffer = buf.range(msg_sz);
                reader->_buffer_occupied -= msg_sz;
                _header_parsed = false;
                read_next = (reader->_buffer_occupied >= sizeof(thrift_message_header) ?
                                 0 : sizeof(thrift_message_header) - reader->_buffer_occupied);
                msg->hdr_format = NET_HDR_THRIFT;
                return msg;
            }
            else // buf_len < msg_sz
            {
                read_next = msg_sz - buf_len;
                return nullptr;
            }
        }
        else // buf_len < sizeof(thrift_message_header)
        {
            read_next = sizeof(thrift_message_header) - buf_len;
            return nullptr;
        }
    }

    void thrift_message_parser::prepare_on_send(message_ex *msg)
    {
        auto& header = msg->header;
        auto& buffers = msg->buffers;

        dassert(!header->context.u.is_request, "only support send response");
        dassert(header->server.error_name[0], "error name should be set");
        dassert(!buffers.empty(), "buffers can not be empty");

        // write thrift response header and thrift message begin
        binary_writer header_writer;
        binary_writer_transport header_trans(header_writer);
        boost::shared_ptr<binary_writer_transport> header_trans_ptr(&header_trans, [](binary_writer_transport*) {});
        ::apache::thrift::protocol::TBinaryProtocol header_proto(header_trans_ptr);
        //first total length, but we don't know the length, so firstly we put a placeholder
        header_proto.writeI32(0);
        char_ptr error_msg(header->server.error_name, strlen(header->server.error_name));
        //then the error_message
        header_proto.writeString<char_ptr>(error_msg);
        //then the thrift message begin
        header_proto.writeMessageBegin(header->rpc_name, ::apache::thrift::protocol::T_REPLY, header->id);

        // write thrift message end
        binary_writer end_writer;
        binary_writer_transport end_trans(end_writer);
        boost::shared_ptr<binary_writer_transport> end_trans_ptr(&end_trans, [](binary_writer_transport*) {});
        ::apache::thrift::protocol::TBinaryProtocol end_proto(end_trans_ptr);
        end_proto.writeMessageEnd();

        // now let's set the total length
        blob header_bb = header_writer.get_buffer();
        blob end_bb = end_writer.get_buffer();
        int32_t* total_length = reinterpret_cast<int32_t*>((void*)header_bb.data());
        *total_length = htobe32(header_bb.length() + header->body_length + end_bb.length());

        unsigned int dsn_size = sizeof(message_header) + header->body_length;
        int dsn_buf_count = 0;
        while (dsn_size > 0 && dsn_buf_count < buffers.size())
        {
            blob& buf = buffers[dsn_buf_count];
            dassert(dsn_size >= buf.length(), "");
            dsn_size -= buf.length();
            ++dsn_buf_count;
        }
        dassert(dsn_size == 0, "");

        // put header_bb and end_bb at the end
        buffers.resize(dsn_buf_count);
        buffers.emplace_back(std::move(header_bb));
        buffers.emplace_back(std::move(end_bb));
    }

    int thrift_message_parser::get_buffer_count_on_send(message_ex* msg)
    {
        return (int)msg->buffers.size();
    }

    int thrift_message_parser::get_buffers_on_send(message_ex* msg, /*out*/ send_buf* buffers)
    {
        auto& msg_header = msg->header;
        auto& msg_buffers = msg->buffers;

        // leave buffers[0] to header
        int i = 1;
        // we must skip the dsn message header
        unsigned int offset = sizeof(message_header);
        unsigned int dsn_size = sizeof(message_header) + msg_header->body_length;
        int dsn_buf_count = 0;
        while (dsn_size > 0 && dsn_buf_count < msg_buffers.size())
        {
            blob& buf = msg_buffers[dsn_buf_count];
            dassert(dsn_size >= buf.length(), "");
            dsn_size -= buf.length();
            ++dsn_buf_count;

            if (offset >= buf.length())
            {
                offset -= buf.length();
                continue;
            }
            buffers[i].buf = (void*)(buf.data() + offset);
            buffers[i].sz = buf.length() - offset;
            offset = 0;
            ++i;
        }
        dassert(dsn_size == 0, "");
        dassert(dsn_buf_count + 2 == msg_buffers.size(), "must have 2 more blob at the end");

        // set header
        blob& header_bb = msg_buffers[dsn_buf_count];
        buffers[0].buf = (void*)header_bb.data();
        buffers[0].sz = header_bb.length();

        // set end if need
        blob& end_bb = msg_buffers[dsn_buf_count + 1];
        if (end_bb.length() > 0)
        {
            buffers[i].buf = (void*)end_bb.data();
            buffers[i].sz = end_bb.length();
            ++i;
        }

        return i;
    }

    void thrift_message_parser::read_thrift_header(const char* buffer, /*out*/ thrift_message_header& header)
    {
        // The header sits at an arbitrary offset inside the shared receive buffer: after a
        // message is consumed get_message_on_receive advances _buffer by msg_sz (an
        // attacker-controlled amount, since it includes body_length), so the header of a
        // following pipelined message generally starts at an unaligned address. Reading the
        // multi-byte fields directly through `*(uint32_t*)buffer` / `*(int64_t*)buffer` is
        // misaligned-access undefined behavior -- benign on x86 but a real hazard (SIGBUS) on
        // stricter ISAs such as arm64, and flagged by UBSan. Copy the fixed-size header into a
        // naturally aligned local first (the struct layout matches the on-wire layout and its
        // size is validated in check_thrift_header), mirroring dsn_message_parser.
        thrift_message_header raw;
        memcpy(static_cast<void*>(&raw), buffer, sizeof(thrift_message_header));

        header.hdr_type = raw.hdr_type;
        header.hdr_version = be32toh(raw.hdr_version);
        header.hdr_length = be32toh(raw.hdr_length);
        header.hdr_crc32 = be32toh(raw.hdr_crc32);
        header.body_length = be32toh(raw.body_length);
        header.body_crc32 = be32toh(raw.body_crc32);
        header.app_id = be32toh(raw.app_id);
        header.partition_index = be32toh(raw.partition_index);
        header.client_timeout = be32toh(raw.client_timeout);
        header.client_thread_hash = be32toh(raw.client_thread_hash);
        header.client_partition_hash = be64toh(raw.client_partition_hash);
    }

    bool thrift_message_parser::check_thrift_header(const thrift_message_header& header)
    {
        if (header.hdr_type != THRIFT_HDR_SIG)
        {
            derror("hdr_type should be %s, but %s",
                message_parser::get_debug_string("THFT").c_str(),
                message_parser::get_debug_string((const char*)&header.hdr_type).c_str()
                );
            return false;
        }
        if (header.hdr_version != 0)
        {
            derror("hdr_version should be 0, but %u", header.hdr_version);
            return false;
        }
        if (header.hdr_length != sizeof(thrift_message_header))
        {
            derror("hdr_length should be %zu, but %u", sizeof(thrift_message_header), header.hdr_length);
            return false;
        }
        return true;
    }

    dsn::message_ex* thrift_message_parser::parse_message(const thrift_message_header& thrift_header, dsn::blob& message_data)
    {
        dsn::blob body_data = message_data.range(thrift_header.hdr_length);

        // A thrift RPC request always carries a non-empty message-begin envelope. An empty body
        // produces a receive message with no readable segment, which would make the read stream
        // constructor (dsn_msg_read_next) below throw std::out_of_range. Reject it up front through
        // the parser error channel so that exception can never escape and terminate the process on
        // a 48-byte header-only message from the network.
        if (body_data.length() == 0)
        {
            derror("thrift message body is empty");
            return nullptr;
        }

        dsn::message_ex* msg = message_ex::create_receive_message_with_standalone_header(body_data);
        if (msg == nullptr)
        {
            derror("thrift message creation failed");
            return nullptr;
        }

        dsn::message_header* dsn_hdr = msg->header;

        std::string fname;
        ::apache::thrift::protocol::TMessageType mtype = ::apache::thrift::protocol::T_CALL;
        int32_t seqid = 0;
        bool parsed = false;

        // The thrift message-begin envelope is decoded from untrusted network bytes. Constructing
        // the read stream and decoding can throw (out_of_range / bad_alloc from the read stream,
        // TProtocolException / out_of_range from the underlying binary_reader). Keep the entire
        // decode -- including the read stream setup -- inside the try/inner scope so any exception
        // is contained and the stream (and its dsn_msg_read_commit) is destroyed before we may free
        // msg on the failure path.
        {
            try
            {
                dsn::rpc_read_stream stream(msg);
                ::dsn::binary_reader_transport binary_transport(stream);
                boost::shared_ptr< ::dsn::binary_reader_transport > trans_ptr(&binary_transport, [](::dsn::binary_reader_transport*) {});
                ::apache::thrift::protocol::TBinaryProtocol iprot(trans_ptr);

                // Bound the decoder to the bytes actually received: no rpc name / container inside
                // the body can legitimately be longer than the message body itself. Thrift's string
                // and container size limits default to 0 (unlimited), so without this a tiny message
                // that claims a huge length makes TBinaryProtocol::readStringBody resize() to that
                // attacker-controlled size (up to ~2GB) up front, before the short read is detected
                // -- a memory-amplification DoS (a 56-byte message can force a multi-GB allocation).
                int32_t read_limit =
                    body_data.length() >
                            static_cast<uint32_t>((std::numeric_limits<int32_t>::max)())
                        ? (std::numeric_limits<int32_t>::max)()
                        : static_cast<int32_t>(body_data.length());
                iprot.setStringSizeLimit(read_limit);
                iprot.setContainerSizeLimit(read_limit);

                iprot.readMessageBegin(fname, mtype, seqid);
                parsed = true;
            }
            catch (std::exception& ex)
            {
                derror("thrift message begin parse failed: %s", ex.what());
            }
        }

        if (!parsed)
        {
            delete msg;
            return nullptr;
        }

        dinfo("rpc name: %s, type: %d, seqid: %d", fname.c_str(), mtype, seqid);

        dsn_hdr->hdr_type = THRIFT_HDR_SIG;
        dsn_hdr->hdr_length = sizeof(message_header);
        dsn_hdr->body_length = thrift_header.body_length;
        dsn_hdr->hdr_crc32 = dsn_hdr->body_crc32 = CRC_INVALID;

        dsn_hdr->id = seqid;
        int name_len = snprintf(dsn_hdr->rpc_name, sizeof(dsn_hdr->rpc_name), "%s", fname.c_str());
        if (name_len < 0 || static_cast<size_t>(name_len) >= sizeof(dsn_hdr->rpc_name))
        {
            derror("thrift rpc name is too long: %s", fname.c_str());
            delete msg;
            return nullptr;
        }
        dsn_hdr->gpid.u.app_id = thrift_header.app_id;
        dsn_hdr->gpid.u.partition_index = thrift_header.partition_index;
        dsn_hdr->client.timeout_ms = thrift_header.client_timeout;
        dsn_hdr->client.thread_hash = thrift_header.client_thread_hash;
        dsn_hdr->client.partition_hash = thrift_header.client_partition_hash;

        if (mtype == ::apache::thrift::protocol::T_CALL || mtype == ::apache::thrift::protocol::T_ONEWAY)
            dsn_hdr->context.u.is_request = 1;

        // Only requests are accepted here; a corrupt/unexpected message type used to abort via
        // dassert. Reject it through the parser's error channel (caller closes the connection)
        // instead of crashing the whole process on malformed network input.
        if (dsn_hdr->context.u.is_request != 1)
        {
            derror("thrift message is not a request, type = %d", mtype);
            delete msg;
            return nullptr;
        }
        dsn_hdr->context.u.serialize_format = DSF_THRIFT_BINARY; // always serialize in thrift binary

        return msg;
    }
}
