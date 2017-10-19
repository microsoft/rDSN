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


# include "daemon.server.h"
# include <dsn/cpp/utils.h>
# include <gtest/gtest.h>

using namespace ::dsn;
using namespace ::dsn::replication;

DEFINE_TASK_CODE_RPC(RPC_DAEMON_TEST, TASK_PRIORITY_COMMON, ::dsn::THREAD_POOL_DEFAULT)

class daemon_test_server : 
    public ::dsn::service_app,
    public ::dsn::serverlet<daemon_test_server>
{
public:
    daemon_test_server(dsn_gpid gpid)
        : service_app(gpid),
        serverlet<daemon_test_server>("daemon.test.server")
    {
    }

    ::dsn::error_code start(int argc, char** argv) override
    {
        register_rpc_handler(RPC_DAEMON_TEST, "rpc.daemon.test", &daemon_test_server::on_test);
        return ::dsn::ERR_OK;
    }

    ::dsn::error_code stop(bool cleanup = false) override
    {
        return ::dsn::ERR_OK;
    }

    void on_test(const std::string& req, /*out*/ std::string& resp)
    {
        resp = req;
    }
};

void daemon_register_test_server()
{
    dsn::register_app< daemon_test_server>("daemon.test.server");
}

void create_test_server()
{
    rpc_address meta_server("localhost", 34601);

    configuration_create_app_request req;
    req.app_name = "myservice";
    req.options.app_type = "mysvc-v1";
    req.options.is_stateful = false;
    req.options.partition_count = 2;
    req.options.replica_count = 2;
    req.options.success_if_exist = true;

    auto resp = rpc::call_wait<configuration_create_app_response>(
        meta_server,
        RPC_CM_CREATE_APP,
        req,
        std::chrono::seconds(5)
        );

    ASSERT_EQ(ERR_OK, resp.first);
    ASSERT_EQ(ERR_OK, resp.second.err);
}

void kill_test_server()
{
    rpc_address meta_server("localhost", 34601);

    configuration_drop_app_request req;
    req.app_name = "myservice";
    req.options.success_if_not_exist = false;

    auto resp = rpc::call_wait<configuration_drop_app_response>(
        meta_server,
        RPC_CM_DROP_APP,
        req,
        std::chrono::seconds(5)
        );

    ASSERT_EQ(ERR_OK, resp.first);
    ASSERT_EQ(ERR_OK, resp.second.err);
}

TEST(daemon, test_server_response)
{
    // start service
    create_test_server();

    // use service
    url_host_address server("dsn://mycluster/myservice");
    std::string req = "hi, server";
    error_code last_err;
    std::string last_resp;

    for (int i = 0; i < 10; i++)
    {
        auto resp = rpc::call_wait<std::string>(
            server,
            RPC_DAEMON_TEST,
            req,
            std::chrono::seconds(5)
            );

        last_err = resp.first;
        last_resp = resp.second;

        printf("%d-th: err = %s, resp = %s\n",
            i,
            last_err.to_string(),
            last_resp.c_str()
            );
    }

    ASSERT_EQ(ERR_OK, last_err);
    ASSERT_EQ(req, last_resp);
    
    // kill service
    kill_test_server();
}

