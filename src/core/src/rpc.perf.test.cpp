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
 * Description:
 *     Rpc performance test
 *
 * Revision history:
 *     2016-01-05, Tianyi Wang, first version
 */
#include <gtest/gtest.h>
#include <dsn/cpp/test_utils.h>
#include <dsn/service_api_cpp.h>
#include <boost/lexical_cast.hpp>


void rpc_testcase(uint64_t block_size, size_t concurrency)
{
    std::atomic<uint64_t> io_count(0);
    std::atomic<uint64_t> cb_flying_count(0);
    volatile bool exit = false;
    std::function<void(int)> cb;    
    std::string req;
    req.resize(block_size, 'x');
    rpc_address server("localhost", 20101);

    std::string test_server = dsn_config_get_value_string("apps.client", "test_server", "", 
        "rpc test server address, i.e., host:port"
        );
    if (test_server.length() > 0)
    {
        url_host_address addr(test_server.c_str());
        server.assign_ipv4(addr.ip(), addr.port());
    }

    cb = [&](int index)
    {
        if (!exit)
        {
            io_count++;            
            cb_flying_count++;

            rpc::call(
                server,
                RPC_TEST_HASH,
                req,
                nullptr,
                [idx = index, &cb, &cb_flying_count](error_code err, std::string&& result)
                {
                    if (ERR_OK == err)
                        cb(idx);
                    cb_flying_count--;
                }
            );
        }
    };

    // start
    auto tic = std::chrono::steady_clock::now();
    for (int i = 0; i < concurrency; i++)
    {
        cb(i);
    }

    // run for seconds
    std::this_thread::sleep_for(std::chrono::seconds(10));
    auto ioc = io_count.load();
    auto bytes = ioc * block_size;
    auto toc = std::chrono::steady_clock::now();

    std::cout
        << "block_size = " << block_size
        << ", concurrency = " << concurrency
        << ", iops = " << (double)ioc / (double)std::chrono::duration_cast<std::chrono::microseconds>(toc - tic).count() * 1000000.0 << " #/s"
        << ", throughput = " << (double)bytes / std::chrono::duration_cast<std::chrono::microseconds>(toc - tic).count() << " mB/s"
        << ", avg_latency = " << (double)std::chrono::duration_cast<std::chrono::microseconds>(toc - tic).count() / (double)(ioc / concurrency) << " us"
        << std::endl;

    // safe exit
    exit = true;

    while (cb_flying_count.load() > 0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

TEST(perf_core, rpc)
{
    for (auto blk_size_bytes : { 1, 128, 256, 4 * 1024 })
        for (auto concurrency : { 1, 2, 4,10,50,100,200 })
            rpc_testcase(blk_size_bytes, concurrency);
}


void lpc_testcase(size_t concurrency)
{
    std::atomic<uint64_t> io_count(0);
    std::atomic<uint64_t> cb_flying_count(0);
    volatile bool exit = false;
    std::function<void(int)> cb;

    cb = [&](int index)
    {
        if (!exit)
        {
            io_count++;
            cb_flying_count++;

            tasking::enqueue(
                LPC_TEST_HASH,
                nullptr,
                [idx = index, &cb, &cb_flying_count]()
                {
                    cb(idx);
                    cb_flying_count--;
                }
            );
        }
    };

    // start
    auto tic = std::chrono::steady_clock::now();
    for (int i = 0; i < concurrency; i++)
    {
        cb(i);
    }

    // run for seconds
    std::this_thread::sleep_for(std::chrono::seconds(10));
    auto ioc = io_count.load();
    auto toc = std::chrono::steady_clock::now();

    std::cout
        << "concurrency = " << concurrency
        << ", iops = " << (double)ioc / (double)std::chrono::duration_cast<std::chrono::microseconds>(toc - tic).count() * 1000000.0 << " #/s"
        << ", avg_latency = " << (double)std::chrono::duration_cast<std::chrono::microseconds>(toc - tic).count() / (double)(ioc / concurrency) << " us"
        << std::endl;

    // safe exit
    exit = true;

    while (cb_flying_count.load() > 0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

TEST(perf_core, lpc)
{
    for (auto concurrency : { 1, 2, 4,10,50,100,200 })
        lpc_testcase(concurrency);
}
