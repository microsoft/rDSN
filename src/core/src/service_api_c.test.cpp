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
 *     Unit-test for c service api.
 *
 * Revision history:
 *     Nov., 2015, @qinzuoyan (Zuoyan Qin), first version
 *     xxxx-xx-xx, author, fix bug about xxx
 */

# include <dsn/service_api_cpp.h>
# include <dsn/cpp/utils.h>
# include <dsn/tool_api.h>
# include <dsn/utility/configuration.h>
# include <gtest/gtest.h>
# include <climits>
# include <string>
# include <thread>
# include <cstring>
# include <chrono>
# include "service_engine.h"
# include <dsn/cpp/test_utils.h>

using namespace dsn;

TEST(core, dsn_error)
{
    ASSERT_EQ(ERR_OK, dsn_error_register("ERR_OK"));
    ASSERT_STREQ("ERR_OK", dsn_error_to_string(ERR_OK));
}

TEST(core, dsn_error_invalid_parameters)
{
    const auto too_long_error_name = std::string(DSN_MAX_ERROR_CODE_NAME_LENGTH, 'e');

    ASSERT_EQ(ERR_INVALID_PARAMETERS, dsn_error_register(nullptr));
    ASSERT_EQ(ERR_INVALID_PARAMETERS, dsn_error_register(""));
    ASSERT_EQ(ERR_INVALID_PARAMETERS, dsn_error_register(too_long_error_name.c_str()));
    ASSERT_STREQ("unknown", dsn_error_to_string(-1));
    ASSERT_EQ(ERR_INVALID_PARAMETERS, dsn_error_from_string(nullptr, ERR_OK));
    ASSERT_EQ(ERR_INVALID_PARAMETERS, dsn_error_from_string("", ERR_OK));
}

DEFINE_THREAD_POOL_CODE(THREAD_POOL_FOR_TEST)
TEST(core, dsn_threadpool_code)
{
    ASSERT_EQ(THREAD_POOL_INVALID, dsn_threadpool_code_from_string("THREAD_POOL_NOT_EXIST", THREAD_POOL_INVALID));

    ASSERT_STREQ("THREAD_POOL_DEFAULT", dsn_threadpool_code_to_string(THREAD_POOL_DEFAULT));
    ASSERT_EQ(THREAD_POOL_DEFAULT, dsn_threadpool_code_from_string("THREAD_POOL_DEFAULT", THREAD_POOL_INVALID));
    ASSERT_LE(THREAD_POOL_DEFAULT, dsn_threadpool_code_max());

    ASSERT_STREQ("THREAD_POOL_FOR_TEST", dsn_threadpool_code_to_string(THREAD_POOL_FOR_TEST));
    ASSERT_EQ(THREAD_POOL_FOR_TEST, dsn_threadpool_code_from_string("THREAD_POOL_FOR_TEST", THREAD_POOL_INVALID));
    ASSERT_LE(THREAD_POOL_FOR_TEST, dsn_threadpool_code_max());

    ASSERT_LT(0, dsn_threadpool_get_current_tid());
}

TEST(core, dsn_threadpool_code_invalid_parameters)
{
    ASSERT_EQ(THREAD_POOL_INVALID, dsn_threadpool_code_register(nullptr));
    ASSERT_EQ(THREAD_POOL_INVALID, dsn_threadpool_code_register(""));
    ASSERT_EQ(THREAD_POOL_INVALID,
              dsn_threadpool_code_from_string(nullptr, THREAD_POOL_DEFAULT));
    ASSERT_EQ(THREAD_POOL_INVALID,
              dsn_threadpool_code_from_string("", THREAD_POOL_DEFAULT));
}

DEFINE_TASK_CODE(TASK_CODE_COMPUTE_FOR_TEST, TASK_PRIORITY_HIGH, THREAD_POOL_DEFAULT)
DEFINE_TASK_CODE_AIO(TASK_CODE_AIO_FOR_TEST, TASK_PRIORITY_COMMON, THREAD_POOL_DEFAULT)
DEFINE_TASK_CODE_RPC(TASK_CODE_RPC_FOR_TEST, TASK_PRIORITY_LOW, THREAD_POOL_DEFAULT)

namespace
{

void noop_task_handler(void*) {}

void noop_rpc_response_handler(dsn_error_t, dsn_message_t, dsn_message_t, void*) {}

void noop_rpc_request_handler(dsn_message_t, void*) {}

void noop_aio_handler(dsn_error_t, size_t, void*) {}

void* noop_checker_create(const char*, dsn_app_info*, int) { return nullptr; }

void noop_checker_apply(void*) {}

} // anonymous namespace

TEST(core, dsn_task_code)
{
    dsn_task_type_t type;
    dsn_task_priority_t pri;
    dsn_threadpool_code_t pool;

    ASSERT_EQ(TASK_CODE_INVALID, dsn_task_code_from_string("TASK_CODE_NOT_EXIST", TASK_CODE_INVALID));

    ASSERT_STREQ("TASK_TYPE_COMPUTE", dsn_task_type_to_string(TASK_TYPE_COMPUTE));

    ASSERT_STREQ("TASK_PRIORITY_HIGH", dsn_task_priority_to_string(TASK_PRIORITY_HIGH));

    ASSERT_STREQ("TASK_CODE_COMPUTE_FOR_TEST", dsn_task_code_to_string(TASK_CODE_COMPUTE_FOR_TEST));
    ASSERT_EQ(TASK_CODE_COMPUTE_FOR_TEST, dsn_task_code_from_string("TASK_CODE_COMPUTE_FOR_TEST", TASK_CODE_INVALID));
    ASSERT_LE(TASK_CODE_COMPUTE_FOR_TEST, dsn_task_code_max());
    dsn_task_code_query(TASK_CODE_COMPUTE_FOR_TEST, &type, &pri, &pool);
    ASSERT_EQ(TASK_TYPE_COMPUTE, type);
    ASSERT_EQ(TASK_PRIORITY_HIGH, pri);
    ASSERT_EQ(THREAD_POOL_DEFAULT, pool);

    ASSERT_STREQ("TASK_CODE_AIO_FOR_TEST", dsn_task_code_to_string(TASK_CODE_AIO_FOR_TEST));
    ASSERT_EQ(TASK_CODE_AIO_FOR_TEST, dsn_task_code_from_string("TASK_CODE_AIO_FOR_TEST", TASK_CODE_INVALID));
    ASSERT_LE(TASK_CODE_AIO_FOR_TEST, dsn_task_code_max());
    dsn_task_code_query(TASK_CODE_AIO_FOR_TEST, &type, &pri, &pool);
    ASSERT_EQ(TASK_TYPE_AIO, type);
    ASSERT_EQ(TASK_PRIORITY_COMMON, pri);
    ASSERT_EQ(THREAD_POOL_DEFAULT, pool);

    ASSERT_STREQ("TASK_CODE_RPC_FOR_TEST", dsn_task_code_to_string(TASK_CODE_RPC_FOR_TEST));
    ASSERT_EQ(TASK_CODE_RPC_FOR_TEST, dsn_task_code_from_string("TASK_CODE_RPC_FOR_TEST", TASK_CODE_INVALID));
    ASSERT_LE(TASK_CODE_RPC_FOR_TEST, dsn_task_code_max());
    dsn_task_code_query(TASK_CODE_RPC_FOR_TEST, &type, &pri, &pool);
    ASSERT_EQ(TASK_TYPE_RPC_REQUEST, type);
    ASSERT_EQ(TASK_PRIORITY_LOW, pri);
    ASSERT_EQ(THREAD_POOL_DEFAULT, pool);

    ASSERT_STREQ("TASK_CODE_RPC_FOR_TEST_ACK", dsn_task_code_to_string(TASK_CODE_RPC_FOR_TEST_ACK));
    ASSERT_EQ(TASK_CODE_RPC_FOR_TEST_ACK, dsn_task_code_from_string("TASK_CODE_RPC_FOR_TEST_ACK", TASK_CODE_INVALID));
    ASSERT_LE(TASK_CODE_RPC_FOR_TEST_ACK, dsn_task_code_max());
    dsn_task_code_query(TASK_CODE_RPC_FOR_TEST_ACK, &type, &pri, &pool);
    ASSERT_EQ(TASK_TYPE_RPC_RESPONSE, type);
    ASSERT_EQ(TASK_PRIORITY_LOW, pri);
    ASSERT_EQ(THREAD_POOL_DEFAULT, pool);

    dsn_task_code_set_threadpool(TASK_CODE_COMPUTE_FOR_TEST, THREAD_POOL_FOR_TEST);
    dsn_task_code_set_priority(TASK_CODE_COMPUTE_FOR_TEST, TASK_PRIORITY_COMMON);
    dsn_task_code_query(TASK_CODE_COMPUTE_FOR_TEST, &type, &pri, &pool);
    ASSERT_EQ(TASK_TYPE_COMPUTE, type);
    ASSERT_EQ(TASK_PRIORITY_COMMON, pri);
    ASSERT_EQ(THREAD_POOL_FOR_TEST, pool);

    dsn_task_code_set_threadpool(TASK_CODE_COMPUTE_FOR_TEST, THREAD_POOL_DEFAULT);
    dsn_task_code_set_priority(TASK_CODE_COMPUTE_FOR_TEST, TASK_PRIORITY_HIGH);
}

