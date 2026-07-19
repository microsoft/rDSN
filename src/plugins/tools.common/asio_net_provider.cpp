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
 *     xxxx-xx-xx, author, first version
 *     xxxx-xx-xx, author, fix bug about xxx
 */

#include "asio_net_provider.h"
#include "asio_rpc_session.h"

#include <exception>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <limits>
#include <thread>

# ifdef __TITLE__
# undef __TITLE__
# endif
# define __TITLE__ "asio.net.provider"

namespace dsn {
    namespace tools{

        asio_network_provider::asio_network_provider(rpc_engine* srv, network* inner_provider)
            : connection_oriented_network(srv, inner_provider)
        {
            _acceptor = nullptr;
        }

        error_code asio_network_provider::start(rpc_channel channel, int port, bool client_only, io_modifer& ctx)
        {
            if (_acceptor != nullptr)
                return ERR_SERVICE_ALREADY_RUNNING;

            int io_service_worker_count = (int)dsn_config_get_value_uint64("network", "io_service_worker_count", 1,
                "thread number for io service (timer and boost network)");
            try
            {
                for (int i = 0; i < io_service_worker_count; i++)
                {
                    _workers.push_back(std::shared_ptr<std::thread>(new std::thread([this, ctx, i]()
                    {
                        task::set_tls_dsn_context(node(), nullptr, ctx.queue);

                        const char* name = ::dsn::tools::get_service_node_name(node());
                        char buffer[128];
                        int name_len = snprintf(buffer, sizeof(buffer), "%s.asio.%d", name, i);
                        if (name_len < 0 || static_cast<size_t>(name_len) >= sizeof(buffer))
                        {
                            dwarn("asio worker name is too long: %s", name);
                        }
                        task_worker::set_name(buffer);

                        boost::asio::io_service::work work(_io_service);
                        _io_service.run();
                    })));
                }
            }
            catch (const std::exception& err)
            {
                derror("failed to start asio tcp worker thread, err: %s", err.what());
                return ERR_NETWORK_START_FAILED;
            }

            _acceptor = nullptr;
                        
            if (channel != RPC_CHANNEL_TCP && channel != RPC_CHANNEL_UDP)
            {
                derror("invalid given channel %s", channel.to_string());
                return ERR_NOT_IMPLEMENTED;
            }

            _address.assign_ipv4(get_local_ipv4(), port);

            if (!client_only)
            {
                auto v4_addr = boost::asio::ip::address_v4::any(); //(ntohl(_address.ip));
                ::boost::asio::ip::tcp::endpoint ep(v4_addr, _address.port());

                try
                {
                    _acceptor.reset(new boost::asio::ip::tcp::acceptor(_io_service, ep, true));
                    do_accept();
                }
                catch (boost::system::system_error& err)
                {
                    derror("asio tcp listen on port %u failed, err: %s", port, err.what());
                    return ERR_ADDRESS_ALREADY_USED;
                }
            }            

            return ERR_OK;
        }

        rpc_session_ptr asio_network_provider::create_client_session(::dsn::rpc_address server_addr)
        {
            auto sock = std::shared_ptr<boost::asio::ip::tcp::socket>(new boost::asio::ip::tcp::socket(_io_service));
            message_parser_ptr parser(new_message_parser(_client_hdr_format));
            return rpc_session_ptr(new asio_rpc_session(*this, server_addr, sock, parser, true));
        }

        void asio_network_provider::do_accept()
        {
            auto socket = std::shared_ptr<boost::asio::ip::tcp::socket>(
                new boost::asio::ip::tcp::socket(_io_service));

            _acceptor->async_accept(*socket,
                [this, socket](boost::system::error_code ec)
            {
                if (!ec)
                {
                    // The peer may have already reset the just-accepted connection. Query the
                    // remote endpoint with an error_code so a disconnected socket returns an
                    // error instead of throwing boost::system::system_error, which would be
                    // uncaught on this io thread and abort the whole process. On failure, drop
                    // this connection and keep accepting.
                    boost::system::error_code ep_ec;
                    auto endpoint = socket->remote_endpoint(ep_ec);
                    if (ep_ec)
                    {
                        derror("failed to get the remote endpoint of an accepted connection, drop it: %s",
                            ep_ec.message().c_str());
                    }
                    else
                    {
                        auto ip = endpoint.address().to_v4().to_ulong();
                        auto port = endpoint.port();
                        ::dsn::rpc_address client_addr(ip, port);

                        message_parser_ptr null_parser;
                        rpc_session_ptr s = new asio_rpc_session(*this, client_addr, 
                            (std::shared_ptr<boost::asio::ip::tcp::socket>&)socket,
                            null_parser, false);
                        this->on_server_session_accepted(s);
                    }
                }

                do_accept();
            });
        }

