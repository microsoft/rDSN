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

# include <dsn/utility/customizable_id.h>
# include <gtest/gtest.h>
# include <atomic>
# include <string>
# include <thread>
# include <vector>

namespace dsn {
namespace {

struct customized_id_test_tag
{
};

TEST(core, customized_id_registration_is_concurrent_and_pointer_stable)
{
    utils::customized_id_mgr<customized_id_test_tag> manager;
    const int anchor_id = manager.register_id("customized_id.anchor");
    ASSERT_GE(anchor_id, 0);

    const char *const anchor_name = manager.get_name(anchor_id);
    ASSERT_NE(nullptr, anchor_name);

    for (int i = 0; i < 2048; ++i)
    {
        ASSERT_GE(manager.register_id(("customized_id.sequential." + std::to_string(i)).c_str()),
                  0);
    }

    EXPECT_EQ(anchor_name, manager.get_name(anchor_id));
    EXPECT_STREQ("customized_id.anchor", anchor_name);

    constexpr int thread_count = 8;
    constexpr int registrations_per_thread = 128;
    std::atomic<bool> start(false);
    std::atomic<bool> failed(false);
    std::vector<int> shared_ids(thread_count, -1);
    std::vector<std::thread> threads;

    for (int thread_index = 0; thread_index < thread_count; ++thread_index)
    {
        threads.emplace_back([&, thread_index]() {
            while (!start.load(std::memory_order_acquire))
            {
                std::this_thread::yield();
            }

            shared_ids[thread_index] = manager.register_id("customized_id.concurrent.shared");
            for (int i = 0; i < registrations_per_thread; ++i)
            {
                const std::string name = "customized_id.concurrent." +
                                         std::to_string(thread_index) + "." + std::to_string(i);
                const int id = manager.register_id(name.c_str());
                const char *registered_name = manager.get_name(id);
                if (id < 0 || registered_name == nullptr || name != registered_name)
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
    for (int i = 1; i < thread_count; ++i)
    {
        EXPECT_EQ(shared_ids[0], shared_ids[i]);
    }
    EXPECT_STREQ("customized_id.anchor", anchor_name);
}

} // anonymous namespace
} // namespace dsn
