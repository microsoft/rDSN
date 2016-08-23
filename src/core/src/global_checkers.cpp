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
 
# include <dsn/tool-api/global_checkers.h>
# include <dsn/utility/singleton.h>
# include <dsn/tool_api.h>

namespace dsn 
{
    class global_checker_store : public ::dsn::utils::singleton< global_checker_store >
    {
    public:
        safe_vector<global_checker> checkers;
    };

    void get_registered_checkers(/*out*/ safe_vector<global_checker>& checkers)
    {
        checkers = ::dsn::global_checker_store::instance().checkers;
    }
}


DSN_API void dsn_register_app_checker(const char* name, dsn_checker_create create, dsn_checker_apply apply)
{
    ::dsn::global_checker ck;
    ck.name = name;
    ck.create = create;
    ck.apply = apply;

    ::dsn::global_checker_store::instance().checkers.push_back(ck);
}

