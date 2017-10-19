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
# include "mutation_log.h"
# include <gtest/gtest.h>
# include <cstdio>

using namespace ::dsn;
using namespace ::dsn::replication;

static void copy_file(const char* from_file, const char* to_file, int64_t to_size = -1)
{
    int64_t from_size;
    ASSERT_TRUE(dsn::utils::filesystem::file_size(from_file, from_size));
    ASSERT_LE(to_size, from_size);
    FILE* from = fopen(from_file, "rb");
    ASSERT_TRUE(from != nullptr);
    FILE* to = fopen(to_file, "wb");
    ASSERT_TRUE(to != nullptr);
    if (to_size == -1)
        to_size = from_size;
    if (to_size > 0)
    {
        std::unique_ptr<char[]> buf(new char[to_size]);
        auto n = fread(buf.get(), 1, to_size, from);
        ASSERT_EQ(to_size, n);
        n = fwrite(buf.get(), 1, to_size, to);
        ASSERT_EQ(to_size, n);
    }
    int r = fclose(from);
    ASSERT_EQ(0, r);
    r = fclose(to);
    ASSERT_EQ(0, r);
}

static void overwrite_file(const char* file, int offset, const void* buf, int size)
{
    FILE* f = fopen(file, "r+b");
    ASSERT_TRUE(f != nullptr);
    int r = fseek(f, offset, SEEK_SET);
    ASSERT_EQ(0, r);
    size_t n = fwrite(buf, 1, size, f);
    ASSERT_EQ(size, n);
    r = fclose(f);
    ASSERT_EQ(0, r);
}

