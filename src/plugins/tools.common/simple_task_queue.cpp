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

# include "simple_task_queue.h"
# include <cstdio>
# include <memory>
# include <thread>

# ifdef __TITLE__
# undef __TITLE__
# endif
# define __TITLE__ "task.queue.simple"

namespace dsn 
{
    namespace tools
    {
        simple_timer_service::simple_timer_service(service_node* node, timer_service* inner_provider)
            : timer_service(node, inner_provider)
        {
            _worker = nullptr;
        }

        void simple_timer_service::start(io_modifer& ctx)
        {
            _worker = std::shared_ptr<std::thread>(new std::thread([this, ctx]()
            {
                task::set_tls_dsn_context(node(), nullptr, ctx.queue);

                char buffer[128];
                int name_len = snprintf(buffer, sizeof(buffer), "%s.%s.timer",
                    get_service_node_name(node()),
                    ctx.queue ? ctx.queue->get_name().c_str():""
                    );
                if (name_len < 0 || static_cast<size_t>(name_len) >= sizeof(buffer))
                {
                    derror("timer worker name is too long");
                }

                task_worker::set_name(buffer);
                task_worker::set_priority(worker_priority_t::THREAD_xPRIORITY_ABOVE_NORMAL);

                boost::asio::io_service::work work(_ios);
                _ios.run();
            }));
        }

        void simple_timer_service::add_timer(task* task)
        {
            int delay_ms = task->delay_milliseconds();
            std::shared_ptr<boost::asio::deadline_timer> timer(new boost::asio::deadline_timer(_ios));
            timer->expires_from_now(boost::posix_time::milliseconds(delay_ms));
            task->set_delay(0);

            timer->async_wait([task, timer, delay_ms](const boost::system::error_code& ec)
            {
                if (!ec)
                {
                    task->enqueue();
                }
                else if (ec != ::boost::asio::error::operation_aborted)
                {
                    // A timer failure used to abort the whole process (dfatal), which is too harsh
                    // for a single timer. But we must not run the task early either: its delay has
                    // not actually elapsed. Restore the delay and re-enqueue -- task::enqueue routes
                    // delayed tasks back to add_timer, re-arming a fresh timer -- so the task still
                    // fires after ~its intended delay instead of immediately, and is not lost.
                    derror("timer failed for task %s, err = %u, rescheduling after %d ms",
                        task->spec().name.c_str(), ec.value(), delay_ms);
                    task->set_delay(delay_ms);
                    task->enqueue();
                }

                // to consume the added ref count by task::enqueue for add_timer
                task->release_ref();
            });
        }

        simple_task_queue::simple_task_queue(task_worker_pool* pool, int index, task_queue* inner_provider)
            : task_queue(pool, index, inner_provider), _samples("")
        {
        }

        void simple_task_queue::enqueue(task* task)
        {
            _samples.enqueue(task, task->spec().priority);
        }

        // always return 1 or 0 task so far
        task* simple_task_queue::dequeue(/*inout*/int& batch_size)
        {
            long c = 0;
            auto t = _samples.dequeue(c, TIME_MS_MAX);
            dassert(t != nullptr, "dequeue does not return empty tasks");
            batch_size = 1;
            return t;
        }
    }
}