TEST(core, dsn_task_code_invalid_parameters)
{
    const auto too_long_task_name = std::string(DSN_MAX_TASK_CODE_NAME_LENGTH, 't');
    dsn_task_type_t type = TASK_TYPE_COUNT;
    dsn_task_priority_t priority = TASK_PRIORITY_COUNT;
    dsn_threadpool_code_t pool = THREAD_POOL_INVALID;

    ASSERT_EQ(TASK_CODE_INVALID, dsn_task_code_register(nullptr,
                                                        TASK_TYPE_COMPUTE,
                                                        TASK_PRIORITY_COMMON,
                                                        THREAD_POOL_DEFAULT));
    ASSERT_EQ(TASK_CODE_INVALID, dsn_task_code_register("",
                                                        TASK_TYPE_COMPUTE,
                                                        TASK_PRIORITY_COMMON,
                                                        THREAD_POOL_DEFAULT));
    ASSERT_EQ(TASK_CODE_INVALID, dsn_task_code_register(too_long_task_name.c_str(),
                                                        TASK_TYPE_COMPUTE,
                                                        TASK_PRIORITY_COMMON,
                                                        THREAD_POOL_DEFAULT));
    ASSERT_EQ(TASK_CODE_INVALID,
              dsn_task_code_register("invalid_task_type",
                                     TASK_TYPE_COUNT,
                                     TASK_PRIORITY_COMMON,
                                     THREAD_POOL_DEFAULT));
    ASSERT_EQ(TASK_CODE_INVALID,
              dsn_task_code_register("invalid_task_priority",
                                     TASK_TYPE_COMPUTE,
                                     TASK_PRIORITY_COUNT,
                                     THREAD_POOL_DEFAULT));
    ASSERT_EQ(TASK_CODE_INVALID,
              dsn_task_code_register("invalid_task_pool",
                                     TASK_TYPE_COMPUTE,
                                     TASK_PRIORITY_COMMON,
                                     THREAD_POOL_INVALID));
    ASSERT_EQ(TASK_CODE_INVALID,
              dsn_task_code_register("negative_task_pool",
                                     TASK_TYPE_COMPUTE,
                                     TASK_PRIORITY_COMMON,
                                     static_cast<dsn_threadpool_code_t>(-1)));

    dsn_task_code_query(TASK_CODE_INVALID, &type, &priority, &pool);
    ASSERT_EQ(TASK_TYPE_COUNT, type);
    ASSERT_EQ(TASK_PRIORITY_COUNT, priority);
    ASSERT_EQ(THREAD_POOL_INVALID, pool);
    dsn_task_code_set_threadpool(TASK_CODE_INVALID, THREAD_POOL_DEFAULT);
    dsn_task_code_set_threadpool(TASK_CODE_COMPUTE_FOR_TEST, THREAD_POOL_INVALID);
    dsn_task_code_set_threadpool(TASK_CODE_COMPUTE_FOR_TEST,
                                 static_cast<dsn_threadpool_code_t>(-1));
    dsn_task_code_set_priority(TASK_CODE_INVALID, TASK_PRIORITY_COMMON);
    dsn_task_code_set_priority(TASK_CODE_COMPUTE_FOR_TEST, TASK_PRIORITY_COUNT);

    ASSERT_STREQ("TASK_CODE_INVALID", dsn_task_code_to_string(TASK_CODE_INVALID));
    ASSERT_STREQ("unknown", dsn_task_code_to_string(static_cast<dsn_task_code_t>(-1)));
    ASSERT_EQ(TASK_CODE_INVALID, dsn_task_code_from_string(nullptr, TASK_CODE_COMPUTE_FOR_TEST));
    ASSERT_EQ(TASK_CODE_INVALID, dsn_task_code_from_string("", TASK_CODE_COMPUTE_FOR_TEST));
    ASSERT_STREQ("Unknown", dsn_task_type_to_string(TASK_TYPE_COUNT));
    ASSERT_STREQ("Unknown", dsn_task_priority_to_string(TASK_PRIORITY_COUNT));
    ASSERT_EQ(nullptr, dsn_task_queue_virtual_length_ptr(TASK_CODE_INVALID, 0));
}

TEST(core, dsn_config)
{
    ASSERT_TRUE(dsn_config_get_value_bool("apps.client", "run", false, "client run"));
    ASSERT_EQ(1u, dsn_config_get_value_uint64("apps.client", "count", 100, "client count"));
    ASSERT_EQ(1.0, dsn_config_get_value_double("apps.client", "count", 100.0, "client count"));
    ASSERT_EQ(1.0, dsn_config_get_value_double("apps.client", "count", 100.0, "client count"));
    const char* buffers[100];
    int buffer_count = 100;
    ASSERT_EQ(2, dsn_config_get_all_keys("core.test", buffers, &buffer_count));
    ASSERT_EQ(2, buffer_count);
    ASSERT_STREQ("count", buffers[0]);
    ASSERT_STREQ("run", buffers[1]);
    buffer_count = 1;
    ASSERT_EQ(2, dsn_config_get_all_keys("core.test", buffers, &buffer_count));
    ASSERT_EQ(1, buffer_count);
    ASSERT_STREQ("count", buffers[0]);
}

TEST(core, dsn_config_get_all_keys_reports_total_count)
{
    const std::string section = "core.test.large_key_count";
    for (int i = 0; i < 140; ++i)
    {
        const std::string key = "key." + std::to_string(i);
        get_main_config()->set(section.c_str(), key.c_str(), "value", "");
    }

    const char* buffers[128];
    int buffer_count = 128;
    ASSERT_EQ(140, dsn_config_get_all_keys(section.c_str(), buffers, &buffer_count));
    ASSERT_EQ(128, buffer_count);

    buffer_count = 0;
    ASSERT_EQ(140, dsn_config_get_all_keys(section.c_str(), nullptr, &buffer_count));
    ASSERT_EQ(0, buffer_count);
}