TEST(replication, log_file)
{
    replica_log_info_map mdecrees;
    gpid gpid(1, 0);

    mdecrees[gpid] = replica_log_info(3, 0);
    std::string fpath = "./log.1.100";
    int index = 1;
    int64_t offset = 100;
    std::string str = "hello, world!";
    error_code err;
    log_file_ptr lf = nullptr;

    // write log
    ASSERT_TRUE(!dsn::utils::filesystem::file_exists(fpath));
    lf = log_file::create_write(
        ".",
        index,
        offset
        );
    ASSERT_TRUE(lf != nullptr);
    ASSERT_EQ(fpath, lf->path());
    ASSERT_EQ(index, lf->index());
    ASSERT_EQ(offset, lf->start_offset());
    ASSERT_EQ(offset, lf->end_offset());
    for (int i = 0; i < 100; i++)
    {
        auto writer = lf->prepare_log_block();

        if (i == 0)
        {
            binary_writer temp_writer;
            lf->write_file_header(
                temp_writer,
                mdecrees
                );
            writer->add(temp_writer.get_buffer());
            ASSERT_EQ(mdecrees, lf->previous_log_max_decrees());
            log_file_header& h = lf->header();
            ASSERT_EQ(100, h.start_global_offset);
        }

        binary_writer temp_writer;
        temp_writer.write(str);
        writer->add(temp_writer.get_buffer());

        task_ptr task = lf->commit_log_block(
            *writer, offset, LPC_AIO_IMMEDIATE_CALLBACK, nullptr, nullptr, 0
            );
        task->wait();
        ASSERT_EQ(ERR_OK, task->error());
        ASSERT_EQ(writer->size(), task->io_size());

        lf->flush();
        offset += writer->size();

        delete writer;
    }
    lf->close();
    lf = nullptr;
    ASSERT_TRUE(dsn::utils::filesystem::file_exists(fpath));

    // file already exist
    offset = 100;
    lf = log_file::create_write(
        ".",
        index,
        offset
        );
    ASSERT_TRUE(lf == nullptr);

    // invalid file name
    lf = log_file::open_read("", err);
    ASSERT_TRUE(lf == nullptr);
    ASSERT_EQ(ERR_INVALID_PARAMETERS, err);
    lf = log_file::open_read("a", err);
    ASSERT_TRUE(lf == nullptr);
    ASSERT_EQ(ERR_INVALID_PARAMETERS, err);
    lf = log_file::open_read("aaaaa", err);
    ASSERT_TRUE(lf == nullptr);
    ASSERT_EQ(ERR_INVALID_PARAMETERS, err);
    lf = log_file::open_read("log.1.2.aaa", err);
    ASSERT_TRUE(lf == nullptr);
    ASSERT_EQ(ERR_INVALID_PARAMETERS, err);
    lf = log_file::open_read("log.1.2.removed", err);
    ASSERT_TRUE(lf == nullptr);
    ASSERT_EQ(ERR_INVALID_PARAMETERS, err);
    lf = log_file::open_read("log.1", err);
    ASSERT_TRUE(lf == nullptr);
    ASSERT_EQ(ERR_INVALID_PARAMETERS, err);
    lf = log_file::open_read("log.1.", err);
    ASSERT_TRUE(lf == nullptr);
    ASSERT_EQ(ERR_INVALID_PARAMETERS, err);
    lf = log_file::open_read("log..2", err);
    ASSERT_TRUE(lf == nullptr);
    ASSERT_EQ(ERR_INVALID_PARAMETERS, err);
    lf = log_file::open_read("log.1a.2", err);
    ASSERT_TRUE(lf == nullptr);
    ASSERT_EQ(ERR_INVALID_PARAMETERS, err);
    lf = log_file::open_read("log.1.2a", err);
    ASSERT_TRUE(lf == nullptr);
    ASSERT_EQ(ERR_INVALID_PARAMETERS, err);

    // file not exist
    lf = log_file::open_read("log.0.0", err);
    ASSERT_TRUE(lf == nullptr);
    ASSERT_EQ(ERR_FILE_OPERATION_FAILED, err);

    // bad file data: empty file
    ASSERT_TRUE(!dsn::utils::filesystem::file_exists("log.1.0"));
    copy_file(fpath.c_str(), "log.1.0", 0);
    ASSERT_TRUE(dsn::utils::filesystem::file_exists("log.1.0"));
    lf = log_file::open_read("log.1.0", err);
    ASSERT_TRUE(lf == nullptr);
    ASSERT_EQ(ERR_HANDLE_EOF, err);
    ASSERT_TRUE(!dsn::utils::filesystem::file_exists("log.1.0"));
    ASSERT_TRUE(dsn::utils::filesystem::file_exists("log.1.0.removed"));

    // bad file data: incomplete log_block_header
    ASSERT_TRUE(!dsn::utils::filesystem::file_exists("log.1.1"));
    copy_file(fpath.c_str(), "log.1.1", sizeof(log_block_header) - 1);
    ASSERT_TRUE(dsn::utils::filesystem::file_exists("log.1.1"));
    lf = log_file::open_read("log.1.1", err);
    ASSERT_TRUE(lf == nullptr);
    ASSERT_EQ(ERR_INCOMPLETE_DATA, err);
    ASSERT_TRUE(!dsn::utils::filesystem::file_exists("log.1.1"));
    ASSERT_TRUE(dsn::utils::filesystem::file_exists("log.1.1.removed"));

    // bad file data: bad log_block_header (magic = 0xfeadbeef)
    ASSERT_TRUE(!dsn::utils::filesystem::file_exists("log.1.2"));
    copy_file(fpath.c_str(), "log.1.2");
    int32_t bad_magic = 0xfeadbeef;
    overwrite_file("log.1.2", FIELD_OFFSET(log_block_header, magic), &bad_magic, sizeof(bad_magic));
    ASSERT_TRUE(dsn::utils::filesystem::file_exists("log.1.2"));
    lf = log_file::open_read("log.1.2", err);
    ASSERT_TRUE(lf == nullptr);
    ASSERT_EQ(ERR_INVALID_DATA, err);
    ASSERT_TRUE(!dsn::utils::filesystem::file_exists("log.1.2"));
    ASSERT_TRUE(dsn::utils::filesystem::file_exists("log.1.2.removed"));

    // bad file data: bad log_block_header (crc check failed)
    ASSERT_TRUE(!dsn::utils::filesystem::file_exists("log.1.3"));
    copy_file(fpath.c_str(), "log.1.3");
    int32_t bad_crc = 0;
    overwrite_file("log.1.3", FIELD_OFFSET(log_block_header, body_crc), &bad_crc, sizeof(bad_crc));
    ASSERT_TRUE(dsn::utils::filesystem::file_exists("log.1.3"));
    lf = log_file::open_read("log.1.3", err);
    ASSERT_TRUE(lf == nullptr);
    ASSERT_EQ(ERR_INVALID_DATA, err);
    ASSERT_TRUE(!dsn::utils::filesystem::file_exists("log.1.3"));
    ASSERT_TRUE(dsn::utils::filesystem::file_exists("log.1.3.removed"));

    // bad file data: incomplete block body
    ASSERT_TRUE(!dsn::utils::filesystem::file_exists("log.1.4"));
    copy_file(fpath.c_str(), "log.1.4", sizeof(log_block_header) + 1);
    ASSERT_TRUE(dsn::utils::filesystem::file_exists("log.1.4"));
    lf = log_file::open_read("log.1.4", err);
    ASSERT_TRUE(lf == nullptr);
    ASSERT_EQ(ERR_INCOMPLETE_DATA, err);
    ASSERT_TRUE(!dsn::utils::filesystem::file_exists("log.1.4"));
    ASSERT_TRUE(dsn::utils::filesystem::file_exists("log.1.4.removed"));
    ASSERT_TRUE(dsn::utils::filesystem::rename_path("log.1.4.removed", "log.1.4"));

    // read the file for test
    offset = 100;
    lf = log_file::open_read(fpath.c_str(), err);
    ASSERT_NE(nullptr, lf);
    EXPECT_EQ(ERR_OK, err);
    ASSERT_EQ(1, lf->index());
    ASSERT_EQ(100, lf->start_offset());
    int64_t sz;
    ASSERT_TRUE(dsn::utils::filesystem::file_size(fpath, sz));
    ASSERT_EQ(lf->start_offset() + sz, lf->end_offset());

    // read data
    lf->reset_stream();
    for (int i = 0; i < 100; i++)
    {
        blob bb;
        auto err = lf->read_next_log_block(bb);
        ASSERT_EQ(ERR_OK, err);

        binary_reader reader(bb);

        if (i == 0)
        {
            lf->read_file_header(reader);
            ASSERT_TRUE(lf->is_right_header());
            ASSERT_EQ(100, lf->header().start_global_offset);
        }

        std::string ss;
        reader.read(ss);
        ASSERT_TRUE(ss == str);

        offset += bb.length() + sizeof(log_block_header);
    }

    ASSERT_TRUE(offset == lf->end_offset());

    blob bb;
    err = lf->read_next_log_block(bb);
    ASSERT_TRUE(err == ERR_HANDLE_EOF);

    lf = nullptr;

    utils::filesystem::remove_path(fpath);
}

