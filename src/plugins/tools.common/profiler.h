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
 *     profiler toollet definition
 *
 * Revision history:
 *     Mar., 2015, @imzhenyu (Zhenyu Guo), first version
 *     xxxx-xx-xx, author, fix bug about xxx
 */

#pragma once

#include <atomic>
#include <dsn/tool_api.h>

/*!
@defgroup profiler Profiler
@ingroup tools

Profiler toollet

This toollet collects many performance counter values for the specified tasks,
as configed below.

<PRE>

[core]

toollets = profiler

[task..default]
is_profile = true

[task.RPC_PING]
is_profile = false

</PRE>
*/
 
namespace dsn {
    namespace tools {

        namespace profiler_detail {

            enum class queue_state
            {
                not_in_queue,
                in_queue,
                cancelled
            };

            class task_state
            {
            public:
                explicit task_state(uint64_t timestamp)
                    : _timestamp(timestamp), _queue_state(queue_state::not_in_queue)
                {
                }

                uint64_t timestamp() const
                {
                    return _timestamp.load(std::memory_order_relaxed);
                }

                void set_timestamp(uint64_t timestamp)
                {
                    _timestamp.store(timestamp, std::memory_order_relaxed);
                }

                bool mark_in_queue()
                {
                    auto expected = queue_state::not_in_queue;
                    return _queue_state.compare_exchange_strong(
                        expected, queue_state::in_queue, std::memory_order_relaxed);
                }

                bool begin_execution()
                {
                    auto expected = queue_state::in_queue;
                    return _queue_state.compare_exchange_strong(
                        expected, queue_state::not_in_queue, std::memory_order_relaxed);
                }

                bool cancel()
                {
                    return _queue_state.exchange(queue_state::cancelled,
                                                 std::memory_order_relaxed) ==
                           queue_state::in_queue;
                }

            private:
                std::atomic<uint64_t> _timestamp;
                std::atomic<queue_state> _queue_state;
            };

            inline bool try_get_timestamp(const task_state* state, uint64_t& timestamp)
            {
                if (state == nullptr)
                {
                    return false;
                }

                timestamp = state->timestamp();
                return true;
            }

            inline void set_timestamp(task_state* state, uint64_t timestamp)
            {
                if (state != nullptr)
                {
                    state->set_timestamp(timestamp);
                }
            }

            inline bool mark_in_queue(task_state* state)
            {
                return state != nullptr && state->mark_in_queue();
            }

            inline bool begin_execution(task_state* state)
            {
                return state != nullptr && state->begin_execution();
            }

            inline bool cancel(task_state* state)
            {
                return state != nullptr && state->cancel();
            }

        } // namespace profiler_detail

        class profiler : public toollet
        {
        public:
            profiler(const char* name);
            virtual void install(service_spec& spec);
        };
    }
}