TEST(core, dsn_config_invalid_numeric_values)
{
    scoped_test_stderr stderr_capture;
    get_main_config()->set("core.invalid_config", "bad_uint64", "12bad", "");
    get_main_config()->set("core.invalid_config", "overflow_uint64",
                           "999999999999999999999999999999999999", "");
    get_main_config()->set("core.invalid_config", "invalid_hex_uint64", "0xzz", "");

    ASSERT_EQ(321u, dsn_config_get_value_uint64("core.invalid_config", "bad_uint64", 321, ""));
    ASSERT_EQ(322u, dsn_config_get_value_uint64("core.invalid_config", "overflow_uint64", 322, ""));
    ASSERT_EQ(323u,
              dsn_config_get_value_uint64("core.invalid_config", "invalid_hex_uint64", 323, ""));
}

TEST(core, dsn_config_invalid_parameters)
{
    const char* buffers[1];
    int buffer_count = 1;
    int invalid_buffer_count = -1;

    ASSERT_STREQ("default", dsn_config_get_value_string(nullptr, "key", "default", ""));
    ASSERT_STREQ("default", dsn_config_get_value_string("", "key", "default", ""));
    ASSERT_STREQ("default", dsn_config_get_value_string("section", nullptr, "default", ""));
    ASSERT_STREQ("default", dsn_config_get_value_string("section", "", "default", ""));
    ASSERT_EQ(nullptr, dsn_config_get_value_string("section", "key", nullptr, ""));

    ASSERT_TRUE(dsn_config_get_value_bool(nullptr, "key", true, ""));
    ASSERT_TRUE(dsn_config_get_value_bool("", "key", true, ""));
    ASSERT_TRUE(dsn_config_get_value_bool("section", nullptr, true, ""));
    ASSERT_TRUE(dsn_config_get_value_bool("section", "", true, ""));

    ASSERT_EQ(123u, dsn_config_get_value_uint64(nullptr, "key", 123, ""));
    ASSERT_EQ(123u, dsn_config_get_value_uint64("", "key", 123, ""));
    ASSERT_EQ(123u, dsn_config_get_value_uint64("section", nullptr, 123, ""));
    ASSERT_EQ(123u, dsn_config_get_value_uint64("section", "", 123, ""));

    ASSERT_EQ(1.5, dsn_config_get_value_double(nullptr, "key", 1.5, ""));
    ASSERT_EQ(1.5, dsn_config_get_value_double("", "key", 1.5, ""));
    ASSERT_EQ(1.5, dsn_config_get_value_double("section", nullptr, 1.5, ""));
    ASSERT_EQ(1.5, dsn_config_get_value_double("section", "", 1.5, ""));

    ASSERT_EQ(-1, dsn_config_get_all_sections(buffers, nullptr));
    ASSERT_EQ(-1, dsn_config_get_all_sections(buffers, &invalid_buffer_count));
    ASSERT_EQ(-1, dsn_config_get_all_sections(nullptr, &buffer_count));

    ASSERT_EQ(-1, dsn_config_get_all_keys(nullptr, buffers, &buffer_count));
    ASSERT_EQ(-1, dsn_config_get_all_keys("", buffers, &buffer_count));
    ASSERT_EQ(-1, dsn_config_get_all_keys("section", buffers, nullptr));
    invalid_buffer_count = -1;
    ASSERT_EQ(-1, dsn_config_get_all_keys("section", buffers, &invalid_buffer_count));
    buffer_count = 1;
    ASSERT_EQ(-1, dsn_config_get_all_keys("section", nullptr, &buffer_count));

    dsn_config_dump(nullptr);
    dsn_config_dump("");
}

TEST(core, dsn_run_invalid_parameters)
{
    char arg0[] = "dsn";
    char* null_argv[] = {arg0, nullptr};

    dsn_run(-1, nullptr, false);
    dsn_run(1, nullptr, false);
    dsn_run(2, null_argv, false);
    ASSERT_FALSE(dsn_run_config(nullptr, false));
    ASSERT_FALSE(dsn_run_config("", false));
}

TEST(core, dsn_coredump)
{
}

TEST(core, dsn_crc32)
{
}

TEST(core, dsn_crc_invalid_parameters)
{
    ASSERT_EQ(CRC_INVALID, dsn_crc32_compute(nullptr, 1, 0));
    ASSERT_EQ(static_cast<uint64_t>(CRC_INVALID), dsn_crc64_compute(nullptr, 1, 0));
}

TEST(core, dsn_task)
{
}

TEST(core, dsn_task_invalid_parameters)
{
    ASSERT_FALSE(dsn_task_is_running_inside(nullptr));
    ASSERT_EQ(0, dsn_task_get_ref(nullptr));
    ASSERT_EQ(ERR_INVALID_PARAMETERS, dsn_task_error(nullptr));
    ASSERT_FALSE(dsn_task_call(nullptr, 0));
    ASSERT_FALSE(dsn_task_set_delay(nullptr, 0));
    ASSERT_FALSE(dsn_task_cancel(nullptr, false));
    ASSERT_FALSE(dsn_task_wait_timeout(nullptr, 0));

    bool finished = true;
    ASSERT_FALSE(dsn_task_cancel2(nullptr, false, &finished));
    ASSERT_FALSE(finished);

    auto compute_task = dsn_task_create(TASK_CODE_COMPUTE_FOR_TEST, noop_task_handler, nullptr, 0);
    ASSERT_NE(nullptr, compute_task);
    dsn_task_add_ref(compute_task);
    ASSERT_FALSE(dsn_task_call(compute_task, -1));
    ASSERT_FALSE(dsn_task_set_delay(compute_task, -1));
    dsn_task_release_ref(compute_task);

    auto aio_task = dsn_file_create_aio_task(TASK_CODE_AIO_FOR_TEST, noop_aio_handler, nullptr, 0);
    ASSERT_NE(nullptr, aio_task);
    dsn_task_add_ref(aio_task);
    ASSERT_FALSE(dsn_task_call(aio_task, 0));
    dsn_task_release_ref(aio_task);
}

TEST(core, dsn_task_create_invalid_parameters)
{
    ASSERT_EQ(nullptr, dsn_task_create(TASK_CODE_INVALID, noop_task_handler, nullptr, 0));
    ASSERT_EQ(nullptr, dsn_task_create(TASK_CODE_COMPUTE_FOR_TEST, nullptr, nullptr, 0));
    ASSERT_EQ(nullptr, dsn_task_create_ex(TASK_CODE_INVALID, noop_task_handler, nullptr, nullptr, 0));
    ASSERT_EQ(nullptr, dsn_task_create_ex(TASK_CODE_COMPUTE_FOR_TEST, nullptr, nullptr, nullptr, 0));
    ASSERT_EQ(nullptr, dsn_task_create_timer(TASK_CODE_INVALID, noop_task_handler, nullptr, 0, 1));
    ASSERT_EQ(nullptr, dsn_task_create_timer(TASK_CODE_COMPUTE_FOR_TEST, nullptr, nullptr, 0, 1));
    ASSERT_EQ(nullptr, dsn_task_create_timer(TASK_CODE_COMPUTE_FOR_TEST, noop_task_handler, nullptr, 0, -1));
    ASSERT_EQ(nullptr, dsn_task_create_timer_ex(TASK_CODE_INVALID,
                                                noop_task_handler,
                                                nullptr,
                                                nullptr,
                                                0,
                                                1));
    ASSERT_EQ(nullptr, dsn_task_create_timer_ex(TASK_CODE_COMPUTE_FOR_TEST,
                                                nullptr,
                                                nullptr,
                                                nullptr,
                                                0,
                                                1));
    ASSERT_EQ(nullptr, dsn_task_create_timer_ex(TASK_CODE_COMPUTE_FOR_TEST,
                                                noop_task_handler,
                                                nullptr,
                                                nullptr,
                                                0,
                                                -1));
    ASSERT_EQ(nullptr, dsn_task_tracker_create(0));
    ASSERT_EQ(nullptr, dsn_task_tracker_create(-1));
    dsn_task_tracker_cancel_all(nullptr);
    dsn_task_tracker_destroy(nullptr);
    dsn_task_tracker_wait_all(nullptr);
}

