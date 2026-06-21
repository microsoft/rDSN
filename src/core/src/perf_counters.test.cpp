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
 *     Unit-test for perf_counters.
 *
 * Revision history:
 *     Nov., 2015, @qinzuoyan (Zuoyan Qin), first version
 *     xxxx-xx-xx, author, fix bug about xxx
 */

# include <dsn/cpp/perf_counter_.h>
# include <dsn/tool-api/perf_counter.h>
# include <gtest/gtest.h>

using namespace ::dsn;

TEST(core, perf_counters)
{
    perf_counter_ptr p;

    p = perf_counter::get_counter("app", "test", "number_counter", COUNTER_TYPE_NUMBER,"", false);
    ASSERT_EQ(nullptr, p);
    p = perf_counter::get_counter("app", "test", "number_counter", COUNTER_TYPE_NUMBER, "", true);
    ASSERT_NE(nullptr, p);
    p = perf_counter::get_counter("app", "test", "number_counter", COUNTER_TYPE_NUMBER, "", false);
    ASSERT_NE(nullptr, p);

    p = perf_counter::get_counter("app", "test", "rate_counter", COUNTER_TYPE_RATE, "", false);
    ASSERT_EQ(nullptr, p);
    p = perf_counter::get_counter("app", "test", "rate_counter", COUNTER_TYPE_RATE, "", true);
    ASSERT_NE(nullptr, p);
    p = perf_counter::get_counter("app", "test", "rate_counter", COUNTER_TYPE_RATE, "", false);
    ASSERT_NE(nullptr, p);

    ASSERT_FALSE(perf_counter::remove_counter("number_counter"));
    ASSERT_FALSE(perf_counter::remove_counter("unexist_counter"));

    ASSERT_TRUE(perf_counter::remove_counter("app*test*number_counter"));
    p = perf_counter::get_counter("app", "test", "number_counter", COUNTER_TYPE_NUMBER,"", false);
    ASSERT_EQ(nullptr, p);

    ASSERT_TRUE(perf_counter::remove_counter("app*test*rate_counter"));
    p = perf_counter::get_counter("app", "test", "rate_counter", COUNTER_TYPE_RATE, "", false);
    ASSERT_EQ(nullptr, p);

    p = perf_counter::get_counter("app", "test", "unexist_counter", COUNTER_TYPE_NUMBER, "", false);
    ASSERT_EQ(nullptr, p);
    ASSERT_FALSE(perf_counter::remove_counter("app*test*unexist_counter"));
}

TEST(core, dsn_perf_counter_invalid_parameters)
{
    ASSERT_EQ(nullptr, dsn_perf_counter_create(nullptr, "name", COUNTER_TYPE_NUMBER, ""));
    ASSERT_EQ(nullptr, dsn_perf_counter_create("", "name", COUNTER_TYPE_NUMBER, ""));
    ASSERT_EQ(nullptr, dsn_perf_counter_create("section", nullptr, COUNTER_TYPE_NUMBER, ""));
    ASSERT_EQ(nullptr, dsn_perf_counter_create("section", "", COUNTER_TYPE_NUMBER, ""));
    ASSERT_EQ(nullptr,
              dsn_perf_counter_create("section", "name", COUNTER_TYPE_INVALID, ""));
    ASSERT_EQ(nullptr,
              dsn_perf_counter_create("section", "name", COUNTER_TYPE_NUMBER, nullptr));

    dsn_perf_counter_remove(nullptr);
    dsn_perf_counter_increment(nullptr);
    dsn_perf_counter_decrement(nullptr);
    dsn_perf_counter_add(nullptr, 1);
    dsn_perf_counter_set(nullptr, 1);
    ASSERT_DOUBLE_EQ(-1.0, dsn_perf_counter_get_value(nullptr));
    ASSERT_EQ(static_cast<uint64_t>(-1), dsn_perf_counter_get_integer_value(nullptr));
    ASSERT_DOUBLE_EQ(-1.0, dsn_perf_counter_get_percentile(nullptr, COUNTER_PERCENTILE_50));
    ASSERT_DOUBLE_EQ(-1.0,
                     dsn_perf_counter_get_percentile(reinterpret_cast<dsn_handle_t>(1),
                                                     COUNTER_PERCENTILE_INVALID));
}

TEST(core, perf_counter_cpp_invalid_parameters)
{
    perf_counter_ counter;
    counter.increment();
    counter.decrement();
    counter.add(1);
    counter.set(1);
    ASSERT_DOUBLE_EQ(-1.0, counter.get_value());
    ASSERT_EQ(static_cast<uint64_t>(-1), counter.get_integer_value());
    ASSERT_DOUBLE_EQ(-1.0, counter.get_percentile(COUNTER_PERCENTILE_50));

    counter.init(nullptr, "name", COUNTER_TYPE_NUMBER, "");
    ASSERT_DOUBLE_EQ(-1.0, counter.get_value());

    counter.init("section", nullptr, COUNTER_TYPE_NUMBER, "");
    ASSERT_DOUBLE_EQ(-1.0, counter.get_value());

    counter.destroy();
}
