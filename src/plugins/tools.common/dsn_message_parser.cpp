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

# include "dsn_message_parser.h"
# include <dsn/service_api_c.h>
# include <cstring>
# include <cinttypes>

# ifdef __TITLE__
# undef __TITLE__
# endif
# define __TITLE__ "dsn.message.parser"

namespace dsn
{
    void dsn_message_parser::reset()
    {
        _header_checked = false;
    }

    message_ex* dsn_message_parser::get_message_on_receive(message_reader* reader, /*out*/ int& read_next)
    {
        read_next = 4096;

        dsn::blob& buf = reader->_buffer;
        char* buf_ptr = (char*)buf.data();
        unsigned int buf_len = reader->_buffer_occupied;

        if (buf_len >= sizeof(message_header))
        {
            // The header sits at an arbitrary offset inside the shared receive buffer,
            // so buf_ptr is generally unaligned. Read its fields through a naturally
            // aligned stack copy to avoid misaligned-access undefined behavior (benign
            // on x86 but a real hazard on stricter ISAs such as arm64).
            // create_receive_message likewise stores the header in aligned storage.
            message_header hdr_copy;
            memcpy(static_cast<void*>(&hdr_copy), buf_ptr, sizeof(message_header));

            if (!_header_checked)
            {
                if (!is_right_header(reinterpret_cast<char*>(&hdr_copy)))
                {
                    derror("dsn message header check failed");
                    read_next = -1;
                    return nullptr;
                }
                else
                {
                    _header_checked = true;
                }
            }

            unsigned int body_length = hdr_copy.body_length;

            // body_length comes from the wire; compute the total size in 64-bit and reject a
            // value that would overflow the 32-bit msg_sz and wrap to a small size (which would
            // desynchronize the stream and cause the rest of the message to be misparsed).
            uint64_t msg_sz64 = (uint64_t)sizeof(message_header) + body_length;
            if (msg_sz64 > UINT32_MAX)
            {
                derror("dsn message body length is too large: %u", body_length);
                read_next = -1;
                return nullptr;
            }
            unsigned int msg_sz = (unsigned int)msg_sz64;

            // msg done
            if (buf_len >= msg_sz)
            {
                dsn::blob msg_bb = buf.range(0, msg_sz);
                message_ex* msg = message_ex::create_receive_message(msg_bb);
                if (msg == nullptr)
                {
                    derror("dsn message creation failed, id = %" PRIu64 ", trace_id = %016" PRIx64 ", rpc_name = %s, from_addr = %s",
                           hdr_copy.id, hdr_copy.trace_id, hdr_copy.rpc_name, hdr_copy.from_address.to_string());
                    read_next = -1;
                    return nullptr;
                }
                if (!is_right_body(msg))
                {
                    derror("dsn message body check failed, id = %" PRIu64 ", trace_id = %016" PRIx64 ", rpc_name = %s, from_addr = %s",
                           hdr_copy.id, hdr_copy.trace_id, hdr_copy.rpc_name, hdr_copy.from_address.to_string());
                    read_next = -1;
                    return nullptr;
                }
                else
                {
                    reader->_buffer = buf.range(msg_sz);
                    reader->_buffer_occupied -= msg_sz;
                    _header_checked = false;
                    read_next = (reader->_buffer_occupied >= sizeof(message_header) ?
                                     0 : sizeof(message_header) - reader->_buffer_occupied);
                    msg->hdr_format = NET_HDR_DSN;
                    return msg;
                }
            }
            else // buf_len < msg_sz
            {
                read_next = msg_sz - buf_len;
                return nullptr;
            }
        }
        else // buf_len < sizeof(message_header)
        {
            read_next = sizeof(message_header) - buf_len;
            return nullptr;
        }
    }

