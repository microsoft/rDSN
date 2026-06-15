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


# include <dsn/cpp/utils.h>
# include <dsn/cpp/blob.h>
# include <dsn/utility/link.h>
# include <dsn/utility/autoref_ptr.h>
# include <gtest/gtest.h>
# include <limits>
# include <sstream>
# include <string>
# if defined(_WIN32)
# include <basetsd.h>
# else
# include <sys/types.h>
# endif

using namespace ::dsn;
using namespace ::dsn::utils;

TEST(core, get_last_component)
{
    ASSERT_EQ("a", get_last_component("a", "/"));
    ASSERT_EQ("b", get_last_component("a/b", "/"));
    ASSERT_EQ("b", get_last_component("a//b", "/"));
    ASSERT_EQ("", get_last_component("a/", "/"));
    ASSERT_EQ("c", get_last_component("a/b_c", "/_"));
}

TEST(core, crc)
{
    char buffer[24];
    for (int i = 0; i < sizeof(buffer) / sizeof(char); i++)
    {
        buffer[i] = dsn_random32(0, 200);
    }

    auto c1 = dsn_crc32_compute(buffer, 12, 0);
    auto c2 = dsn_crc32_compute(buffer + 12, 12, c1);
    auto c3 = dsn_crc32_compute(buffer, 24, 0);
    auto c4 = dsn_crc32_concatenate(0, 0, c1, 12, c1, c2, 12);
    EXPECT_TRUE(c3 == c4);
}

TEST(core, binary_io)
{
    int value = 0xdeadbeef;
    binary_writer writer;
    writer.write(value);

    auto buf = writer.get_buffer();
    binary_reader reader(buf);
    int value3;
    reader.read(value3);

    EXPECT_TRUE(value3 == value);
}

TEST(core, binary_io_string)
{
    // an empty string round-trips as empty and consumes only the length prefix
    {
        binary_writer writer;
        writer.write(std::string());

        binary_reader reader(writer.get_buffer());
        std::string s = "should be overwritten";
        int consumed = reader.read(s);

        EXPECT_TRUE(s.empty());
        EXPECT_EQ((int)sizeof(int), consumed);
        EXPECT_EQ(0, reader.get_remaining_size());
    }

    // a non-empty string round-trips and consumes the length prefix + payload
    {
        std::string value = "hello, rDSN";
        binary_writer writer;
        writer.write(value);

        binary_reader reader(writer.get_buffer());
        std::string s;
        int consumed = reader.read(s);

        EXPECT_EQ(value, s);
        EXPECT_EQ((int)(sizeof(int) + value.size()), consumed);
        EXPECT_EQ(0, reader.get_remaining_size());
    }
}

TEST(core, binary_io_blob)
{
    // an empty blob round-trips as empty AND resets a previously non-empty output blob
    {
        binary_writer writer;
        writer.write(blob());

        binary_reader reader(writer.get_buffer());

        std::string preset = "preset";
        auto preset_buf = make_shared_array<char>(preset.size());
        memcpy(preset_buf.get(), preset.data(), preset.size());
        blob b(preset_buf, (unsigned int)preset.size()); // start non-empty

        int consumed = reader.read(b);

        EXPECT_EQ(0u, b.length()); // must be reset to empty, not left stale
        EXPECT_EQ((int)sizeof(int), consumed);
        EXPECT_EQ(0, reader.get_remaining_size());
    }

    // a non-empty blob round-trips with matching length and content
    {
        std::string content = "blob content";
        auto src = make_shared_array<char>(content.size());
        memcpy(src.get(), content.data(), content.size());
        blob in(src, (unsigned int)content.size());

        binary_writer writer;
        writer.write(in);

        binary_reader reader(writer.get_buffer());
        blob out;
        int consumed = reader.read(out);

        EXPECT_EQ((unsigned int)content.size(), out.length());
        EXPECT_EQ(0, memcmp(out.data(), content.data(), content.size()));
        EXPECT_EQ((int)(sizeof(int) + content.size()), consumed);
        EXPECT_EQ(0, reader.get_remaining_size());
    }
}


