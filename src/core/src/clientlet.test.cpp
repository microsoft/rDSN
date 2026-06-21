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
 *     Unit-test for clientlet.
 *
 * Revision history:
 *     Nov., 2015, @shengofsun (Weijie Sun), first version
 *     xxxx-xx-xx, author, fix bug about xxx
 */

#include <dsn/cpp/clientlet.h>
#include <dsn/cpp/layer2_handler.h>
#include <dsn/cpp/replicated_service_app.h>
#include <dsn/cpp/serverlet.h>
#include <gtest/gtest.h>
#include <functional>
#include <chrono>
#include <dsn/cpp/test_utils.h>

DEFINE_TASK_CODE(LPC_TEST_CLIENTLET, TASK_PRIORITY_COMMON, THREAD_POOL_TEST_SERVER)
using namespace dsn;

int global_value;
class test_clientlet: public clientlet {
public:
    std::string str;
    int number;

public:
    test_clientlet(): clientlet(), str("before called"), number(0) {
        global_value = 0;
    }
    void callback_function1()
    {
        check_hashed_access();
        str = "after called";
        ++global_value;
    }

    void callback_function2() {
        check_hashed_access();
        number = 0;
        for (int i=0; i<1000; ++i)
            number+=i;
        ++global_value;
    }

    void callback_function3() { ++global_value; }
};

TEST(dev_cpp, clientlet_task)
{
    /* normal lpc*/
    test_clientlet *cl = new test_clientlet();
    task_ptr t = tasking::enqueue(LPC_TEST_CLIENTLET, cl, [cl] {cl->callback_function1();});
    EXPECT_TRUE(t != nullptr);
    t->wait();
    EXPECT_TRUE(cl->str == "after called");
    delete cl;

    /* task tracking */
    cl = new test_clientlet();
    std::vector<task_ptr> test_tasks;
    t = tasking::enqueue(LPC_TEST_CLIENTLET, cl, [=] {cl->callback_function1();}, 0, std::chrono::seconds(30));
    test_tasks.push_back(t);
    t = tasking::enqueue(LPC_TEST_CLIENTLET, cl, [cl] {cl->callback_function1();}, 0, std::chrono::seconds(30));
    test_tasks.push_back(t);
    t = tasking::enqueue_timer(LPC_TEST_CLIENTLET, cl, [cl] {cl->callback_function1();}, std::chrono::seconds(20), 0, std::chrono::seconds(30));
    test_tasks.push_back(t);

    delete cl;
    for (unsigned int i=0; i!=test_tasks.size(); ++i)
        EXPECT_FALSE( test_tasks[i]->cancel(true) );
}

TEST(dev_cpp, clientlet_task_invalid_parameters)
{
    safe_task_handle invalid_task;
    EXPECT_FALSE(invalid_task.cancel(false));
    invalid_task.wait();
    EXPECT_EQ(ERR_INVALID_PARAMETERS, invalid_task.error());
    EXPECT_EQ(static_cast<size_t>(-1), invalid_task.io_size());
    EXPECT_EQ(nullptr, invalid_task.response());

    auto task = tasking::create_task(TASK_CODE_INVALID, nullptr, [] {});
    EXPECT_EQ(nullptr, task.get());

    task = tasking::create_timer_task(TASK_CODE_INVALID, nullptr, [] {}, std::chrono::milliseconds(1));
    EXPECT_EQ(nullptr, task.get());

    task = tasking::enqueue(TASK_CODE_INVALID, nullptr, [] {});
    EXPECT_EQ(nullptr, task.get());

    task = tasking::enqueue_timer(TASK_CODE_INVALID, nullptr, [] {}, std::chrono::milliseconds(1));
    EXPECT_EQ(nullptr, task.get());

    auto late_task = tasking::create_late_task(TASK_CODE_INVALID, [] {});
    EXPECT_EQ(nullptr, late_task);
}