    void dsn_message_parser::prepare_on_send(message_ex* msg)
    {
        auto& header = msg->header;
        auto& buffers = msg->buffers;

#ifndef NDEBUG
        int i_max = (int)buffers.size() - 1;
        size_t len = 0;
        for (int i = 0; i <= i_max; i++)
        {
            len += (size_t)buffers[i].length();
        }
        dassert(len == (size_t)header->body_length + sizeof(message_header),
            "data length is wrong");
#endif

        if (task_spec::get(msg->local_rpc_code)->rpc_message_crc_required)
        {
            // compute data crc if necessary (only once for the first time)
            if (header->body_crc32 == CRC_INVALID)
            {
                int i_max = (int)buffers.size() - 1;
                uint32_t crc32 = 0;
                size_t len = 0;
                for (int i = 0; i <= i_max; i++)
                {
                    uint32_t lcrc;
                    const void* ptr;
                    size_t sz;

                    if (i == 0)
                    {
                        ptr = (const void*)(buffers[i].data() + sizeof(message_header));
                        sz = (size_t)buffers[i].length() - sizeof(message_header);
                    }
                    else
                    {
                        ptr = (const void*)buffers[i].data();
                        sz = (size_t)buffers[i].length();
                    }

                    lcrc = dsn_crc32_compute(ptr, sz, crc32);
                    crc32 = dsn_crc32_concatenate(
                        0,
                        0, crc32, len,
                        crc32, lcrc, sz
                        );

                    len += sz;
                }

                dassert  (len == (size_t)header->body_length, "data length is wrong");
                header->body_crc32 = crc32;
            }

            // always compute header crc
            header->hdr_crc32 = CRC_INVALID;
            header->hdr_crc32 = dsn_crc32_compute(header, sizeof(message_header), 0);
        }
    }

    int dsn_message_parser::get_buffer_count_on_send(message_ex* msg)
    {
        return (int)msg->buffers.size();
    }

    int dsn_message_parser::get_buffers_on_send(message_ex* msg, /*out*/ send_buf* buffers)
    {
        int i = 0;        
        for (auto& buf : msg->buffers)
        {
            buffers[i].buf = (void*)buf.data();
            buffers[i].sz = buf.length();
            ++i;
        }
        return i;
    }

    /*static*/ bool dsn_message_parser::is_right_header(char* hdr)
    {
        // rpc_name and error_name are consumed as C-strings throughout the runtime (handler
        // lookup, task/error code resolution, "%s" logging, ...). A legitimate sender's names are
        // always shorter than the field, so a header whose rpc_name/error_name fills the whole
        // field with no NUL terminator is malformed: reject it here at the trust boundary instead
        // of reading past the field downstream. We validate (not mutate) the received buffer.
        message_header* header = reinterpret_cast<message_header*>(hdr);
        if (memchr(header->rpc_name, '\0', sizeof(header->rpc_name)) == nullptr)
        {
            derror("dsn message header check failed: rpc_name is not null-terminated");
            return false;
        }
        if (memchr(header->server.error_name, '\0', sizeof(header->server.error_name)) == nullptr)
        {
            derror("dsn message header check failed: error_name is not null-terminated");
            return false;
        }

        // from_address is carried on the wire and, by contract, is always a plain ipv4
        // address (or the INVALID sentinel that the network layer later replaces with the
        // peer's real address). GROUP/URI addresses hold process-local heap pointers whose
        // values are meaningless -- and unsafe to dereference -- when they arrive from an
        // untrusted peer. Reject them here at the trust boundary before any downstream code
        // (e.g. from_address.to_string() logging, or dispatch) dereferences a bogus pointer.
        dsn_host_type_t from_type = header->from_address.type();
        if (from_type != HOST_TYPE_INVALID && from_type != HOST_TYPE_IPV4)
        {
            derror("dsn message header check failed: invalid from_address type %d", (int)from_type);
            return false;
        }

        uint32_t* pcrc = reinterpret_cast<uint32_t*>(hdr + FIELD_OFFSET(message_header, hdr_crc32));
        uint32_t crc32 = *pcrc;
        if (crc32 != CRC_INVALID)
        {
            *pcrc = CRC_INVALID;
            bool r = (crc32 == dsn_crc32_compute(hdr, sizeof(message_header), 0));
            *pcrc = crc32;
            if (!r)
            {
                derror("dsn message header crc check failed");
            }
            return r;
        }

        // crc is not enabled
        else
        {
            return true;
        }
    }

    /*static*/ bool dsn_message_parser::is_right_body(message_ex* msg)
    {
        auto& header = msg->header;
        auto& buffers = msg->buffers;

        if (header->body_crc32 != CRC_INVALID)
        {
            int i_max = (int)buffers.size() - 1;
            uint32_t crc32 = 0;
            size_t len = 0;
            for (int i = 0; i <= i_max; i++)
            {
                const void* ptr = (const void*)buffers[i].data();
                size_t sz = (size_t)buffers[i].length();

                uint32_t lcrc = dsn_crc32_compute(ptr, sz, crc32);
                crc32 = dsn_crc32_concatenate(
                    0,
                    0, crc32, len,
                    crc32, lcrc, sz
                    );

                len += sz;
            }

            dassert(len == (size_t)header->body_length, "data length is wrong");

            bool r = (header->body_crc32 == crc32);
            if (!r)
            {
                derror("dsn message body crc check failed");
            }
            return r;
        }

        // crc is not enabled
        else
        {
            return true;
        }
    }
}