        void asio_udp_provider::send_message(message_ex* request)
        {
            auto parser = get_message_parser(request->hdr_format);
            parser->prepare_on_send(request);
            auto lcount = parser->get_buffer_count_on_send(request);
            std::unique_ptr<message_parser::send_buf[]> bufs(new message_parser::send_buf[lcount]);
            auto rcount = parser->get_buffers_on_send(request, bufs.get());
            dassert(lcount >= rcount, "");

            size_t tlen = 0, offset = 0;
            for (int i = 0; i < rcount; i ++)
            {
                tlen += bufs[i].sz;
            }
            if (tlen > max_udp_packet_size)
            {
                // Do not abort the whole process because one message is oversized: drop it and
                // let the rpc matcher time the call out (same as the send-failure path below).
                derror("the message (%zu bytes) is too large to send via a udp channel (max %zu); dropping it",
                    tlen,
                    max_udp_packet_size);
                return;
            }

            auto packet_buffer = std::make_shared<std::vector<char>>(tlen);
            for (int i = 0; i < rcount; i ++)
            {
                memcpy(packet_buffer->data() + offset, bufs[i].buf, bufs[i].sz);
                offset += bufs[i].sz;
            };

            ::boost::asio::ip::udp::endpoint ep(::boost::asio::ip::address_v4(request->to_address.ip()), request->to_address.port());
            auto completion = detail::retain_udp_packet_until_send_complete(
                packet_buffer,
                [ep](const boost::system::error_code& error, std::size_t bytes_transferred)
                {
                    (void)bytes_transferred;
                    if (error) {
                        dwarn("send udp packet to ep %s:%d failed, message = %s", ep.address().to_string().c_str(), ep.port(), error.message().c_str());
                        //we do not handle failure here, rpc matcher would handle timeouts
                    }
                });
            _socket->async_send_to(
                ::boost::asio::buffer(packet_buffer->data(), packet_buffer->size()),
                ep,
                std::move(completion));
        }

        asio_udp_provider::asio_udp_provider(rpc_engine* srv, network* inner_provider)
            : network(srv, inner_provider),
              _is_client(false),
              _recv_reader(_message_buffer_block_size)
        {
            _parsers = new message_parser*[network_header_format::max_value() + 1];
            memset(_parsers, 0, sizeof(message_parser*) * (network_header_format::max_value() + 1));
        }

        asio_udp_provider::~asio_udp_provider()
        {
            for (int i = 0; i <= network_header_format::max_value(); i++)
            {
                if (_parsers[i] != nullptr)
                {
                    delete _parsers[i];
                    _parsers[i] = nullptr;
                }
            }
            delete []_parsers;
            _parsers = nullptr;
        }

        message_parser* asio_udp_provider::get_message_parser(network_header_format hdr_format)
        {
            if (_parsers[hdr_format] == nullptr)
            {
                utils::auto_lock<utils::ex_lock_nr> l(_lock);
                if (_parsers[hdr_format] == nullptr) // double check
                {
                    _parsers[hdr_format] = new_message_parser(hdr_format);
                }
            }
            return _parsers[hdr_format];
        }

        bool asio_udp_provider::do_receive()
        {
            std::shared_ptr< ::boost::asio::ip::udp::endpoint> send_endpoint(new ::boost::asio::ip::udp::endpoint);

            _recv_reader.truncate_read();
            auto buffer_ptr = _recv_reader.read_buffer_ptr(max_udp_packet_size);
            if (buffer_ptr == nullptr || _recv_reader.read_buffer_capacity() < max_udp_packet_size)
            {
                derror("%s: asio udp read failed: unable to prepare read buffer", _address.to_string());
                return false;
            }

            _socket->async_receive_from(
                ::boost::asio::buffer(buffer_ptr, max_udp_packet_size),
                *send_endpoint,
                [this, send_endpoint](const boost::system::error_code& error, std::size_t bytes_transferred)
                {
                    if (!!error)
                    {
                        derror("%s: asio udp read failed: %s", _address.to_string(), error.message().c_str());
                        do_receive();
                        return;
                    }

                    if (bytes_transferred < sizeof(uint32_t))
                    {
                        derror("%s: asio udp read failed: too short message", _address.to_string());
                        do_receive();
                        return;
                    }

                    auto hdr_format = message_parser::get_header_type(_recv_reader._buffer.data());
                    if (NET_HDR_INVALID == hdr_format)
                    {
                        derror("%s: asio udp read failed: invalid header type '%s'", 
                            _address.to_string(), 
                            message_parser::get_debug_string(_recv_reader._buffer.data()).c_str()
                            );
                        do_receive();
                        return;
                    }

                    auto parser = get_message_parser(hdr_format);
                    parser->reset();

                    _recv_reader.mark_read(bytes_transferred);

                    int read_next = -1;

                    message_ex* msg = parser->get_message_on_receive(&_recv_reader, read_next);
                    if (msg == nullptr)
                    {
                        derror("%s: asio udp read failed: invalid udp packet", _address.to_string());
                        do_receive();
                        return;
                    }

                    msg->to_address = _address;
                    if (msg->header->context.u.is_request)
                    {
                        on_recv_request(msg, 0);
                    }
                    else
                    {
                        on_recv_reply(msg->header->id, msg, 0);
                    }

                    do_receive();
                }
            );

            return true;
        }