TEST(core, dsn_exlock)
{
    if(dsn::service_engine::fast_instance().spec().semaphore_factory_name == "dsn::tools::sim_semaphore_provider")
        return;
    {
        dsn_handle_t l = dsn_exlock_create(false);
        ASSERT_NE(nullptr, l);
        ASSERT_TRUE(dsn_exlock_try_lock(l));
        dsn_exlock_unlock(l);
        dsn_exlock_lock(l);
        dsn_exlock_unlock(l);
        dsn_exlock_destroy(l);
    }
    {
        dsn_handle_t l = dsn_exlock_create(true);
        ASSERT_NE(nullptr, l);
        ASSERT_TRUE(dsn_exlock_try_lock(l));
        ASSERT_TRUE(dsn_exlock_try_lock(l));
        dsn_exlock_unlock(l);
        dsn_exlock_unlock(l);
        dsn_exlock_lock(l);
        dsn_exlock_lock(l);
        dsn_exlock_unlock(l);
        dsn_exlock_unlock(l);
        dsn_exlock_destroy(l);
    }
}

TEST(core, dsn_sync_invalid_parameters)
{
    dsn_exlock_destroy(nullptr);
    dsn_exlock_lock(nullptr);
    ASSERT_FALSE(dsn_exlock_try_lock(nullptr));
    dsn_exlock_unlock(nullptr);

    dsn_rwlock_nr_destroy(nullptr);
    dsn_rwlock_nr_lock_read(nullptr);
    dsn_rwlock_nr_unlock_read(nullptr);
    ASSERT_FALSE(dsn_rwlock_nr_try_lock_read(nullptr));
    dsn_rwlock_nr_lock_write(nullptr);
    dsn_rwlock_nr_unlock_write(nullptr);
    ASSERT_FALSE(dsn_rwlock_nr_try_lock_write(nullptr));

    ASSERT_EQ(nullptr, dsn_semaphore_create(-1));
    dsn_semaphore_destroy(nullptr);
    dsn_semaphore_signal(nullptr, 1);
    dsn_semaphore_signal(nullptr, 0);
    dsn_semaphore_wait(nullptr);
    ASSERT_FALSE(dsn_semaphore_wait_timeout(nullptr, 0));

    auto semaphore = dsn_semaphore_create(0);
    ASSERT_NE(nullptr, semaphore);
    dsn_semaphore_signal(semaphore, 0);
    dsn_semaphore_signal(semaphore, -1);
    dsn_semaphore_destroy(semaphore);
}

TEST(core, dsn_rwlock)
{
    if(dsn::service_engine::fast_instance().spec().semaphore_factory_name == "dsn::tools::sim_semaphore_provider")
        return;
    dsn_handle_t l = dsn_rwlock_nr_create();
    ASSERT_NE(nullptr, l);
    dsn_rwlock_nr_lock_read(l);
    dsn_rwlock_nr_unlock_read(l);
    dsn_rwlock_nr_lock_write(l);
    dsn_rwlock_nr_unlock_write(l);
    dsn_rwlock_nr_destroy(l);
}

TEST(core, dsn_semaphore)
{
    if(dsn::service_engine::fast_instance().spec().semaphore_factory_name == "dsn::tools::sim_semaphore_provider")
        return;
    dsn_handle_t s = dsn_semaphore_create(2);
    dsn_semaphore_wait(s);
    ASSERT_TRUE(dsn_semaphore_wait_timeout(s, 10));
    ASSERT_FALSE(dsn_semaphore_wait_timeout(s, 10));
    dsn_semaphore_signal(s, 1);
    dsn_semaphore_wait(s);
    dsn_semaphore_destroy(s);
}

TEST(core, dsn_rpc)
{
}

TEST(core, dsn_rpc_registration_invalid_parameters)
{
    ASSERT_FALSE(dsn_rpc_register_handler(TASK_CODE_INVALID,
                                          "invalid_code",
                                          noop_rpc_request_handler,
                                          nullptr,
                                          dsn_gpid()));
    ASSERT_FALSE(dsn_rpc_register_handler(TASK_CODE_RPC_FOR_TEST,
                                          nullptr,
                                          noop_rpc_request_handler,
                                          nullptr,
                                          dsn_gpid()));
    ASSERT_FALSE(dsn_rpc_register_handler(TASK_CODE_RPC_FOR_TEST,
                                          "",
                                          noop_rpc_request_handler,
                                          nullptr,
                                          dsn_gpid()));
    ASSERT_FALSE(dsn_rpc_register_handler(TASK_CODE_RPC_FOR_TEST,
                                          "null_callback",
                                          nullptr,
                                          nullptr,
                                          dsn_gpid()));
    ASSERT_EQ(nullptr, dsn_rpc_unregiser_handler(TASK_CODE_INVALID, dsn_gpid()));
}

TEST(core, dsn_rpc_registration_conflict)
{
    int first_context = 1;
    int second_context = 2;

    ASSERT_TRUE(dsn_rpc_register_handler(TASK_CODE_RPC_FOR_TEST,
                                         "first_handler",
                                         noop_rpc_request_handler,
                                         &first_context,
                                         dsn_gpid()));
    ASSERT_FALSE(dsn_rpc_register_handler(TASK_CODE_RPC_FOR_TEST,
                                          "second_handler",
                                          noop_rpc_request_handler,
                                          &second_context,
                                          dsn_gpid()));
    ASSERT_EQ(&first_context, dsn_rpc_unregiser_handler(TASK_CODE_RPC_FOR_TEST, dsn_gpid()));
}

