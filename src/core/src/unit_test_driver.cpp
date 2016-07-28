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

# include <dsn/service_api_c.h>
# include <gtest/gtest.h>
# include <dsn/cpp/utils.h>
# include <dsn/cpp/test_utils.h>
# include <dsn/utility/singleton_store.h>
# include <fstream>

extern void task_engine_module_init();
extern void command_manager_module_init();

void run_all_unit_tests_prepare_when_necessary()
{
    // check gtest enabled or not
    if (!dsn_config_get_value_bool("core", "gtest", false,
        "runs all gtest cases or not, default is false"))
        return;

    // register test-required apps etc.
    // register all tools
    task_engine_module_init();
    command_manager_module_init();

    // register all possible services
    dsn::register_app<test_client>("test");
}

void run_all_unit_tests_when_necessary()
{
    if (!dsn_config_get_value_bool("core", "gtest", false,
        "runs all gtest cases or not, default is false"))
        return;
    
    auto params = dsn_config_get_value_string("core", "gtest_arguments", "", 
        "arguments passed to initGoogleTest"
    );

    std::vector<std::string> args;
    std::vector<const char*> args_ptr;

    ::dsn::utils::split_args(params, args);

    int ret = 0;        
    int argc = (int)args.size() + 1;
    char** argv = nullptr;

    args_ptr.push_back("rdsn.gtest");
    if (argc > 1)
    {        
        for (auto& arg : args)
        {
            args_ptr.push_back(arg.c_str());
        }
    }
    argv = (char**)&args_ptr[0];

    testing::InitGoogleTest(&argc, argv);
    ret = RUN_ALL_TESTS();

#ifndef ENABLE_GCOV
    dsn_exit(ret);
#else
    dsn_exit(0);
#endif

    dassert(false, "impossible execution here");
}