TEST(dev_cpp, clientlet_rpc)
{
    rpc_address addr("localhost", 20101);
    rpc_address addr2("localhost", TEST_PORT_END);
    rpc_address addr3("localhost", 32767);

    test_clientlet* cl = new test_clientlet();
    rpc::call_one_way_typed(addr, RPC_TEST_STRING_COMMAND, std::string("expect_no_reply"), 0);
    std::vector< task_ptr > task_vec;
    const char* command = "echo hello world";

    std::shared_ptr<std::string> str_command(new std::string(command));
    auto t = rpc::call(
        addr3,
        RPC_TEST_STRING_COMMAND,
        *str_command,
        cl,
        [str_command](error_code ec, std::string&& resp)
        {
            if (ERR_OK == ec)
            {
                EXPECT_TRUE(str_command->substr(5) == resp);
            }
        }
    );
    task_vec.push_back(t);
    t = rpc::call(
        addr2,
        RPC_TEST_STRING_COMMAND,
        std::string(command),
        cl,
        [](error_code ec, std::string&& resp)
        {
            EXPECT_TRUE(ec == ERR_OK);
        }
    );
    task_vec.push_back(t);
    for (int i=0; i!=task_vec.size(); ++i)
        task_vec[i]->wait();
}

TEST(dev_cpp, clientlet_rpc_invalid_parameters)
{
    auto response_task = rpc::create_rpc_response_task(nullptr, nullptr, empty_callback);
    EXPECT_EQ(nullptr, response_task.get());

    response_task = rpc::create_rpc_response_task(
        nullptr, nullptr, [](error_code, dsn_message_t, dsn_message_t) {});
    EXPECT_EQ(nullptr, response_task.get());

    auto call_task = rpc::call(rpc_address(), static_cast<dsn_message_t>(nullptr), nullptr, empty_callback);
    EXPECT_EQ(nullptr, call_task.get());

    call_task = rpc::call(rpc_address(), TASK_CODE_INVALID, std::string("request"), nullptr, empty_callback);
    EXPECT_EQ(nullptr, call_task.get());

    call_task = rpc::create_message(TASK_CODE_INVALID, std::string("request"))
                    .call(rpc_address(), nullptr, empty_callback);
    EXPECT_EQ(nullptr, call_task.get());

    auto wait_result = rpc::call_wait<std::string>(
        rpc_address(), TASK_CODE_INVALID, std::string("request"));
    EXPECT_EQ(ERR_INVALID_PARAMETERS, wait_result.first);
    EXPECT_EQ(std::string(), wait_result.second);

    rpc::call_one_way_typed(rpc_address(), TASK_CODE_INVALID, std::string("request"));
}

class test_service_app_for_invalid : public service_app
{
public:
    explicit test_service_app_for_invalid(dsn_gpid gpid) : service_app(gpid) {}

    error_code start(int, char**) override { return ERR_OK; }
    error_code stop(bool) override { return ERR_OK; }
};

class test_layer2_handler_for_invalid : public layer2_handler
{
public:
    explicit test_layer2_handler_for_invalid(dsn_gpid gpid) : layer2_handler(gpid), request_count(0) {}

    error_code start(int, char**) override { return ERR_OK; }
    error_code stop(bool) override { return ERR_OK; }
    void on_request(dsn_gpid, bool, dsn_message_t) override { ++request_count; }

    int request_count;
};

class test_replicated_service_app_for_invalid : public replicated_service_app_type_1
{
public:
    explicit test_replicated_service_app_for_invalid(dsn_gpid gpid)
        : replicated_service_app_type_1(gpid)
    {
    }

    error_code start(int, char**) override { return ERR_OK; }
    error_code stop(bool) override { return ERR_OK; }
    error_code get_checkpoint(int64_t, int64_t, void*, int, app_learn_state&) override
    {
        return ERR_OK;
    }
    error_code apply_checkpoint(dsn_chkpt_apply_mode, int64_t, const dsn_app_learn_state&) override
    {
        return ERR_OK;
    }
};

class test_serverlet_for_invalid : public serverlet<test_serverlet_for_invalid>
{
public:
    test_serverlet_for_invalid() : serverlet<test_serverlet_for_invalid>(nullptr) {}

    bool register_raw_public(dsn_task_code_t code,
                             const char* name,
                             void (test_serverlet_for_invalid::*handler)(dsn_message_t))
    {
        return register_rpc_handler(code, name, handler);
    }

