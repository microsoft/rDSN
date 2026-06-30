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


# include <dsn/service_api_cpp.h>
# include <dsn/cpp/utils.h>
# include <gtest/gtest.h>
# include <dsn/cpp/test_utils.h>

using namespace ::dsn;

TEST(core, aio)
{
    // if in dsn_mimic_app() and disk_io_mode == IOE_PER_QUEUE
    if (task::get_current_disk() == nullptr) return;

    ASSERT_TRUE(::dsn::utils::test::prepare_test_tmp_dir("dsn.core.aio"));
    const std::string tmp_file = ::dsn::utils::test::test_tmp_path("dsn.core.aio", "tmp");
    const char* buffer = "hello, world";
    int len = (int)strlen(buffer);

    // write
    auto fp = dsn_file_open(tmp_file.c_str(), O_RDWR | O_CREAT | O_BINARY, 0666);
    
    std::list<task_ptr> tasks;
    uint64_t offset = 0;

    // new write
    for (int i = 0; i < 100; i++)
    {
        auto t = ::dsn::file::write(fp, buffer, len, offset, LPC_AIO_TEST, nullptr, dsn::empty_callback);
        tasks.push_back(t);
        offset += len;
    }

    for (auto& t : tasks)
    {
        t->wait();
    }

    // overwrite 
    offset = 0;
    tasks.clear();
    for (int i = 0; i < 100; i++)
    {
        auto t = ::dsn::file::write(fp, buffer, len, offset, LPC_AIO_TEST, nullptr, dsn::empty_callback);
        tasks.push_back(t);
        offset += len;
    }

    for (auto& t : tasks)
    {
        t->wait();
        EXPECT_TRUE(t->io_size() == (size_t)len);
    }

    // vector write
    tasks.clear();
    std::unique_ptr<dsn_file_buffer_t[]> buffers(new dsn_file_buffer_t[100]);
    for (int i = 0; i < 10; i ++)
    {
        buffers[i].buffer = reinterpret_cast<void*>(const_cast<char*>(buffer));
        buffers[i].size = len;
    }
    for (int i = 0; i < 10; i ++)
    {
        tasks.push_back(::dsn::file::write_vector(fp, buffers.get(), 10, offset, LPC_AIO_TEST, nullptr, dsn::empty_callback));
        offset += 10 * len;
    }
    for (auto& t : tasks)
    {
        t->wait();
        EXPECT_TRUE(t->io_size() == 10 * len);
    }
    auto err = dsn_file_close(fp);
    EXPECT_TRUE(err == ERR_OK);

    // read
    char* buffer2 = (char*)alloca((size_t)len);
    fp = dsn_file_open(tmp_file.c_str(), O_RDONLY | O_BINARY, 0);

    // concurrent read
    offset = 0;
    tasks.clear();
    for (int i = 0; i < 100; i++)
    {
        auto t = ::dsn::file::read(fp, buffer2, len, offset, LPC_AIO_TEST, nullptr, dsn::empty_callback);
        tasks.push_back(t);
        offset += len;
    }

    for (auto& t : tasks)
    {
        t->wait();
        EXPECT_TRUE(t->io_size() == (size_t)len);
    }

    // sequential read
    offset = 0;
    tasks.clear();
    for (int i = 0; i < 200; i++)
    {
        buffer2[0] = 'x';
        auto t = ::dsn::file::read(fp, buffer2, len, offset, LPC_AIO_TEST, nullptr, dsn::empty_callback);
        offset += len;

        t->wait();
        EXPECT_TRUE(t->io_size() == (size_t)len);
        EXPECT_TRUE(memcmp(buffer, buffer2, len) == 0);
    }

    err = dsn_file_close(fp);
    EXPECT_TRUE(err == ERR_OK);

    utils::filesystem::remove_path(tmp_file);
}

TEST(core, aio_share)
{
    // if in dsn_mimic_app() and disk_io_mode == IOE_PER_QUEUE
    if (task::get_current_disk() == nullptr) return;

    ASSERT_TRUE(::dsn::utils::test::prepare_test_tmp_dir("dsn.core.aio_share"));
    const std::string tmp_file = ::dsn::utils::test::test_tmp_path("dsn.core.aio_share", "tmp");
    auto fp = dsn_file_open(tmp_file.c_str(), O_WRONLY | O_CREAT | O_BINARY, 0666);
    EXPECT_TRUE(fp != nullptr);

    auto fp2 = dsn_file_open(tmp_file.c_str(), O_RDONLY | O_BINARY, 0);
    EXPECT_TRUE(fp2 != nullptr);

    dsn_file_close(fp);
    dsn_file_close(fp2);

    utils::filesystem::remove_path(tmp_file);
}

TEST(core, aio_cpp_invalid_parameters)
{
    auto task = ::dsn::file::create_aio_task(TASK_CODE_INVALID, nullptr, dsn::empty_callback, 0);
    EXPECT_EQ(nullptr, task.get());

    task = ::dsn::file::create_aio_task(
        TASK_CODE_INVALID, nullptr, [](::dsn::error_code, size_t) {}, 0);
    EXPECT_EQ(nullptr, task.get());

    char buffer[1] = {};
    task = ::dsn::file::read(
        nullptr, buffer, sizeof(buffer), 0, TASK_CODE_INVALID, nullptr, dsn::empty_callback);
    EXPECT_EQ(nullptr, task.get());

    task = ::dsn::file::write(
        nullptr, buffer, sizeof(buffer), 0, TASK_CODE_INVALID, nullptr, dsn::empty_callback);
    EXPECT_EQ(nullptr, task.get());

    dsn_file_buffer_t file_buffer{buffer, sizeof(buffer)};
    task = ::dsn::file::write_vector(
        nullptr, &file_buffer, 1, 0, TASK_CODE_INVALID, nullptr, dsn::empty_callback);
    EXPECT_EQ(nullptr, task.get());

    task = ::dsn::file::copy_remote_files(rpc_address(),
                                          "",
                                          {},
                                          "",
                                          false,
                                          TASK_CODE_INVALID,
                                          nullptr,
                                          dsn::empty_callback);
    EXPECT_EQ(nullptr, task.get());

    task = ::dsn::file::copy_remote_directory(
        rpc_address(), "", "", false, TASK_CODE_INVALID, nullptr, dsn::empty_callback);
    EXPECT_EQ(nullptr, task.get());
}