TEST(core, dsn_rpc_dispatch_invalid_parameters)
{
    const dsn_address_t invalid_address = {};

    ASSERT_EQ(ERR_INVALID_PARAMETERS, dsn_rpc_call(invalid_address, nullptr));
    ASSERT_EQ(ERR_INVALID_PARAMETERS, dsn_rpc_call_one_way(invalid_address, nullptr));
    ASSERT_EQ(ERR_INVALID_PARAMETERS, dsn_rpc_reply(nullptr, ERR_OK));
    ASSERT_EQ(ERR_INVALID_PARAMETERS, dsn_rpc_forward(nullptr, invalid_address));
    ASSERT_EQ(ERR_INVALID_PARAMETERS, dsn_rpc_enqueue_response(nullptr, ERR_OK, nullptr));

    auto request = dsn_msg_create_request(TASK_CODE_RPC_FOR_TEST, 100, 0, 0);
    ASSERT_NE(nullptr, request);
    dsn_msg_add_ref(request);

    ASSERT_EQ(nullptr, dsn_rpc_create_response_task_ex(nullptr,
                                                       noop_rpc_response_handler,
                                                       nullptr,
                                                       nullptr,
                                                       0,
                                                       nullptr));
    ASSERT_EQ(nullptr, dsn_rpc_call_wait(invalid_address, nullptr));
    ASSERT_EQ(nullptr, dsn_rpc_call_wait(invalid_address, request));

    ASSERT_EQ(ERR_INVALID_PARAMETERS, dsn_rpc_call_one_way(invalid_address, request));
    ASSERT_EQ(ERR_INVALID_PARAMETERS, dsn_rpc_forward(request, invalid_address));

    auto rpc_response_task =
        dsn_rpc_create_response_task(request, noop_rpc_response_handler, nullptr, 0);
    ASSERT_NE(nullptr, rpc_response_task);
    dsn_task_add_ref(rpc_response_task);
    ASSERT_EQ(ERR_INVALID_PARAMETERS, dsn_rpc_call(invalid_address, rpc_response_task));
    dsn_task_release_ref(rpc_response_task);

    // A valid, non-null message that is not a valid RPC request (here, a response message whose
    // code is the paired _ACK / TASK_TYPE_RPC_RESPONSE code) must be rejected with nullptr instead
    // of tripping the rpc_response_task constructor's invariant asserts and aborting the process.
    auto response = dsn_msg_create_response(request);
    ASSERT_NE(nullptr, response);
    dsn_msg_add_ref(response);
    ASSERT_EQ(nullptr,
              dsn_rpc_create_response_task(response, noop_rpc_response_handler, nullptr, 0));
    ASSERT_EQ(nullptr, dsn_rpc_create_response_task_ex(response,
                                                       noop_rpc_response_handler,
                                                       nullptr,
                                                       nullptr,
                                                       0,
                                                       nullptr));
    dsn_msg_release_ref(response);

    auto compute_task = dsn_task_create(TASK_CODE_COMPUTE_FOR_TEST, noop_task_handler, nullptr, 0);
    ASSERT_NE(nullptr, compute_task);
    dsn_task_add_ref(compute_task);
    ASSERT_EQ(ERR_INVALID_PARAMETERS, dsn_rpc_call(invalid_address, compute_task));
    ASSERT_EQ(ERR_INVALID_PARAMETERS, dsn_rpc_enqueue_response(compute_task, ERR_OK, nullptr));
    ASSERT_EQ(nullptr, dsn_rpc_get_response(compute_task));
    dsn_task_release_ref(compute_task);

    ASSERT_EQ(nullptr, dsn_rpc_get_response(nullptr));
    dsn_msg_release_ref(request);
}

TEST(core, dsn_hosted_app_invalid_parameters)
{
    auto fake_app_context = reinterpret_cast<void*>(1);
    void* downcall_context = nullptr;
    void* callback_context = nullptr;
    dsn_gpid invalid_app_id = {};
    invalid_app_id.u.app_id = 0;
    invalid_app_id.u.partition_index = 1;
    dsn_gpid invalid_partition_index = {};
    invalid_partition_index.u.app_id = 1;
    invalid_partition_index.u.partition_index = -1;
    dsn_gpid valid_gpid = {};
    valid_gpid.u.app_id = 1;
    valid_gpid.u.partition_index = 0;
    char* null_argv[] = {nullptr};

    ASSERT_EQ(ERR_INVALID_PARAMETERS,
              dsn_hosted_app_create(nullptr,
                                    valid_gpid,
                                    "data",
                                    &downcall_context,
                                    &callback_context));
    ASSERT_EQ(ERR_INVALID_PARAMETERS,
              dsn_hosted_app_create("",
                                    valid_gpid,
                                    "data",
                                    &downcall_context,
                                    &callback_context));
    ASSERT_EQ(ERR_INVALID_PARAMETERS,
              dsn_hosted_app_create("test",
                                    invalid_app_id,
                                    "data",
                                    &downcall_context,
                                    &callback_context));
    ASSERT_EQ(ERR_INVALID_PARAMETERS,
              dsn_hosted_app_create("test",
                                    invalid_partition_index,
                                    "data",
                                    &downcall_context,
                                    &callback_context));
    ASSERT_EQ(ERR_INVALID_PARAMETERS,
              dsn_hosted_app_create("test",
                                    valid_gpid,
                                    nullptr,
                                    &downcall_context,
                                    &callback_context));
    ASSERT_EQ(ERR_INVALID_PARAMETERS,
              dsn_hosted_app_create("test",
                                    valid_gpid,
                                    "",
                                    &downcall_context,
                                    &callback_context));
    ASSERT_EQ(ERR_INVALID_PARAMETERS,
              dsn_hosted_app_create("test", valid_gpid, "data", nullptr, &callback_context));
    ASSERT_EQ(ERR_INVALID_PARAMETERS,
              dsn_hosted_app_create("test", valid_gpid, "data", &downcall_context, nullptr));

    ASSERT_EQ(ERR_INVALID_PARAMETERS, dsn_hosted_app_start(nullptr, 0, nullptr));
    ASSERT_EQ(ERR_INVALID_PARAMETERS, dsn_hosted_app_start(fake_app_context, -1, nullptr));
    ASSERT_EQ(ERR_INVALID_PARAMETERS, dsn_hosted_app_start(fake_app_context, 1, nullptr));
    ASSERT_EQ(ERR_INVALID_PARAMETERS, dsn_hosted_app_start(fake_app_context, 1, null_argv));
    ASSERT_EQ(ERR_INVALID_PARAMETERS, dsn_hosted_app_destroy(nullptr, false));

    ASSERT_EQ(ERR_INVALID_PARAMETERS,
              dsn_hosted_app_commit_rpc_request(nullptr, nullptr, false));
    ASSERT_EQ(ERR_INVALID_PARAMETERS,
              dsn_hosted_app_commit_rpc_request(fake_app_context, nullptr, false));
}

TEST(core, dsn_app_registration_invalid_parameters)
{
    dsn_app_callbacks callbacks = {};

    ASSERT_FALSE(dsn_register_app(nullptr));
    dsn_app app = {};
    ASSERT_FALSE(dsn_register_app(&app));
    memset(app.type_name, 'a', sizeof(app.type_name));
    ASSERT_FALSE(dsn_register_app(&app));
    memset(app.type_name, 0, sizeof(app.type_name));
    snprintf(app.type_name, sizeof(app.type_name), "%s", "dsn_app_reg_test");
    ASSERT_TRUE(dsn_register_app(&app));
    ASSERT_FALSE(dsn_register_app(&app));

    ASSERT_FALSE(dsn_get_app_callbacks(nullptr, &callbacks));
    ASSERT_FALSE(dsn_get_app_callbacks("", &callbacks));
    ASSERT_FALSE(dsn_get_app_callbacks("test", nullptr));
    ASSERT_FALSE(dsn_register_app_checker(nullptr, noop_checker_create, noop_checker_apply));
    ASSERT_FALSE(dsn_register_app_checker("", noop_checker_create, noop_checker_apply));
    ASSERT_FALSE(dsn_register_app_checker("invalid_checker", nullptr, noop_checker_apply));
    ASSERT_FALSE(dsn_register_app_checker("invalid_checker", noop_checker_create, nullptr));
}

struct aio_result
{
    dsn_error_t err;
    size_t sz;
};

