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
 *     Disk IO performance test
 *
 * Revision history:
 *     2016-01-05, Tianyi Wang, first version
 */

#include <cinttypes>
#include <gtest/gtest.h>
#include <dsn/service_api_cpp.h>
#include <dsn/cpp/test_utils.h>
#include <boost/lexical_cast.hpp>

void aio_testcase(uint64_t block_size, size_t concurrency, bool is_write, bool shared)
{
    std::unique_ptr<char[]> buffer(new char[block_size]);
    std::vector<dsn_handle_t> files;
    files.resize(concurrency);

    int flag;
    if (is_write)
    {
        flag = O_CREAT | O_RDWR;
        if (shared)
        {
            if (utils::filesystem::file_exists("temp"))
                utils::filesystem::remove_path("temp");
        }
        else
        {
            for (int i = 0; i < concurrency; i++)
            {
                std::stringstream ss;
                ss << "temp." << i;
                auto file = ss.str();
                if (utils::filesystem::file_exists(file))
                    utils::filesystem::remove_path(file);
            }
        }
    }
    else
    {
        flag = O_RDWR;
    }

    if (shared)
    {
        auto file_handle = dsn_file_open("temp", flag, 0666);
        EXPECT_TRUE(file_handle != nullptr);
        for (int i = 0; i < concurrency; i++)
            files[i] = file_handle;
    }
    else
    {
        for (int i = 0; i < concurrency; i++)
        {
            std::stringstream ss;
            ss << "temp." << i;
            auto file = ss.str();
            auto file_handle = dsn_file_open(file.c_str(), flag, 0666);
            EXPECT_TRUE(file_handle != nullptr);
            files[i] = file_handle;
        }
    }
    
    std::atomic<uint64_t> io_count(0);
    std::atomic<uint64_t> cb_flying_count(0);
    volatile bool exit = false;
    std::function<void(int)> cb;
    std::vector<uint64_t> offsets;
    offsets.resize(concurrency);
    
    cb = [&](int index)
    {
        if (!exit)
        {
            auto ioc = io_count++;
            uint64_t offset;
            if (!shared)
            {
                offset = offsets[index];
                offsets[index] += block_size;
            }
            else
            {
                offset = ioc * block_size;
            }

            cb_flying_count++;
            if (is_write)
            {
                file::write(files[index], buffer.get(), (int)block_size, offset,
                    LPC_AIO_TEST, nullptr, [idx = index, &cb, &cb_flying_count](::dsn::error_code code, size_t sz) 
                    {                        
                        if (ERR_OK == code)
                            cb(idx);
                        cb_flying_count--;
                    });                
            }
            else
            {
                file::read(files[index], buffer.get(), (int)block_size, offset,
                    LPC_AIO_TEST, nullptr, [idx = index, &cb, &cb_flying_count](::dsn::error_code code, size_t sz)
                    {                        
                        if (ERR_OK == code)
                            cb(idx);
                        cb_flying_count--;
                    });
            }
        }
    };

    // start
    auto tic = std::chrono::steady_clock::now();
    for (int i = 0; i < concurrency; i++)
    {
        offsets[i] = 0;
        cb(i);
    }

    // run for seconds
    std::this_thread::sleep_for(std::chrono::seconds(10));
    auto ioc = io_count.load();
    auto bytes = ioc * block_size;    
    auto toc = std::chrono::steady_clock::now();
    
    std::cout << "is_write = " << is_write        
        << ", block_size = " << block_size
        << ", shared = " << shared
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

    if (shared)
    {
        dsn_file_flush(files[0]);
        auto cok = dsn_file_close(files[0]);
        EXPECT_EQ(cok, ERR_OK);
    }
    else
    {
        for (auto& f : files)
        {
            dsn_file_flush(f);
            auto cok = dsn_file_close(f);
            EXPECT_EQ(cok, ERR_OK);
        }
    }
}

TEST(perf_core, aio)
{
    for (auto is_write : { true, false })
        for (auto shared : { false, true })
            for (auto blk_size_bytes : { 256, 1024, 4 * 1024 })
                for (auto concurrency : { 1, 2, 4})
                    aio_testcase(blk_size_bytes, concurrency, is_write, shared);
}