    bool register_one_public(dsn_task_code_t code,
                             const char* name,
                             void (test_serverlet_for_invalid::*handler)(const std::string&))
    {
        return register_rpc_handler(code, name, handler);
    }

    bool register_response_public(
        dsn_task_code_t code,
        const char* name,
        void (test_serverlet_for_invalid::*handler)(const std::string&, std::string&))
    {
        return register_rpc_handler(code, name, handler);
    }

    bool register_async_public(
        dsn_task_code_t code,
        const char* name,
        void (test_serverlet_for_invalid::*handler)(const std::string&, rpc_replier<std::string>&))
    {
        return register_async_rpc_handler(code, name, handler);
    }

    bool unregister_public(dsn_task_code_t code) { return unregister_rpc_handler(code); }
    void reply_public(dsn_message_t request) { reply(request, std::string("response")); }

    void on_raw(dsn_message_t) {}
    void on_one(const std::string&) {}
    void on_response(const std::string&, std::string&) {}
    void on_async(const std::string&, rpc_replier<std::string>&) {}
};

TEST(dev_cpp, service_cpp_invalid_parameters)
{
    char* argv[] = {const_cast<char*>("test_app")};
    char* null_argv[] = {nullptr};

    EXPECT_EQ(ERR_INVALID_PARAMETERS, service_app::app_start(nullptr, 1, argv));
    test_service_app_for_invalid app{dsn_gpid()};
    EXPECT_EQ(ERR_INVALID_PARAMETERS, service_app::app_start(&app, -1, argv));
    EXPECT_EQ(ERR_INVALID_PARAMETERS, service_app::app_start(&app, 0, nullptr));
    EXPECT_EQ(ERR_INVALID_PARAMETERS, service_app::app_start(&app, 1, nullptr));
    EXPECT_EQ(ERR_INVALID_PARAMETERS, service_app::app_start(&app, 1, null_argv));
    EXPECT_EQ(ERR_INVALID_PARAMETERS, service_app::app_destroy(nullptr, false));

    EXPECT_FALSE(register_app<test_service_app_for_invalid>(nullptr));
    EXPECT_FALSE(register_app<test_service_app_for_invalid>(""));
}

TEST(dev_cpp, serverlet_cpp_invalid_parameters)
{
    test_serverlet_for_invalid server;
    EXPECT_TRUE(server.name().empty());

    EXPECT_FALSE(server.register_raw_public(
        TASK_CODE_INVALID, "invalid_code", &test_serverlet_for_invalid::on_raw));
    EXPECT_FALSE(server.register_raw_public(
        RPC_TEST_STRING_COMMAND, nullptr, &test_serverlet_for_invalid::on_raw));

    void (test_serverlet_for_invalid::*raw_handler)(dsn_message_t) = nullptr;
    EXPECT_FALSE(server.register_raw_public(RPC_TEST_STRING_COMMAND, "null_raw", raw_handler));

    void (test_serverlet_for_invalid::*one_handler)(const std::string&) = nullptr;
    EXPECT_FALSE(server.register_one_public(RPC_TEST_STRING_COMMAND, "null_one", one_handler));

    void (test_serverlet_for_invalid::*response_handler)(const std::string&, std::string&) =
        nullptr;
    EXPECT_FALSE(
        server.register_response_public(RPC_TEST_STRING_COMMAND, "null_response", response_handler));

    void (test_serverlet_for_invalid::*async_handler)(const std::string&,
                                                      rpc_replier<std::string>&) = nullptr;
    EXPECT_FALSE(server.register_async_public(RPC_TEST_STRING_COMMAND, "null_async", async_handler));

    EXPECT_FALSE(server.unregister_public(TASK_CODE_INVALID));
}

TEST(dev_cpp, layer2_cpp_invalid_parameters)
{
    layer2_handler::on_layer2_rpc_request(nullptr, dsn_gpid(), false, nullptr);

    test_layer2_handler_for_invalid handler{dsn_gpid()};
    layer2_handler::on_layer2_rpc_request(&handler, dsn_gpid(), false, nullptr);
    EXPECT_EQ(1, handler.request_count);

    EXPECT_FALSE(register_layer2_framework<test_layer2_handler_for_invalid>(nullptr, DSN_APP_MASK_FRAMEWORK));
    EXPECT_FALSE(register_layer2_framework<test_layer2_handler_for_invalid>("", DSN_APP_MASK_FRAMEWORK));
}

