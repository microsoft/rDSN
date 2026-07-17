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

# include <dsn/utility/singleton_vector_store.h>
# include <gtest/gtest.h>
# include <atomic>
# include <thread>
# include <vector>

namespace dsn {
namespace {

TEST(core, singleton_vector_store_supports_concurrent_growth)
{
    utils::singleton_vector_store<long, -1> store;
    constexpr int thread_count = 8;
    constexpr int values_per_thread = 256;
    std::atomic<bool> start(false);
    std::atomic<bool> failed(false);
    std::vector<std::thread> threads;

    for (int thread_index = 0; thread_index < thread_count; ++thread_index)
    {
        threads.emplace_back([&, thread_index]() {
            while (!start.load(std::memory_order_acquire))
            {
                std::this_thread::yield();
            }

            for (int i = 0; i < values_per_thread; ++i)
            {
                const int index = thread_index * values_per_thread + i;
                const long value = static_cast<long>(index) + 1000;
                if (!store.put(index, value) || !store.contains(index) ||
                    store.get(index) != value)
                {
                    failed.store(true, std::memory_order_relaxed);
                    return;
                }
            }
        });
    }

    start.store(true, std::memory_order_release);
    for (auto &thread : threads)
    {
        thread.join();
    }

    EXPECT_FALSE(failed.load(std::memory_order_relaxed));
    for (int i = 0; i < thread_count * values_per_thread; ++i)
    {
        EXPECT_TRUE(store.contains(i));
        EXPECT_EQ(static_cast<long>(i) + 1000, store.get(i));
    }
}

} // anonymous namespace
} // namespace dsn
