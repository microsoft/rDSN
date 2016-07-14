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
 *     service core initialization
 *
 * Revision history:
 *     July, 2015, @imzhenyu (Zhenyu Guo), first version
 *     xxxx-xx-xx, author, fix bug about xxx
 */


# include <dsn/service_api_c.h>
# include <dsn/utility/ports.h>

# include <dsn/tool/simulator.h>
# include <dsn/tool/nativerun.h>
# include <dsn/tool/fastrun.h>
# include <dsn/toollet/tracer.h>
# include <dsn/toollet/profiler.h>
# include <dsn/toollet/fault_injector.h>
# include <dsn/toollet/explorer.h>

# include <dsn/tool/providers.common.h>
# include <dsn/tool/providers.hpc.h>
# include <dsn/tool/nfs.h>
# include <dsn/utility/singleton.h>

# include <dsn/dist/dist.providers.common.h>

//# include <dsn/thrift_helper.h>

# ifdef __TITLE__
# undef __TITLE__
# endif
# define __TITLE__ "core.main"

void dsn_core_init()
{
}
