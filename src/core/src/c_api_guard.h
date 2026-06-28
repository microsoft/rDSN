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
 *     no-throw boundary guard for the rDSN C API (DSN_API) entry points.
 *
 *     The rDSN public API is a C ABI. A C++ exception that unwinds across the
 *     C ABI boundary is undefined behavior, so every DSN_API entry point that
 *     may invoke throwing C++ code (allocation, STL containers, serialization,
 *     calls into the engines) must translate an escaping exception into the
 *     API's documented failure value (dsn_error_t / bool / nullptr / 0)
 *     instead of letting it propagate.
 *
 *     Usage:
 *
 *         DSN_API dsn_message_t dsn_msg_create_request(...)
 *         {
 *             DSN_C_GUARD_BEGIN
 *             ... body that may throw ...
 *             return msg;
 *             DSN_C_GUARD_END(nullptr)   // fail value for this API
 *         }
 *
 *         DSN_API void dsn_some_void_api(...)
 *         {
 *             DSN_C_GUARD_BEGIN
 *             ... body that may throw ...
 *             DSN_C_GUARD_END_VOID()
 *         }
 *
 *     Two catch clauses are intentional: catch(const std::exception&) reports
 *     the diagnostic via ex.what(); catch(...) is still required because the C
 *     ABI boundary must also stop non-std exceptions (e.g. thrown ints, or
 *     foreign exception objects) from unwinding into the C caller.
 *
 *     This guard does NOT catch dsn_coredump()/abort() (e.g. from dassert):
 *     those are deliberate fatal-invariant aborts, not exceptions, and are
 *     handled separately by converting recoverable aborts into error returns.
 *
 * Revision history:
 *     2026-xx-xx, first version
 */

# pragma once

# include <exception>
# include <dsn/c/api_utilities.h>

// open the guarded region; pair with DSN_C_GUARD_END / DSN_C_GUARD_END_VOID
# define DSN_C_GUARD_BEGIN try {

// close the guarded region for an API that returns a value; on an escaping
// C++ exception, logs the diagnostic and returns (fail_ret). The trailing
// return after the handlers is intentional: it makes (fail_ret) the function's
// last statement so -Wreturn-type (-Werror) never fires regardless of how the
// guarded body is structured, and it is the value returned on the exception
// path.
# define DSN_C_GUARD_END(fail_ret)                                              \
    }                                                                           \
    catch (const std::exception &ex)                                            \
    {                                                                           \
        derror("%s: C API call aborted by C++ exception: %s",                   \
               __FUNCTION__, ex.what());                                        \
    }                                                                           \
    catch (...)                                                                 \
    {                                                                           \
        derror("%s: C API call aborted by unknown (non-std) C++ exception",     \
               __FUNCTION__);                                                   \
    }                                                                           \
    return (fail_ret);

// close the guarded region for an API that returns void; on an escaping C++
// exception, logs the diagnostic and returns normally.
# define DSN_C_GUARD_END_VOID()                                                 \
    }                                                                           \
    catch (const std::exception &ex)                                            \
    {                                                                           \
        derror("%s: C API call aborted by C++ exception: %s",                   \
               __FUNCTION__, ex.what());                                        \
    }                                                                           \
    catch (...)                                                                 \
    {                                                                           \
        derror("%s: C API call aborted by unknown (non-std) C++ exception",     \
               __FUNCTION__);                                                   \
    }
