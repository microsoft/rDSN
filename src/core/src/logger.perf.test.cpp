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


# include <gtest/gtest.h>
# include <dsn/utility/factory_store.h>
# include <dsn/tool_api.h>
# include <dsn/cpp/test_utils.h>

using namespace ::dsn;
const char str[64] = "this is a logging test for log %010d @ thread %010d";

void logv(dsn::logging_provider* logger,const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    logger->dsn_logv(
        __FILE__,
        __FUNCTION__,
        __LINE__,
        LOG_LEVEL_DEBUG,
        "log.test",
        str,
        ap
        );
    va_end(ap);
}

void logger_test(logging_provider::factory f, int thread_count, int record_count)
{
    std::list<std::thread*> threads;
    logging_provider* logger = f("./");

    uint64_t nts = dsn_now_ns();
    uint64_t nts_start = nts;

    
    for (int i = 0; i < thread_count; ++i)
    {
        threads.push_back(new std::thread([&,i]
        {
            task::set_tls_dsn_context(task::get_current_node2(), nullptr, nullptr);
            for (int j = 0; j < record_count; j++)
            {
                logv(logger, str, j, i);
            }
            
        }));
    }
    
    for (auto& thr : threads)
    {
        thr->join();
        delete thr;
    }
    threads.clear(); 

    nts = dsn_now_ns();
    
    //one sample log
    size_t size_per_log = strlen(
        "13:11:02.678 (1446037862678885017 1d50) unknown.io-thrd.07504: this is a logging test for log 0000000000 @ thread 000000000"
        )+1;

    std::cout 
        << thread_count << "\t\t\t "
        << record_count << "\t\t\t " 
        << static_cast<double>(thread_count * record_count) 
            * size_per_log / (1024*1024) / (nts - nts_start) * 1000000000
        << "MB/s" 
        << std::endl;

    //logger.flush();
    delete logger;
}

TEST(perf_core, logger_test)
{
    auto fs = dsn::utils::factory_store<logging_provider>::get_all_factories<logging_provider::factory>();
    for (auto& f : fs)
    {
        if (f.type == ::dsn::provider_type::PROVIDER_TYPE_MAIN)
        {
            dwarn("test %s ...", f.name.c_str());
            
            std::cout << "thread_count\t\t record_count\t\t speed" << std::endl;

            auto threads_count = { 1, 2,  5, 10 };
            for (int i : threads_count)
            {
                logger_test(f.factory, i, 100000);
            }
        }
    }
}