TEST(core, dsn_file_dispatch_invalid_parameters)
{
    auto fake_file = reinterpret_cast<dsn_handle_t>(1);
    char buffer[16];
    dsn_file_buffer_t valid_buffers[] = {{buffer, static_cast<int>(sizeof(buffer))}};
    const char* source_files[] = {"command.txt", nullptr};
    const auto remote = dsn_address_build("localhost", 20101);
    const dsn_address_t invalid_address = {};

    auto aio_task = dsn_file_create_aio_task(TASK_CODE_AIO_FOR_TEST, noop_aio_handler, nullptr, 0);
    ASSERT_NE(nullptr, aio_task);
    dsn_task_add_ref(aio_task);

    ASSERT_EQ(ERR_INVALID_PARAMETERS, dsn_file_read(nullptr, buffer, sizeof(buffer), 0, aio_task));
    ASSERT_EQ(ERR_INVALID_PARAMETERS, dsn_file_read(fake_file, nullptr, sizeof(buffer), 0, aio_task));
    ASSERT_EQ(ERR_INVALID_PARAMETERS, dsn_file_read(fake_file, buffer, -1, 0, aio_task));
    ASSERT_EQ(ERR_INVALID_PARAMETERS, dsn_file_read(fake_file, buffer, sizeof(buffer), 0, nullptr));

    ASSERT_EQ(ERR_INVALID_PARAMETERS, dsn_file_write(nullptr, buffer, sizeof(buffer), 0, aio_task));
    ASSERT_EQ(ERR_INVALID_PARAMETERS, dsn_file_write(fake_file, nullptr, sizeof(buffer), 0, aio_task));
    ASSERT_EQ(ERR_INVALID_PARAMETERS, dsn_file_write(fake_file, buffer, -1, 0, aio_task));
    ASSERT_EQ(ERR_INVALID_PARAMETERS, dsn_file_write(fake_file, buffer, sizeof(buffer), 0, nullptr));

    ASSERT_EQ(ERR_INVALID_PARAMETERS,
              dsn_file_write_vector(nullptr, valid_buffers, 1, 0, aio_task));
    ASSERT_EQ(ERR_INVALID_PARAMETERS, dsn_file_write_vector(fake_file, nullptr, 1, 0, aio_task));
    ASSERT_EQ(ERR_INVALID_PARAMETERS,
              dsn_file_write_vector(fake_file, valid_buffers, 0, 0, aio_task));
    ASSERT_EQ(ERR_INVALID_PARAMETERS,
              dsn_file_write_vector(fake_file, valid_buffers, -1, 0, aio_task));
    ASSERT_EQ(ERR_INVALID_PARAMETERS,
              dsn_file_write_vector(fake_file, valid_buffers, 1, 0, nullptr));

    if (task::get_current_disk() != nullptr)
    {
        dsn_file_buffer_t negative_size_buffers[] = {{buffer, -1}};
        dsn_file_buffer_t null_data_buffers[] = {{nullptr, static_cast<int>(sizeof(buffer))}};
        ASSERT_EQ(ERR_INVALID_PARAMETERS,
                  dsn_file_write_vector(fake_file, negative_size_buffers, 1, 0, aio_task));
        ASSERT_EQ(ERR_INVALID_PARAMETERS,
                  dsn_file_write_vector(fake_file, null_data_buffers, 1, 0, aio_task));
    }

    ASSERT_EQ(ERR_INVALID_PARAMETERS,
              dsn_file_copy_remote_directory(invalid_address, ".", ".", false, aio_task));
    ASSERT_EQ(ERR_INVALID_PARAMETERS,
              dsn_file_copy_remote_directory(remote, nullptr, ".", false, aio_task));
    ASSERT_EQ(ERR_INVALID_PARAMETERS,
              dsn_file_copy_remote_directory(remote, "", ".", false, aio_task));
    ASSERT_EQ(ERR_INVALID_PARAMETERS,
              dsn_file_copy_remote_directory(remote, ".", nullptr, false, aio_task));
    ASSERT_EQ(ERR_INVALID_PARAMETERS,
              dsn_file_copy_remote_directory(remote, ".", "", false, aio_task));
    ASSERT_EQ(ERR_INVALID_PARAMETERS,
              dsn_file_copy_remote_directory(remote, ".", ".", false, nullptr));

    ASSERT_EQ(ERR_INVALID_PARAMETERS,
              dsn_file_copy_remote_files(invalid_address, ".", source_files, ".", false, aio_task));
    ASSERT_EQ(ERR_INVALID_PARAMETERS,
              dsn_file_copy_remote_files(remote, nullptr, source_files, ".", false, aio_task));
    ASSERT_EQ(ERR_INVALID_PARAMETERS,
              dsn_file_copy_remote_files(remote, "", source_files, ".", false, aio_task));
    ASSERT_EQ(ERR_INVALID_PARAMETERS,
              dsn_file_copy_remote_files(remote, ".", nullptr, ".", false, aio_task));
    ASSERT_EQ(ERR_INVALID_PARAMETERS,
              dsn_file_copy_remote_files(remote, ".", source_files, nullptr, false, aio_task));
    ASSERT_EQ(ERR_INVALID_PARAMETERS,
              dsn_file_copy_remote_files(remote, ".", source_files, "", false, aio_task));
    ASSERT_EQ(ERR_INVALID_PARAMETERS,
              dsn_file_copy_remote_files(remote, ".", source_files, ".", false, nullptr));

    ASSERT_EQ(ERR_INVALID_PARAMETERS, dsn_file_task_enqueue(nullptr, ERR_OK, 0));

    auto compute_task = dsn_task_create(TASK_CODE_COMPUTE_FOR_TEST, noop_task_handler, nullptr, 0);
    ASSERT_NE(nullptr, compute_task);
    dsn_task_add_ref(compute_task);
    ASSERT_EQ(ERR_INVALID_PARAMETERS, dsn_file_task_enqueue(compute_task, ERR_OK, 0));
    dsn_task_release_ref(compute_task);

    dsn_task_release_ref(aio_task);
}

TEST(core, dsn_file_write_vector_rejection_is_transactional)
{
    if (task::get_current_disk() == nullptr)
    {
        return;
    }

    auto aio_task =
        dsn_file_create_aio_task(TASK_CODE_AIO_FOR_TEST, noop_aio_handler, nullptr, 0);
    ASSERT_NE(nullptr, aio_task);
    dsn_task_add_ref(aio_task);

    auto *callback_task = static_cast< ::dsn::aio_task *>(aio_task);
    char sentinel = 0;
    const auto original_file = reinterpret_cast<dsn_handle_t>(1);
    callback_task->aio()->file = original_file;
    callback_task->aio()->engine = task::get_current_disk();
    callback_task->aio()->buffer = &sentinel;
    callback_task->aio()->buffer_size = 17;
    callback_task->aio()->file_offset = 99;
    callback_task->aio()->type = AIO_Read;
    callback_task->_unmerged_write_buffers.push_back({&sentinel, 1});

    dsn_file_buffer_t buffers[] = {
        {&sentinel, INT_MAX}, {&sentinel, INT_MAX}, {&sentinel, INT_MAX}};
    EXPECT_EQ(ERR_INVALID_PARAMETERS,
              dsn_file_write_vector(
                  reinterpret_cast<dsn_handle_t>(2), buffers, 3, 123, aio_task));

    EXPECT_EQ(original_file, callback_task->aio()->file);
    EXPECT_EQ(task::get_current_disk(), callback_task->aio()->engine);
    EXPECT_EQ(&sentinel, callback_task->aio()->buffer);
    EXPECT_EQ(17u, callback_task->aio()->buffer_size);
    EXPECT_EQ(99u, callback_task->aio()->file_offset);
    EXPECT_EQ(AIO_Read, callback_task->aio()->type);
    ASSERT_EQ(1u, callback_task->_unmerged_write_buffers.size());
    EXPECT_EQ(&sentinel, callback_task->_unmerged_write_buffers[0].buffer);
    EXPECT_EQ(1u, callback_task->_unmerged_write_buffers[0].size);

    dsn_task_release_ref(aio_task);
}