TEST(core, split_args)
{
    std::string value = "a ,b, c ";
    std::vector<std::string> sargs;
    std::list<std::string> sargs2;
    ::dsn::utils::split_args(value.c_str(), sargs, ',');
    ::dsn::utils::split_args(value.c_str(), sargs2, ',');

    EXPECT_EQ(sargs.size(), 3);
    EXPECT_EQ(sargs[0], "a");
    EXPECT_EQ(sargs[1], "b");
    EXPECT_EQ(sargs[2], "c");

    EXPECT_EQ(sargs2.size(), 3);
    auto it = sargs2.begin();
    EXPECT_EQ(*it++, "a");
    EXPECT_EQ(*it++, "b");
    EXPECT_EQ(*it++, "c");
}

TEST(core, trim_string)
{
    std::string value = " x x x x ";
    auto r = trim_string((char*)value.c_str());
    EXPECT_EQ(std::string(r), "x x x x");
}

TEST(core, host_name)
{
    auto hostname = asio::host_name();
    EXPECT_FALSE(hostname.empty());
    EXPECT_EQ(std::string::npos, hostname.find('\0'));
}

static std::string signed_integer_to_string(std::intmax_t value)
{
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

static std::string unsigned_integer_to_string(std::uintmax_t value)
{
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

TEST(core, lexical_cast_integer_accepts_valid_values)
{
    EXPECT_EQ(-12, static_cast<int>(lexical_cast<int8_t>("-12")));
    EXPECT_EQ(12, static_cast<int>(lexical_cast<int8_t>("12")));
    EXPECT_EQ(123, static_cast<int>(lexical_cast<uint8_t>("123")));

    EXPECT_EQ((int16_t)-12345, lexical_cast<int16_t>("-12345"));
    EXPECT_EQ((uint16_t)12345, lexical_cast<uint16_t>("12345"));

    EXPECT_EQ((int32_t)-123456789, lexical_cast<int32_t>("-123456789"));
    EXPECT_EQ((uint32_t)123456789u, lexical_cast<uint32_t>("123456789"));

    EXPECT_EQ((int64_t)-123456789012345LL, lexical_cast<int64_t>("-123456789012345"));
    EXPECT_EQ((uint64_t)123456789012345ULL, lexical_cast<uint64_t>("123456789012345"));

    EXPECT_EQ((size_t)42, lexical_cast<size_t>("42"));
# if defined(_WIN32)
    EXPECT_EQ((SSIZE_T)-42, lexical_cast<SSIZE_T>("-42"));
    EXPECT_EQ((SSIZE_T)42, lexical_cast<SSIZE_T>("42"));
# else
    EXPECT_EQ((ssize_t)-42, lexical_cast<ssize_t>("-42"));
    EXPECT_EQ((ssize_t)42, lexical_cast<ssize_t>("42"));
# endif
    EXPECT_EQ(123, lexical_cast<int>("+123"));

    EXPECT_EQ(static_cast<int>(std::numeric_limits<int8_t>::min()),
              static_cast<int>(lexical_cast<int8_t>(
                  signed_integer_to_string(std::numeric_limits<int8_t>::min()))));
    EXPECT_EQ(static_cast<int>(std::numeric_limits<int8_t>::max()),
              static_cast<int>(lexical_cast<int8_t>(
                  signed_integer_to_string(std::numeric_limits<int8_t>::max()))));
    EXPECT_EQ(static_cast<int>(std::numeric_limits<uint8_t>::min()),
              static_cast<int>(lexical_cast<uint8_t>(
                  unsigned_integer_to_string(std::numeric_limits<uint8_t>::min()))));
    EXPECT_EQ(static_cast<int>(std::numeric_limits<uint8_t>::max()),
              static_cast<int>(lexical_cast<uint8_t>(
                  unsigned_integer_to_string(std::numeric_limits<uint8_t>::max()))));

    EXPECT_EQ(std::numeric_limits<int16_t>::min(),
              lexical_cast<int16_t>(
                  signed_integer_to_string(std::numeric_limits<int16_t>::min())));
    EXPECT_EQ(std::numeric_limits<int16_t>::max(),
              lexical_cast<int16_t>(
                  signed_integer_to_string(std::numeric_limits<int16_t>::max())));
    EXPECT_EQ(std::numeric_limits<uint16_t>::min(),
              lexical_cast<uint16_t>(
                  unsigned_integer_to_string(std::numeric_limits<uint16_t>::min())));
    EXPECT_EQ(std::numeric_limits<uint16_t>::max(),
              lexical_cast<uint16_t>(
                  unsigned_integer_to_string(std::numeric_limits<uint16_t>::max())));

    EXPECT_EQ(std::numeric_limits<int32_t>::min(),
              lexical_cast<int32_t>(
                  signed_integer_to_string(std::numeric_limits<int32_t>::min())));
    EXPECT_EQ(std::numeric_limits<int32_t>::max(),
              lexical_cast<int32_t>(
                  signed_integer_to_string(std::numeric_limits<int32_t>::max())));
    EXPECT_EQ(std::numeric_limits<uint32_t>::min(),
              lexical_cast<uint32_t>(
                  unsigned_integer_to_string(std::numeric_limits<uint32_t>::min())));
    EXPECT_EQ(std::numeric_limits<uint32_t>::max(),
              lexical_cast<uint32_t>(
                  unsigned_integer_to_string(std::numeric_limits<uint32_t>::max())));

    EXPECT_EQ(std::numeric_limits<int64_t>::min(),
              lexical_cast<int64_t>(
                  signed_integer_to_string(std::numeric_limits<int64_t>::min())));
    EXPECT_EQ(std::numeric_limits<int64_t>::max(),
              lexical_cast<int64_t>(
                  signed_integer_to_string(std::numeric_limits<int64_t>::max())));
    EXPECT_EQ(std::numeric_limits<uint64_t>::min(),
              lexical_cast<uint64_t>(
                  unsigned_integer_to_string(std::numeric_limits<uint64_t>::min())));
    EXPECT_EQ(std::numeric_limits<uint64_t>::max(),
              lexical_cast<uint64_t>(
                  unsigned_integer_to_string(std::numeric_limits<uint64_t>::max())));

    EXPECT_EQ(std::numeric_limits<size_t>::min(),
              lexical_cast<size_t>(
                  unsigned_integer_to_string(std::numeric_limits<size_t>::min())));
    EXPECT_EQ(std::numeric_limits<size_t>::max(),
              lexical_cast<size_t>(
                  unsigned_integer_to_string(std::numeric_limits<size_t>::max())));
# if defined(_WIN32)
    EXPECT_EQ(std::numeric_limits<SSIZE_T>::min(),
              lexical_cast<SSIZE_T>(
                  signed_integer_to_string(std::numeric_limits<SSIZE_T>::min())));
    EXPECT_EQ(std::numeric_limits<SSIZE_T>::max(),
              lexical_cast<SSIZE_T>(
                  signed_integer_to_string(std::numeric_limits<SSIZE_T>::max())));
# else
    EXPECT_EQ(std::numeric_limits<ssize_t>::min(),
              lexical_cast<ssize_t>(
                  signed_integer_to_string(std::numeric_limits<ssize_t>::min())));
    EXPECT_EQ(std::numeric_limits<ssize_t>::max(),
              lexical_cast<ssize_t>(
                  signed_integer_to_string(std::numeric_limits<ssize_t>::max())));
# endif
}

TEST(core, lexical_cast_integer_rejects_invalid_values)
{
    EXPECT_THROW(lexical_cast<int>(""), std::invalid_argument);
    EXPECT_THROW(lexical_cast<int>(" 123"), std::invalid_argument);
    EXPECT_THROW(lexical_cast<int>("123 "), std::invalid_argument);
    EXPECT_THROW(lexical_cast<int>("12 3"), std::invalid_argument);
    EXPECT_THROW(lexical_cast<int>("123abc"), std::invalid_argument);
    EXPECT_THROW(lexical_cast<int>("--12"), std::invalid_argument);
    EXPECT_THROW(lexical_cast<int>("-"), std::invalid_argument);
    EXPECT_THROW(lexical_cast<int>("+"), std::invalid_argument);
    EXPECT_THROW(lexical_cast<unsigned int>("-"), std::invalid_argument);
    EXPECT_THROW(lexical_cast<unsigned int>("+"), std::invalid_argument);

    EXPECT_THROW(lexical_cast<int8_t>("-129"), std::out_of_range);
    EXPECT_THROW(lexical_cast<int8_t>("128"), std::out_of_range);
    EXPECT_THROW(lexical_cast<uint8_t>("-1"), std::out_of_range);
    EXPECT_THROW(lexical_cast<uint8_t>("256"), std::out_of_range);
    EXPECT_THROW(lexical_cast<int16_t>("-32769"), std::out_of_range);
    EXPECT_THROW(lexical_cast<int16_t>("32768"), std::out_of_range);
    EXPECT_THROW(lexical_cast<uint16_t>("65536"), std::out_of_range);
    EXPECT_THROW(lexical_cast<uint16_t>("-1"), std::out_of_range);
    EXPECT_THROW(lexical_cast<int32_t>("-2147483649"), std::out_of_range);
    EXPECT_THROW(lexical_cast<int32_t>("2147483648"), std::out_of_range);
    EXPECT_THROW(lexical_cast<uint32_t>("-1"), std::out_of_range);
    EXPECT_THROW(lexical_cast<uint32_t>("4294967296"), std::out_of_range);
    EXPECT_THROW(lexical_cast<int64_t>("-9223372036854775809"), std::out_of_range);
    EXPECT_THROW(lexical_cast<int64_t>("9223372036854775808"), std::out_of_range);
    EXPECT_THROW(lexical_cast<uint64_t>("18446744073709551616"), std::out_of_range);
    EXPECT_THROW(lexical_cast<size_t>("-1"), std::out_of_range);
    EXPECT_THROW(lexical_cast<size_t>("18446744073709551616"), std::out_of_range);
    EXPECT_THROW(lexical_cast<uint64_t>("-1"), std::out_of_range);
# if defined(_WIN32)
    EXPECT_THROW(lexical_cast<SSIZE_T>("-9223372036854775809"), std::out_of_range);
    EXPECT_THROW(lexical_cast<SSIZE_T>("9223372036854775808"), std::out_of_range);
# else
    EXPECT_THROW(lexical_cast<ssize_t>("-9223372036854775809"), std::out_of_range);
    EXPECT_THROW(lexical_cast<ssize_t>("9223372036854775808"), std::out_of_range);
# endif

    EXPECT_THROW(lexical_cast<int64_t>("9223372036854775808 "), std::invalid_argument);
    EXPECT_THROW(lexical_cast<int64_t>(" 9223372036854775808"), std::invalid_argument);
    EXPECT_THROW(lexical_cast<int64_t>("9223372036854775808ab"), std::invalid_argument);
    EXPECT_THROW(lexical_cast<int64_t>("9223372036854775808 ab"), std::invalid_argument);
    EXPECT_THROW(lexical_cast<int64_t>("-9223372036854775809 "), std::invalid_argument);
    EXPECT_THROW(lexical_cast<uint64_t>("18446744073709551616 "), std::invalid_argument);
    EXPECT_THROW(lexical_cast<uint64_t>("18446744073709551616ab"), std::invalid_argument);

    EXPECT_THROW(lexical_cast<int64_t>("9223372036854775807 "), std::invalid_argument);
    EXPECT_THROW(lexical_cast<int64_t>(" 9223372036854775807"), std::invalid_argument);
    EXPECT_THROW(lexical_cast<int64_t>("9223372036854775807ab"), std::invalid_argument);
    EXPECT_THROW(lexical_cast<uint64_t>("18446744073709551615 "), std::invalid_argument);
    EXPECT_THROW(lexical_cast<uint64_t>("18446744073709551615ab"), std::invalid_argument);
}

TEST(core, lexical_cast_bool_accepts_valid_values)
{
    EXPECT_FALSE(lexical_cast<bool>("0"));
    EXPECT_TRUE(lexical_cast<bool>("1"));
}

TEST(core, lexical_cast_bool_rejects_invalid_values)
{
    EXPECT_THROW(lexical_cast<bool>(""), std::invalid_argument);
    EXPECT_THROW(lexical_cast<bool>("2"), std::invalid_argument);
    EXPECT_THROW(lexical_cast<bool>("true"), std::invalid_argument);
    EXPECT_THROW(lexical_cast<bool>("false"), std::invalid_argument);
    EXPECT_THROW(lexical_cast<bool>("False"), std::invalid_argument);
    EXPECT_THROW(lexical_cast<bool>(" 1"), std::invalid_argument);
    EXPECT_THROW(lexical_cast<bool>("1 "), std::invalid_argument);
}

TEST(core, lexical_cast_floating_point_accepts_valid_values)
{
    EXPECT_FLOAT_EQ(1.25f, lexical_cast<float>("1.25"));
    EXPECT_DOUBLE_EQ(-0.03125, lexical_cast<double>("-3.125e-2"));
    EXPECT_EQ((long double)0.125, lexical_cast<long double>("0.125"));
    EXPECT_DOUBLE_EQ(123.0, lexical_cast<double>("+123"));
}

TEST(core, lexical_cast_floating_point_rejects_invalid_values)
{
    EXPECT_THROW(lexical_cast<float>(""), std::invalid_argument);
    EXPECT_THROW(lexical_cast<float>(" 1.25"), std::invalid_argument);
    EXPECT_THROW(lexical_cast<float>("1.25 "), std::invalid_argument);
    EXPECT_THROW(lexical_cast<double>("1.2.3"), std::invalid_argument);
    EXPECT_THROW(lexical_cast<double>("abc"), std::invalid_argument);
}

TEST(core, dlink)
{
    dlink links[10];
    dlink hdr;

    for (int i = 0; i < 10; i++)
        links[i].insert_before(&hdr);

    int count = 0;
    dlink* p = hdr.next();
    while (p != &hdr)
    {
        count++;
        p = p->next();
    }

    EXPECT_EQ(count, 10);

    p = hdr.next();
    while (p != &hdr)
    {
        auto p1 = p;
        p = p->next();
        p1->remove();
        count--;
    }

    EXPECT_TRUE(hdr.is_alone());
    EXPECT_TRUE(count == 0);
}



class foo : public ::dsn::ref_counter
{
public:
    foo(int &count)
        : _count(count)
    {
        _count++;
    }

    ~foo()
    {
        _count--;
    }

private:
    int &_count;
};

typedef ::dsn::ref_ptr<foo> foo_ptr;

TEST(core, ref_ptr)
{
    int count = 0;
    foo_ptr x = nullptr;
    auto y = new foo(count);
    x = y;
    EXPECT_TRUE(x->get_count() == 1);
    EXPECT_TRUE(count == 1);
    x = new foo(count);
    EXPECT_TRUE(x->get_count() == 1);
    EXPECT_TRUE(count == 1);
    x = nullptr;
    EXPECT_TRUE(count == 0);

    std::map<int, foo_ptr> xs;
    x = new foo(count);
    EXPECT_TRUE(x->get_count() == 1);
    EXPECT_TRUE(count == 1);
    xs.insert(std::make_pair(1, x));
    EXPECT_TRUE(x->get_count() == 2);
    EXPECT_TRUE(count == 1);
    x = nullptr;
    EXPECT_TRUE(count == 1);
    xs.clear();
    EXPECT_TRUE(count == 0);

    x = new foo(count);
    EXPECT_TRUE(count == 1);
    xs[2] = x;
    EXPECT_TRUE(x->get_count() == 2);
    x = nullptr;
    EXPECT_TRUE(count == 1);
    xs.clear();
    EXPECT_TRUE(count == 0);

    y = new foo(count);
    EXPECT_TRUE(count == 1);
    xs.insert(std::make_pair(1, y));
    EXPECT_TRUE(count == 1);
    EXPECT_TRUE(y->get_count() == 1);
    xs.clear();
    EXPECT_TRUE(count == 0);

    y = new foo(count);
    EXPECT_TRUE(count == 1);
    xs[2] = y;
    EXPECT_TRUE(count == 1);
    EXPECT_TRUE(y->get_count() == 1);
    xs.clear();
    EXPECT_TRUE(count == 0);


    foo_ptr z = new foo(count);
    EXPECT_TRUE(count == 1);
    z = foo_ptr();
    EXPECT_TRUE(count == 0);
}