TEST(replication, mutation_log)
{
    gpid gpid(1, 0);
    std::string str = "hello, world!";
    std::string logp = "./test-log";
    std::vector<mutation_ptr> mutations;

    // prepare
    utils::filesystem::remove_path(logp);
    utils::filesystem::create_directory(logp);

    // writing logs
    mutation_log_ptr mlog = new mutation_log_private(
        logp,
        4,
        gpid,
        nullptr,
        1024,
        512
        );

    auto err = mlog->open(nullptr, nullptr);
    EXPECT_EQ(err, ERR_OK);

    for (int i = 0; i < 1000; i++)
    {
        mutation_ptr mu(new mutation());
        mu->data.header.ballot = 1;
        mu->data.header.decree = 2 + i;
        mu->data.header.pid = gpid;
        mu->data.header.last_committed_decree = i;
        mu->data.header.log_offset = 0;

        binary_writer writer;
        for (int j = 0; j < 100; j++)
        {
            writer.write(str);
        }
        mu->data.updates.push_back(mutation_update());
        mu->data.updates.back().code = RPC_REPLICATION_WRITE_EMPTY;
        mu->data.updates.back().data = writer.get_buffer();

        mu->client_requests.push_back(nullptr);

        mutations.push_back(mu);

        mlog->append(mu, LPC_AIO_IMMEDIATE_CALLBACK, nullptr, nullptr, 0);
    }

    mlog->close();

    // reading logs
    mlog = new mutation_log_private(
        logp,
        4,
        gpid,
        nullptr,
        1024,
        512
        );

    int mutation_index = -1;
    mlog->open(
        [&mutations, &mutation_index](mutation_ptr& mu)->bool
    {
        mutation_ptr wmu = mutations[++mutation_index];
#ifdef DSN_USE_THRIFT_SERIALIZATION
        EXPECT_TRUE(wmu->data.header == mu->data.header);
#else
        EXPECT_TRUE(memcmp((const void*)&wmu->data.header,
            (const void*)&mu->data.header,
            sizeof(mu->data.header)) == 0
            );
#endif
        EXPECT_TRUE(wmu->data.updates.size() == mu->data.updates.size());
        EXPECT_TRUE(wmu->data.updates[0].data.length() == mu->data.updates[0].data.length());
        EXPECT_TRUE(memcmp((const void*)wmu->data.updates[0].data.data(),
            (const void*)mu->data.updates[0].data.data(),
            mu->data.updates[0].data.length()) == 0
            );
        EXPECT_TRUE(wmu->data.updates[0].code == mu->data.updates[0].code);
        EXPECT_TRUE(wmu->client_requests.size() == mu->client_requests.size());
        return true;
    }, nullptr
    );
    EXPECT_TRUE(mutation_index + 1 == (int)mutations.size());
    mlog->close();

    // clear all
    utils::filesystem::remove_path(logp);
}

