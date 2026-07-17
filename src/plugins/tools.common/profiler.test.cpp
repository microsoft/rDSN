#include "profiler.h"

#include <atomic>
#include <memory>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

namespace dsn {
namespace tools {
namespace {

TEST(tools_common, profiler_task_state_transitions)
{
    profiler_detail::task_state state(0);

    EXPECT_TRUE(state.mark_in_queue());
    EXPECT_FALSE(state.mark_in_queue());
    EXPECT_TRUE(state.begin_execution());
    EXPECT_FALSE(state.begin_execution());
    EXPECT_FALSE(state.cancel());

    profiler_detail::task_state cancelled(0);
    EXPECT_TRUE(cancelled.mark_in_queue());
    EXPECT_TRUE(cancelled.cancel());
    EXPECT_FALSE(cancelled.begin_execution());
    EXPECT_FALSE(cancelled.cancel());
}

TEST(tools_common, profiler_missing_task_state_is_ignored)
{
    uint64_t timestamp = 123;
    EXPECT_FALSE(profiler_detail::try_get_timestamp(nullptr, timestamp));
    EXPECT_EQ(123u, timestamp);

    profiler_detail::set_timestamp(nullptr, 456);
    EXPECT_FALSE(profiler_detail::mark_in_queue(nullptr));
    EXPECT_FALSE(profiler_detail::begin_execution(nullptr));
    EXPECT_FALSE(profiler_detail::cancel(nullptr));
}

TEST(tools_common, profiler_task_state_enqueue_cancel_race)
{
    constexpr size_t state_count = 10000;
    std::vector<std::unique_ptr<profiler_detail::task_state>> states;
    std::vector<unsigned char> marked(state_count);
    std::vector<unsigned char> cancelled(state_count);
    states.reserve(state_count);
    for (size_t i = 0; i < state_count; ++i) {
        states.emplace_back(new profiler_detail::task_state(0));
    }

    std::atomic<bool> start(false);
    std::thread enqueue_thread([&]() {
        while (!start.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        for (size_t i = 0; i < state_count; ++i) {
            marked[i] = states[i]->mark_in_queue();
        }
    });
    std::thread cancel_thread([&]() {
        while (!start.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        for (size_t i = 0; i < state_count; ++i) {
            cancelled[i] = states[i]->cancel();
        }
    });

    start.store(true, std::memory_order_release);
    enqueue_thread.join();
    cancel_thread.join();

    for (size_t i = 0; i < state_count; ++i) {
        EXPECT_EQ(marked[i], cancelled[i]);
        EXPECT_FALSE(states[i]->begin_execution());
    }
}

} // anonymous namespace
} // namespace tools
} // namespace dsn