TEST(dev_cpp, replicated_service_cpp_invalid_parameters)
{
    EXPECT_EQ(ERR_INVALID_PARAMETERS, replicated_service_app_type_1::app_get_physical_error(nullptr));
    EXPECT_EQ(ERR_INVALID_PARAMETERS, replicated_service_app_type_1::app_sync_checkpoint(nullptr, 0));
    EXPECT_EQ(ERR_INVALID_PARAMETERS, replicated_service_app_type_1::app_async_checkpoint(nullptr, 0));
    EXPECT_EQ(-1, replicated_service_app_type_1::app_get_last_checkpoint_decree(nullptr));
    EXPECT_EQ(ERR_INVALID_PARAMETERS,
              replicated_service_app_type_1::app_prepare_get_checkpoint(nullptr, nullptr, 0, nullptr));

    dsn_app_learn_state state;
    EXPECT_EQ(ERR_INVALID_PARAMETERS,
              replicated_service_app_type_1::app_get_checkpoint(
                  nullptr, 0, 0, nullptr, 0, &state, sizeof(state)));
    EXPECT_EQ(ERR_INVALID_PARAMETERS,
              replicated_service_app_type_1::app_get_checkpoint(
                  reinterpret_cast<void*>(1), 0, 0, nullptr, 0, nullptr, sizeof(state)));
    EXPECT_EQ(ERR_INVALID_PARAMETERS,
              replicated_service_app_type_1::app_get_checkpoint(
                  reinterpret_cast<void*>(1), 0, 0, nullptr, 0, &state, -1));
    EXPECT_EQ(ERR_INVALID_PARAMETERS,
              replicated_service_app_type_1::app_apply_checkpoint(
                  nullptr, DSN_CHKPT_LEARN, 0, &state));
    EXPECT_EQ(ERR_INVALID_PARAMETERS,
              replicated_service_app_type_1::app_apply_checkpoint(
                  reinterpret_cast<void*>(1), DSN_CHKPT_LEARN, 0, nullptr));
    replicated_service_app_type_1::app_on_batched_write_requests(nullptr, 0, nullptr, 0);
    replicated_service_app_type_1::app_on_batched_write_requests(
        reinterpret_cast<void*>(1), 0, nullptr, -1);
    replicated_service_app_type_1::app_on_batched_write_requests(
        reinterpret_cast<void*>(1), 0, nullptr, 1);

    test_replicated_service_app_for_invalid app{dsn_gpid()};
    int occupied = 0;
    EXPECT_EQ(ERR_INVALID_PARAMETERS,
              replicated_service_app_type_1::app_prepare_get_checkpoint(
                  &app, nullptr, 0, nullptr));
    EXPECT_EQ(ERR_OK,
              replicated_service_app_type_1::app_prepare_get_checkpoint(
                  &app, nullptr, 0, &occupied));
    EXPECT_EQ(ERR_INVALID_PARAMETERS,
              replicated_service_app_type_1::app_get_checkpoint(
                  &app, 0, 0, nullptr, 0, &state, 0));
    EXPECT_EQ(ERR_INVALID_PARAMETERS,
              replicated_service_app_type_1::app_apply_checkpoint(
                  &app, DSN_CHKPT_LEARN, 0, nullptr));

    replicated_service_app_type_1::app_learn_state cpp_state;
    EXPECT_EQ(ERR_INVALID_PARAMETERS, cpp_state.to_c_state(state, -1));
    EXPECT_EQ(ERR_INVALID_PARAMETERS, cpp_state.to_c_state(state, 0));

    EXPECT_FALSE(register_app_with_type_1_replication_support<test_replicated_service_app_for_invalid>(nullptr));
    EXPECT_FALSE(register_app_with_type_1_replication_support<test_replicated_service_app_for_invalid>(""));
}