        error_code asio_udp_provider::start(rpc_channel channel, int port, bool client_only, io_modifer& ctx)
        {
            _is_client = client_only;
            int io_service_worker_count = (int)dsn_config_get_value_uint64("network", "io_service_worker_count", 1,
                                                                   "thread number for io service (timer and boost network)");
           
            if (channel != RPC_CHANNEL_UDP)
            {
                derror("invalid given channel %s", channel.to_string());
                return ERR_NOT_IMPLEMENTED;
            }

            if (client_only)
            {
                static const int max_client_port_retry_count = 10;
                int retry_count = 0;
                for (; retry_count < max_client_port_retry_count; retry_count++)
                {
                    //FIXME: we actually do not need to set a random port for client if the rpc_engine is refactored
                    _address.assign_ipv4(get_local_ipv4(), (std::numeric_limits<uint16_t>::max)() -
                        dsn_random64((std::numeric_limits<uint64_t>::min)(),
                                     (std::numeric_limits<uint64_t>::max)()) % 5000);
                    ::boost::asio::ip::udp::endpoint ep(boost::asio::ip::address_v4::any(), _address.port());
                    try
                    {
                        _socket.reset(new ::boost::asio::ip::udp::socket(_io_service, ep));
                        break;
                    }
                    catch (boost::system::system_error& err)
                    {
                        if (err.code() == boost::asio::error::address_in_use)
                        {
                            ddebug("asio udp listen on port %u failed, err: %s", _address.port(), err.what());
                            continue;
                        }

                        derror("asio udp listen on port %u failed, err: %s", _address.port(), err.what());
                        return ERR_NETWORK_START_FAILED;
                    }
                }

                if (retry_count >= max_client_port_retry_count)
                {
                    derror("asio udp failed to find an available client port after %d retries",
                           max_client_port_retry_count);
                    return ERR_ADDRESS_ALREADY_USED;
                }
            }
            else
            {
                _address.assign_ipv4(get_local_ipv4(), port);
                ::boost::asio::ip::udp::endpoint ep(boost::asio::ip::address_v4::any(), _address.port());
                try
                {
                    _socket.reset(new ::boost::asio::ip::udp::socket(_io_service, ep));
                }
                catch (boost::system::system_error& err)
                {
                    derror("asio udp listen on port %u failed, err: %s", port, err.what());
                    return ERR_ADDRESS_ALREADY_USED;
                }
            }

            try
            {
                for (int i = 0; i < io_service_worker_count; i++)
                {
                    _workers.push_back(std::shared_ptr<std::thread>(new std::thread([this, ctx, i]()
                    {
                        task::set_tls_dsn_context(node(), nullptr, ctx.queue);

                        const char* name = ::dsn::tools::get_service_node_name(node());
                        char buffer[128];
                        int name_len = snprintf(buffer, sizeof(buffer), "%s.asio.udp.%d.%d", name, (int)(this->address().port()), i);
                        if (name_len < 0 || static_cast<size_t>(name_len) >= sizeof(buffer))
                        {
                            dwarn("asio udp worker name is too long: %s", name);
                        }
                        task_worker::set_name(buffer);

                        boost::asio::io_service::work work(_io_service);
                        _io_service.run();
                    })));
                }
            }
            catch (const std::exception& err)
            {
                derror("failed to start asio udp worker thread, err: %s", err.what());
                return ERR_NETWORK_START_FAILED;
            }

            if (!do_receive())
            {
                return ERR_NETWORK_START_FAILED;
            }

            return ERR_OK;
        }
    }
}