TEST(core, dsn_file_handle_invalid_parameters)
{
    ASSERT_EQ(nullptr, dsn_file_open(nullptr, O_RDONLY, 0));
    ASSERT_EQ(nullptr, dsn_file_open("", O_RDONLY, 0));
    ASSERT_EQ(ERR_INVALID_PARAMETERS, dsn_file_close(nullptr));
    ASSERT_EQ(ERR_INVALID_PARAMETERS, dsn_file_flush(nullptr));
    ASSERT_EQ(nullptr, dsn_file_native_handle(nullptr));
    ASSERT_EQ(nullptr, dsn_file_create_aio_task(TASK_CODE_INVALID, noop_aio_handler, nullptr, 0));
    ASSERT_EQ(nullptr,
              dsn_file_create_aio_task_ex(TASK_CODE_INVALID,
                                          noop_aio_handler,
                                          nullptr,
                                          nullptr,
                                          0));
    ASSERT_EQ(static_cast<size_t>(-1), dsn_file_get_io_size(nullptr));

    auto compute_task = dsn_task_create(TASK_CODE_COMPUTE_FOR_TEST, noop_task_handler, nullptr, 0);
    ASSERT_NE(nullptr, compute_task);
    dsn_task_add_ref(compute_task);
    ASSERT_EQ(static_cast<size_t>(-1), dsn_file_get_io_size(compute_task));
    dsn_task_release_ref(compute_task);
}

TEST(core, dsn_app_info_invalid_parameters)
{
    dsn_gpid invalid_app_id = {};
    invalid_app_id.u.app_id = 0;
    invalid_app_id.u.partition_index = 1;
    dsn_gpid invalid_partition_index = {};
    invalid_partition_index.u.app_id = 1;
    invalid_partition_index.u.partition_index = -1;

    ASSERT_FALSE(dsn_mimic_app(nullptr, 1));
    ASSERT_FALSE(dsn_mimic_app("", 1));
    ASSERT_FALSE(dsn_mimic_app("client", 0));
    ASSERT_FALSE(dsn_get_current_app_info(nullptr));
    ASSERT_EQ(-1, dsn_get_all_apps(nullptr, 1));
    dsn_app_info apps[1];
    ASSERT_EQ(-1, dsn_get_all_apps(apps, -1));
    ASSERT_EQ(nullptr, dsn_get_app_data_dir(invalid_app_id));
    ASSERT_EQ(nullptr, dsn_get_app_data_dir(invalid_partition_index));
    ASSERT_EQ(nullptr, dsn_get_app_info_ptr(invalid_app_id));
    ASSERT_EQ(nullptr, dsn_get_app_info_ptr(invalid_partition_index));
}

TEST(core, dsn_file)
{
    // if in dsn_mimic_app() and disk_io_mode == IOE_PER_QUEUE
    if (task::get_current_disk() == nullptr) return;

    int64_t fin_size, fout_size;
    ASSERT_TRUE(utils::filesystem::file_size("command.txt", fin_size));
    ASSERT_LT(0, fin_size);

    dsn_handle_t fin = dsn_file_open("command.txt", O_RDONLY, 0);
    ASSERT_NE(nullptr, fin);
    dsn_handle_t fout = dsn_file_open("command.copy.txt", O_RDWR | O_CREAT | O_TRUNC, 0666);
    ASSERT_NE(nullptr, fout);
    char buffer[1024];
    uint64_t offset = 0;
    while (true)
    {
        aio_result rin;
        dsn_task_t tin = dsn_file_create_aio_task(LPC_AIO_TEST_READ,
            [](dsn_error_t err, size_t sz, void* param)
            {
                aio_result* r = (aio_result*)param;
                r->err = err;
                r->sz = sz;
            },
            &rin, 0);
        dsn_task_add_ref(tin);
        ASSERT_NE(nullptr, tin);
        ASSERT_EQ(1, dsn_task_get_ref(tin));
        ASSERT_EQ(ERR_OK, dsn_file_read(fin, buffer, 1024, offset, tin));
        dsn_task_wait(tin);
        ASSERT_EQ(rin.err, dsn_task_error(tin));
        if (rin.err != ERR_OK)
        {
            ASSERT_EQ(ERR_HANDLE_EOF, rin.err);
            break;
        }
        ASSERT_LT(0u, rin.sz);
        ASSERT_EQ(rin.sz, dsn_file_get_io_size(tin));
        // this is only true for emulator
        if (dsn::tools::get_current_tool()->name() == "emulator")
        {
            ASSERT_EQ(1, dsn_task_get_ref(tin));
        }
        dsn_task_release_ref(tin);

        aio_result rout;
        dsn_task_t tout = dsn_file_create_aio_task(LPC_AIO_TEST_WRITE,
            [](dsn_error_t err, size_t sz, void* param)
            {
                aio_result* r = (aio_result*)param;
                r->err = err;
                r->sz = sz;
            },
            &rout, 0);
        dsn_task_add_ref(tout);
        ASSERT_NE(nullptr, tout);
        ASSERT_EQ(ERR_OK, dsn_file_write(fout, buffer, rin.sz, offset, tout));
        dsn_task_wait(tout);
        ASSERT_EQ(ERR_OK, rout.err);
        ASSERT_EQ(ERR_OK, dsn_task_error(tout));
        ASSERT_EQ(rin.sz, rout.sz);
        ASSERT_EQ(rin.sz, dsn_file_get_io_size(tout));
        // this is only true for emulator
        if (dsn::tools::get_current_tool()->name() == "emulator")
        {
            ASSERT_EQ(1, dsn_task_get_ref(tout));
        }
        dsn_task_release_ref(tout);

        ASSERT_EQ(ERR_OK, dsn_file_flush(fout));

        offset += rin.sz;
    }

    ASSERT_EQ((uint64_t)fin_size, offset);
    ASSERT_EQ(ERR_OK, dsn_file_close(fout));
    ASSERT_EQ(ERR_OK, dsn_file_close(fin));

    ASSERT_TRUE(utils::filesystem::file_size("command.copy.txt", fout_size));
    ASSERT_EQ(fin_size, fout_size);
}