TEST(core, operation_failed)
{
    // if in dsn_mimic_app() and disk_io_mode == IOE_PER_QUEUE
    if (task::get_current_disk() == nullptr) return;

    ASSERT_TRUE(::dsn::utils::test::prepare_test_tmp_dir("dsn.core.aio_operation_failed"));
    const std::string tmp_file =
        ::dsn::utils::test::test_tmp_path("dsn.core.aio_operation_failed", "tmp_test_file");
    auto fp = dsn_file_open(tmp_file.c_str(), O_WRONLY, 0600);
    EXPECT_TRUE(fp == nullptr);

    ::dsn::error_code* err = new ::dsn::error_code;
    size_t* count = new size_t;
    auto io_callback = [err, count](::dsn::error_code e, size_t n) {
        *err = e;
        *count = n;
    };

    fp = dsn_file_open(tmp_file.c_str(), O_WRONLY|O_CREAT|O_BINARY, 0666);
    EXPECT_TRUE(fp != nullptr);
    char buffer[512];
    const char* str = "hello file";
    auto t = ::dsn::file::write(fp, str, strlen(str), 0, LPC_AIO_TEST, nullptr, io_callback, 0);
    t->wait();
    EXPECT_TRUE(*err == ERR_OK && *count==strlen(str));

    t = ::dsn::file::read(fp, buffer, 512, 0, LPC_AIO_TEST, nullptr, io_callback, 0);
    t->wait();
    EXPECT_TRUE(*err == ERR_FILE_OPERATION_FAILED);

    auto fp2 = dsn_file_open(tmp_file.c_str(), O_RDONLY|O_BINARY, 0);
    EXPECT_TRUE(fp2 != nullptr);

    t = ::dsn::file::read(fp2, buffer, 512, 0, LPC_AIO_TEST, nullptr, io_callback, 0);
    t->wait();
    EXPECT_TRUE(*err == ERR_OK && *count==strlen(str));

    t = ::dsn::file::read(fp2, buffer, 5, 0, LPC_AIO_TEST, nullptr, io_callback, 0);
    t->wait();
    EXPECT_TRUE(*err == ERR_OK && *count==5);

    t = ::dsn::file::read(fp2, buffer, 512, 100, LPC_AIO_TEST, nullptr, io_callback, 0);
    t->wait();
    ddebug("error code: %s", err->to_string());
    dsn_file_close(fp);
    dsn_file_close(fp2);

    EXPECT_TRUE(utils::filesystem::remove_path(tmp_file));
}

// Pins the zero-length aio contract that the NFS file-copy plugin depends on:
// a zero-length write/read completes with ERR_HANDLE_EOF (not ERR_OK) and count 0.
// Because of this, nfs_service_impl::on_copy / nfs_client_impl::continue_write must
// special-case empty (size==0) files instead of issuing a zero-length read/write,
// which would otherwise be reported to the copy caller as a failure.
TEST(core, aio_zero_length)
{
    // if in dsn_mimic_app() and disk_io_mode == IOE_PER_QUEUE
    if (task::get_current_disk() == nullptr) return;

    ASSERT_TRUE(::dsn::utils::test::prepare_test_tmp_dir("dsn.core.aio_zero"));
    const std::string tmp_file =
        ::dsn::utils::test::test_tmp_path("dsn.core.aio_zero", "zero_test_file");

    ::dsn::error_code werr;
    size_t wcount = 0xdead;
    ::dsn::error_code rerr;
    size_t rcount = 0xdead;
    char buffer[8] = {};

    // zero-length write to a freshly created file
    auto fp = dsn_file_open(tmp_file.c_str(), O_RDWR | O_CREAT | O_BINARY, 0666);
    ASSERT_TRUE(fp != nullptr);
    auto t = ::dsn::file::write(fp, buffer, 0, 0, LPC_AIO_TEST, nullptr,
        [&werr, &wcount](::dsn::error_code e, size_t n) { werr = e; wcount = n; }, 0);
    t->wait();
    EXPECT_TRUE(werr == ERR_HANDLE_EOF);
    EXPECT_EQ(0u, wcount);
    dsn_file_close(fp);

    // zero-length read from the (now empty) file
    auto fp2 = dsn_file_open(tmp_file.c_str(), O_RDONLY | O_BINARY, 0);
    ASSERT_TRUE(fp2 != nullptr);
    t = ::dsn::file::read(fp2, buffer, 0, 0, LPC_AIO_TEST, nullptr,
        [&rerr, &rcount](::dsn::error_code e, size_t n) { rerr = e; rcount = n; }, 0);
    t->wait();
    EXPECT_TRUE(rerr == ERR_HANDLE_EOF);
    EXPECT_EQ(0u, rcount);
    dsn_file_close(fp2);

    EXPECT_TRUE(utils::filesystem::remove_path(tmp_file));
}
