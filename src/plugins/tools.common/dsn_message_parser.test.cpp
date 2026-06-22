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

#include "dsn_message_parser.h"
#include <gtest/gtest.h>
#include <cstring>

using namespace dsn;

namespace
{

void init_header(message_header &header)
{
    header = message_header();
    header.hdr_length = sizeof(message_header);
    header.hdr_crc32 = CRC_INVALID;
    header.body_crc32 = CRC_INVALID;
    header.body_length = 0;
    header.context.u.is_request = 1;
    const char rpc_name[] = "RPC_TEST";
    memcpy(header.rpc_name, rpc_name, sizeof(rpc_name));
}

void set_reader_header(message_reader &reader, const message_header &header)
{
    char *buffer = reader.read_buffer_ptr(sizeof(header));
    memcpy(buffer, &header, sizeof(header));
    reader.mark_read(sizeof(header));
}

}

TEST(tools_common, dsn_message_parser_rejects_non_terminated_names)
{
    message_header header;
    int read_next = 0;

    init_header(header);
    memset(header.rpc_name, 'r', sizeof(header.rpc_name));
    dsn_message_parser parser;
    message_reader reader(4096);
    set_reader_header(reader, header);
    ASSERT_EQ(nullptr, parser.get_message_on_receive(&reader, read_next));
    ASSERT_EQ(-1, read_next);

    init_header(header);
    memset(header.server.error_name, 'e', sizeof(header.server.error_name));
    parser.reset();
    message_reader reader2(4096);
    set_reader_header(reader2, header);
    ASSERT_EQ(nullptr, parser.get_message_on_receive(&reader2, read_next));
    ASSERT_EQ(-1, read_next);
}