mutation_ptr generate_mutation(uint64_t block_size)
{
    gpid gd;
    gd.set_app_id(1);
    gd.set_partition_index(1);

    // prepare mutation
    mutation_ptr mu(new mutation());
    mu->data.header.ballot = 1;
    mu->data.header.decree = 0xdeadbeef;
    mu->data.header.pid = gd;
    mu->data.header.last_committed_decree = 0;
    mu->data.header.log_offset = 0;
    binary_writer writer;
    std::unique_ptr<char[]> buffer(new char[block_size]);
    writer.write((const char*)buffer.get(), block_size);
    mu->data.updates.push_back(mutation_update());
    mu->data.updates.back().code = RPC_REPLICATION_WRITE_EMPTY;
    mu->data.updates.back().data = writer.get_buffer();
    mu->client_requests.push_back(nullptr);

    return mu;
}

void mutation_log_testcase(uint64_t block_size, size_t concurrency, bool is_write, bool force_flush, bool shared)
{
    gpid gd;
    gd.set_app_id(1);
    gd.set_partition_index(1);

    // prepare logs
    std::vector<mutation_log_ptr> logs;
    logs.resize(concurrency);
    auto log_count = concurrency;
    if (shared)
    {
        log_count = 1;
    }

    if (is_write)
    {
        for (int i = 0; i < log_count; i++)
        {
            std::stringstream ss;
            ss << "temp." << i;
            auto path = ss.str();
            if (utils::filesystem::directory_exists(path))
                utils::filesystem::remove_path(path);
            
            mutation_log_ptr mlog = new mutation_log_shared(
                path,
                4,
                force_flush
                );

            auto err = mlog->open(nullptr, nullptr);
            EXPECT_EQ(err, ERR_OK);
            mlog->on_partition_reset(gd, 1);
            logs[i] = mlog;
        }
    }
    else
    {
        for (int i = 0; i < log_count; i++)
        {
            std::stringstream ss;
            ss << "temp." << i;
            auto path = ss.str();

            mutation_log_ptr mlog = new mutation_log_shared(
                path,
                4,
                force_flush
                );
            mlog->on_partition_reset(gd, 1);
            logs[i] = mlog;
        }
    }

    if (log_count < concurrency)
    {
        dassert (1 == log_count, "");

        for (int i = 1; i < concurrency; i++)
            logs[i] = logs[0];
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
            cb_flying_count++;
            if (is_write)
            {
                io_count++;
                auto mu = generate_mutation(block_size);
                logs[index]->append(mu, LPC_AIO_IMMEDIATE_CALLBACK, nullptr, 
                    [idx = index, &cb, &cb_flying_count, mu2 = mu](::dsn::error_code code, size_t sz)
                    {
                        if (ERR_OK == code)
                            cb(idx);
                        cb_flying_count--;
                    }, 
                    0);
            }
            else
            {
                if (index == 0 || logs[index] != logs[0])
                {
                    logs[index]->open(
                        [&](mutation_ptr& lmu)->bool
                    {
                        EXPECT_TRUE(lmu->data.header.decree == 0xdeadbeef);
                        return true;
                    }, nullptr
                    );

                    io_count += logs[index]->size();
                }
                cb_flying_count--;
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

    if (is_write)
    {
        // run for seconds
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
    
    auto ioc = io_count.load();
    auto bytes = ioc * block_size;
    if (!is_write) bytes = ioc;
    auto toc = std::chrono::steady_clock::now();

    if (is_write)
    {
        std::cout << "(W)"
            << "block_size = " << block_size
            << ", shared = " << shared
            << ", concurrency = " << concurrency
            << ", iops = " << (double)ioc / (double)std::chrono::duration_cast<std::chrono::microseconds>(toc - tic).count() * 1000000.0 << " #/s"
            << ", throughput = " << (double)bytes / std::chrono::duration_cast<std::chrono::microseconds>(toc - tic).count() << " mB/s"
            << ", avg_latency = " << (double)std::chrono::duration_cast<std::chrono::microseconds>(toc - tic).count() / (double)(ioc / concurrency) << " us"
            << ", flush = " << force_flush
            << std::endl;
    }
    else
    {
        std::cout << "(R)"
            << "block_size = " << block_size
            << ", shared = " << shared
            << ", concurrency = " << concurrency
            << ", throughput = " << (double)bytes / std::chrono::duration_cast<std::chrono::microseconds>(toc - tic).count() << " mB/s"
            << std::endl;
    }

    // safe exit
    exit = true;

    while (cb_flying_count.load() > 0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    for (int i = 0; i < log_count; i++)
    {
        logs[i]->close();
    }
}

TEST(perf_replication, mutation_log)
{
    for (auto force_flush : { false, true })
        for (auto is_write : { true, false })
            for (auto blk_size_bytes : { 100, 256, 1024, 4 * 1024})
                    for (auto concurrency : { 1, 4, 128 })
                        for (auto shared : { false, true})
                            mutation_log_testcase(blk_size_bytes, concurrency, is_write, force_flush, shared);
}