//TODO: On windows an opened file cannot be deleted, so this test cannot pass
#ifndef WIN32
TEST(core, dsn_nfs)
{
    // if in dsn_mimic_app() and nfs_io_mode == IOE_PER_QUEUE
    if (task::get_current_nfs() == nullptr) return;

    ASSERT_TRUE(::dsn::utils::test::prepare_test_tmp_dir("dsn.core.nfs"));
    const std::string nfs_test_dir = ::dsn::utils::test::test_tmp_path("dsn.core.nfs", "nfs_test_dir");
    const std::string nfs_test_dir_copy =
        ::dsn::utils::test::test_tmp_path("dsn.core.nfs", "nfs_test_dir_copy");

    ASSERT_FALSE(utils::filesystem::directory_exists(nfs_test_dir));
    ASSERT_FALSE(utils::filesystem::directory_exists(nfs_test_dir_copy));

    ASSERT_TRUE(utils::filesystem::create_directory(nfs_test_dir));
    ASSERT_TRUE(utils::filesystem::directory_exists(nfs_test_dir));

    { // copy nfs_test_file1 nfs_test_file2 nfs_test_dir
        const std::string copied_file1 =
            utils::filesystem::path_combine(nfs_test_dir, "nfs_test_file1");
        const std::string copied_file2 =
            utils::filesystem::path_combine(nfs_test_dir, "nfs_test_file2");
        ASSERT_FALSE(utils::filesystem::file_exists(copied_file1));
        ASSERT_FALSE(utils::filesystem::file_exists(copied_file2));

        const char* files[] = { "nfs_test_file1", "nfs_test_file2", nullptr };

        aio_result r;
        dsn_task_t t = dsn_file_create_aio_task(LPC_AIO_TEST_NFS,
                [](dsn_error_t err, size_t sz, void* param)
                {
                aio_result* r = (aio_result*)param;
                r->err = err;
                r->sz = sz;
                },
                &r, 0);
        dsn_task_add_ref(t);
        ASSERT_NE(nullptr, t);
        ASSERT_EQ(ERR_OK, dsn_file_copy_remote_files(dsn_address_build("localhost", 20101),
                ".", files, nfs_test_dir.c_str(), false, t));
        ASSERT_TRUE(dsn_task_wait_timeout(t, 20000));
        ASSERT_EQ(r.err, dsn_task_error(t));
        ASSERT_EQ(ERR_OK, r.err);
        ASSERT_EQ(r.sz, dsn_file_get_io_size(t));
        // this is only true for emulator
        if (dsn::tools::get_current_tool()->name() == "emulator")
        {
            ASSERT_EQ(1, dsn_task_get_ref(t));
        }
        dsn_task_release_ref(t);

        ASSERT_TRUE(utils::filesystem::file_exists(copied_file1));
        ASSERT_TRUE(utils::filesystem::file_exists(copied_file2));

        int64_t sz1, sz2;
        ASSERT_TRUE(utils::filesystem::file_size("nfs_test_file1", sz1));
        ASSERT_TRUE(utils::filesystem::file_size(copied_file1, sz2));
        ASSERT_EQ(sz1, sz2);
        ASSERT_TRUE(utils::filesystem::file_size("nfs_test_file2", sz1));
        ASSERT_TRUE(utils::filesystem::file_size(copied_file2, sz2));
        ASSERT_EQ(sz1, sz2);
    }

    { // copy files again, overwrite
        ASSERT_TRUE(utils::filesystem::file_exists(
            utils::filesystem::path_combine(nfs_test_dir, "nfs_test_file1")));
        ASSERT_TRUE(utils::filesystem::file_exists(
            utils::filesystem::path_combine(nfs_test_dir, "nfs_test_file2")));

        const char* files[] = { "nfs_test_file1", "nfs_test_file2", nullptr };

        aio_result r;
        dsn_task_t t = dsn_file_create_aio_task(LPC_AIO_TEST_NFS,
                [](dsn_error_t err, size_t sz, void* param)
                {
                aio_result* r = (aio_result*)param;
                r->err = err;
                r->sz = sz;
                },
                &r, 0);
        dsn_task_add_ref(t);
        ASSERT_NE(nullptr, t);
        ASSERT_EQ(ERR_OK, dsn_file_copy_remote_files(dsn_address_build("localhost", 20101),
                ".", files, nfs_test_dir.c_str(), true, t));
        ASSERT_TRUE(dsn_task_wait_timeout(t, 20000));
        ASSERT_EQ(r.err, dsn_task_error(t));
        ASSERT_EQ(ERR_OK, r.err);
        ASSERT_EQ(r.sz, dsn_file_get_io_size(t));
        // this is only true for emulator
        if (dsn::tools::get_current_tool()->name() == "emulator")
        {
            ASSERT_EQ(1, dsn_task_get_ref(t));
        }
        dsn_task_release_ref(t);
    }

    { // copy nfs_test_dir nfs_test_dir_copy
        ASSERT_FALSE(utils::filesystem::directory_exists(nfs_test_dir_copy));

        aio_result r;
        dsn_task_t t = dsn_file_create_aio_task(LPC_AIO_TEST_NFS,
                [](dsn_error_t err, size_t sz, void* param)
                {
                aio_result* r = (aio_result*)param;
                r->err = err;
                r->sz = sz;
                },
                &r, 0);
        dsn_task_add_ref(t);
        ASSERT_NE(nullptr, t);
        ASSERT_EQ(ERR_OK, dsn_file_copy_remote_directory(dsn_address_build("localhost", 20101),
                nfs_test_dir.c_str(), nfs_test_dir_copy.c_str(), false, t));
        ASSERT_TRUE(dsn_task_wait_timeout(t, 20000));
        ASSERT_EQ(r.err, dsn_task_error(t));
        ASSERT_EQ(ERR_OK, r.err);
        ASSERT_EQ(r.sz, dsn_file_get_io_size(t));
        // this is only true for emulator
        if (dsn::tools::get_current_tool()->name() == "emulator")
        {
            ASSERT_EQ(1, dsn_task_get_ref(t));
        }
        dsn_task_release_ref(t);

        ASSERT_TRUE(utils::filesystem::directory_exists(nfs_test_dir_copy));
        ASSERT_TRUE(utils::filesystem::file_exists(
            utils::filesystem::path_combine(nfs_test_dir_copy, "nfs_test_file1")));
        ASSERT_TRUE(utils::filesystem::file_exists(
            utils::filesystem::path_combine(nfs_test_dir_copy, "nfs_test_file2")));

        std::vector<std::string> sub1, sub2;
        ASSERT_TRUE(utils::filesystem::get_subfiles(nfs_test_dir, sub1, true));
        ASSERT_TRUE(utils::filesystem::get_subfiles(nfs_test_dir_copy, sub2, true));
        ASSERT_EQ(sub1.size(), sub2.size());

        int64_t sz1, sz2;
        ASSERT_TRUE(utils::filesystem::file_size(
            utils::filesystem::path_combine(nfs_test_dir, "nfs_test_file1"), sz1));
        ASSERT_TRUE(utils::filesystem::file_size(
            utils::filesystem::path_combine(nfs_test_dir_copy, "nfs_test_file1"), sz2));
        ASSERT_EQ(sz1, sz2);
        ASSERT_TRUE(utils::filesystem::file_size(
            utils::filesystem::path_combine(nfs_test_dir, "nfs_test_file2"), sz1));
        ASSERT_TRUE(utils::filesystem::file_size(
            utils::filesystem::path_combine(nfs_test_dir_copy, "nfs_test_file2"), sz2));
        ASSERT_EQ(sz1, sz2);
    }
}
#endif

TEST(core, dsn_env)
{
    if(dsn::service_engine::fast_instance().spec().tool == "emulator")
        return;
    uint64_t now1 = dsn_now_ns();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    uint64_t now2 = dsn_now_ns();
    ASSERT_LE(now1 + 1000000, now2);
    uint64_t r = dsn_random64(100, 200);
    ASSERT_LE(100, r);
    ASSERT_GE(200, r);
}

TEST(core, dsn_system)
{
    ASSERT_TRUE(tools::is_engine_ready());
    tools::tool_app* tool = tools::get_current_tool();
    ASSERT_EQ(tool->name(), dsn_config_get_value_string("core", "tool", "", ""));

    int app_count = 5;
    int type_count = 1;
    if (tool->get_service_spec().enable_default_app_mimic)
    {
        app_count++;
        type_count++;
    }   

    {
        dsn_app_info apps[20];
        int count = dsn_get_all_apps(apps, 20);
        ASSERT_EQ(app_count, count);
        std::map<std::string, int> type_to_count;
        for (int i = 0; i < count; ++i)
        {
            type_to_count[apps[i].type] += 1;
        }

        ASSERT_EQ(type_count, static_cast<int>(type_to_count.size()));
        ASSERT_EQ(5, type_to_count["test"]);

        count = dsn_get_all_apps(apps, 3);
        ASSERT_EQ(app_count, count);
    }
}
