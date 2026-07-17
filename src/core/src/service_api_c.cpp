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
# include <dsn/tool_api.h>
# include <dsn/utility/enum_helper.h>
# include <dsn/cpp/auto_codes.h>
# include <dsn/cpp/serialization.h>
# include <dsn/tool-api/task_spec.h>
# include <dsn/tool-api/zlock_provider.h>
# include <dsn/tool-api/nfs.h>
# include <dsn/tool-api/env_provider.h>
# include <dsn/utility/factory_store.h>
# include <dsn/tool-api/task.h>
# include <dsn/utility/singleton_store.h>
# include <dsn/cpp/utils.h> 

# include <dsn/utility/configuration.h>
# include "command_manager.h"
# include "service_engine.h"
# include "rpc_engine.h"
# include "disk_engine.h"
# include "task_engine.h"
# include "coredump.h"
# include "crc.h"
# include "transient_memory.h"
# include "library_utils.h"
# include "c_api_guard.h"
# include <fstream>
# include <cstring>
# include <limits>

# if defined(_WIN32)
# include <tlhelp32.h>
# else
# include <signal.h>
# include <unistd.h>
# endif

# if defined(__TITLE__)
# undef __TITLE__
# endif
# define __TITLE__ "service_api_c"

//------------------------------------------------------------------------------
//
// common types
//
//------------------------------------------------------------------------------
struct dsn_error_placeholder {};
class error_code_mgr : public ::dsn::utils::customized_id_mgr < dsn_error_placeholder >
{
public:
    error_code_mgr()
    {
        auto err = register_id("ERR_OK"); // make sure ERR_OK is always registered first
        dassert(0 == err, "");
    }
};

DSN_API dsn_error_t dsn_error_register(const char* name)
{
    if (name == nullptr || name[0] == '\0')
    {
        derror("dsn_error_register got null or empty name");
        return ::dsn::ERR_INVALID_PARAMETERS.get();
    }

    if (strlen(name) >= DSN_MAX_ERROR_CODE_NAME_LENGTH)
    {
        derror("dsn_error_register got too long name '%s' - length must be smaller than %d",
               name,
               DSN_MAX_ERROR_CODE_NAME_LENGTH);
        return ::dsn::ERR_INVALID_PARAMETERS.get();
    }

    return static_cast<dsn_error_t>(error_code_mgr::instance().register_id(name));
}

DSN_API const char* dsn_error_to_string(dsn_error_t err)
{
    if (err < 0)
    {
        derror("dsn_error_to_string got invalid error code = %d", err);
        return "unknown";
    }

    return error_code_mgr::instance().get_name(static_cast<int>(err));
}

DSN_API dsn_error_t dsn_error_from_string(const char* s, dsn_error_t default_err)
{
    DSN_C_GUARD_BEGIN
    if (s == nullptr || s[0] == '\0')
    {
        derror("dsn_error_from_string got null or empty string");
        return ::dsn::ERR_INVALID_PARAMETERS.get();
    }

    auto r = error_code_mgr::instance().get_id(s);
    return r == -1 ? default_err : r;
    DSN_C_GUARD_END(default_err)
}

DSN_API volatile int* dsn_task_queue_virtual_length_ptr(
    dsn_task_code_t code,
    int hash
    )
{
    DSN_C_GUARD_BEGIN
    if (code < 0 || code == ::dsn::TASK_CODE_INVALID)
    {
        derror("dsn_task_queue_virtual_length_ptr got invalid code = %d", code);
        return nullptr;
    }

    auto node = dsn::task::get_current_node();
    if (node == nullptr)
    {
        derror("dsn_task_queue_virtual_length_ptr got null current node");
        return nullptr;
    }

    return node->computation()->get_task_queue_virtual_length_ptr(code, hash);
    DSN_C_GUARD_END(nullptr)
}

// use ::dsn::threadpool_code2; for parsing purpose
DSN_API dsn_threadpool_code_t dsn_threadpool_code_register(const char* name)
{
    if (name == nullptr || name[0] == '\0')
    {
        derror("dsn_threadpool_code_register got null or empty name");
        return (dsn_threadpool_code_t)::dsn::THREAD_POOL_INVALID;
    }

    return static_cast<dsn_threadpool_code_t>(
        ::dsn::utils::customized_id_mgr< ::dsn::threadpool_code2_>::instance().register_id(name)
        );
}

DSN_API const char* dsn_threadpool_code_to_string(dsn_threadpool_code_t pool_code)
{
    return ::dsn::utils::customized_id_mgr< ::dsn::threadpool_code2_>::instance().get_name(static_cast<int>(pool_code));
}

DSN_API dsn_threadpool_code_t dsn_threadpool_code_from_string(const char* s, dsn_threadpool_code_t default_code)
{
    DSN_C_GUARD_BEGIN
    if (s == nullptr || s[0] == '\0')
    {
        derror("dsn_threadpool_code_from_string got null or empty string");
        return static_cast<dsn_threadpool_code_t>(::dsn::THREAD_POOL_INVALID);
    }

    auto r = ::dsn::utils::customized_id_mgr< ::dsn::threadpool_code2_>::instance().get_id(s);
    return r == -1 ? default_code : r;
    DSN_C_GUARD_END(default_code)
}

DSN_API int dsn_threadpool_code_max()
{
    return ::dsn::utils::customized_id_mgr< ::dsn::threadpool_code2_>::instance().max_value();
}

DSN_API int dsn_threadpool_get_current_tid()
{
    return ::dsn::utils::get_current_tid();
}

struct task_code_placeholder { };
DSN_API dsn_task_code_t dsn_task_code_register(
    const char* name, 
    dsn_task_type_t type,
    dsn_task_priority_t pri,
    dsn_threadpool_code_t pool
    )
{
    if (name == nullptr || name[0] == '\0')
    {
        derror("dsn_task_code_register got null or empty name");
        return static_cast<dsn_task_code_t>(::dsn::TASK_CODE_INVALID);
    }

    if (strlen(name) >= DSN_MAX_TASK_CODE_NAME_LENGTH)
    {
        derror("dsn_task_code_register got too long name '%s' - length must be smaller than %d",
               name,
               DSN_MAX_TASK_CODE_NAME_LENGTH);
        return static_cast<dsn_task_code_t>(::dsn::TASK_CODE_INVALID);
    }

    if (type < TASK_TYPE_RPC_REQUEST || type >= TASK_TYPE_COUNT)
    {
        derror("dsn_task_code_register got invalid type = %d", type);
        return static_cast<dsn_task_code_t>(::dsn::TASK_CODE_INVALID);
    }

    if (pri < TASK_PRIORITY_LOW || pri >= TASK_PRIORITY_COUNT)
    {
        derror("dsn_task_code_register got invalid priority = %d", pri);
        return static_cast<dsn_task_code_t>(::dsn::TASK_CODE_INVALID);
    }

    if (pool < 0 || pool == ::dsn::THREAD_POOL_INVALID)
    {
        derror("dsn_task_code_register got invalid pool = %d", pool);
        return static_cast<dsn_task_code_t>(::dsn::TASK_CODE_INVALID);
    }

    auto r = static_cast<dsn_task_code_t>(
        ::dsn::utils::customized_id_mgr<task_code_placeholder>::instance().register_id(name));
    if (!::dsn::task_spec::register_task_code(r, type, pri, pool))
    {
        return static_cast<dsn_task_code_t>(::dsn::TASK_CODE_INVALID);
    }
    return r;
}

DSN_API void dsn_task_code_query(dsn_task_code_t code, dsn_task_type_t *ptype, dsn_task_priority_t *ppri, dsn_threadpool_code_t *ppool)
{
    auto sp = ::dsn::task_spec::get(code);
    if (code == ::dsn::TASK_CODE_INVALID || sp == nullptr)
    {
        derror("dsn_task_code_query got invalid code = %d", code);
        return;
    }

    if (ptype) *ptype = sp->type;
    if (ppri) *ppri = sp->priority;
    if (ppool) *ppool = sp->pool_code;
}

DSN_API void dsn_task_code_set_threadpool(dsn_task_code_t code, dsn_threadpool_code_t pool)
{
    auto sp = ::dsn::task_spec::get(code);
    if (code == ::dsn::TASK_CODE_INVALID || sp == nullptr)
    {
        derror("dsn_task_code_set_threadpool got invalid code = %d", code);
        return;
    }

    if (pool < 0 || pool == ::dsn::THREAD_POOL_INVALID)
    {
        derror("dsn_task_code_set_threadpool got invalid pool = %d", pool);
        return;
    }

    sp->pool_code = pool;
}

DSN_API void dsn_task_code_set_priority(dsn_task_code_t code, dsn_task_priority_t pri)
{
    auto sp = ::dsn::task_spec::get(code);
    if (code == ::dsn::TASK_CODE_INVALID || sp == nullptr)
    {
        derror("dsn_task_code_set_priority got invalid code = %d", code);
        return;
    }

    if (pri < TASK_PRIORITY_LOW || pri >= TASK_PRIORITY_COUNT)
    {
        derror("dsn_task_code_set_priority got invalid priority = %d", pri);
        return;
    }

    sp->priority = pri;
}

DSN_API const char* dsn_task_code_to_string(dsn_task_code_t code)
{
    if (code < 0)
    {
        derror("dsn_task_code_to_string got invalid code = %d", code);
        return "unknown";
    }

    return ::dsn::utils::customized_id_mgr<task_code_placeholder>::instance().get_name(static_cast<int>(code));
}

DSN_API dsn_task_code_t dsn_task_code_from_string(const char* s, dsn_task_code_t default_code)
{
    DSN_C_GUARD_BEGIN
    if (s == nullptr || s[0] == '\0')
    {
        derror("dsn_task_code_from_string got null or empty string");
        return static_cast<dsn_task_code_t>(::dsn::TASK_CODE_INVALID);
    }

    auto r = ::dsn::utils::customized_id_mgr<task_code_placeholder>::instance().get_id(s);
    return r == -1 ? default_code : r;
    DSN_C_GUARD_END(default_code)
}

DSN_API int dsn_task_code_max()
{
    return ::dsn::utils::customized_id_mgr<task_code_placeholder>::instance().max_value();
}

DSN_API const char* dsn_task_type_to_string(dsn_task_type_t tt)
{
    if (tt < TASK_TYPE_RPC_REQUEST || tt >= TASK_TYPE_COUNT)
    {
        derror("dsn_task_type_to_string got invalid task type = %d", tt);
        return "Unknown";
    }

    return enum_to_string(tt);
}

DSN_API const char* dsn_task_priority_to_string(dsn_task_priority_t tt)
{
    if (tt < TASK_PRIORITY_LOW || tt >= TASK_PRIORITY_COUNT)
    {
        derror("dsn_task_priority_to_string got invalid task priority = %d", tt);
        return "Unknown";
    }

    return enum_to_string(tt);
}

DSN_API bool dsn_task_is_running_inside(dsn_task_t t)
{
    if (t == nullptr)
    {
        derror("dsn_task_is_running_inside got null task");
        return false;
    }

    return ::dsn::task::get_current_task() == (::dsn::task*)t;
}

DSN_API void dsn_coredump()
{
    ::dsn::utils::coredump::write(); 
    ::abort();
}

DSN_API uint32_t dsn_crc32_compute(const void* ptr, size_t size, uint32_t init_crc)
{
    if (ptr == nullptr)
    {
        derror("dsn_crc32_compute got null ptr");
        return CRC_INVALID;
    }

    return ::dsn::utils::crc32::compute(ptr, size, init_crc);
}

DSN_API uint32_t dsn_crc32_concatenate(uint32_t xy_init, uint32_t x_init, uint32_t x_final, size_t x_size, uint32_t y_init, uint32_t y_final, size_t y_size)
{
    return ::dsn::utils::crc32::concatenate(
        0,
        x_init, x_final, (uint64_t)x_size,
        y_init, y_final, (uint64_t)y_size
        );
}


DSN_API uint64_t dsn_crc64_compute(const void* ptr, size_t size, uint64_t init_crc)
{
    if (ptr == nullptr)
    {
        derror("dsn_crc64_compute got null ptr");
        return CRC_INVALID;
    }

    return ::dsn::utils::crc64::compute(ptr, size, init_crc);
}

DSN_API uint64_t dsn_crc64_concatenate(uint32_t xy_init, uint64_t x_init, uint64_t x_final, size_t x_size, uint64_t y_init, uint64_t y_final, size_t y_size)
{
    return ::dsn::utils::crc64::concatenate(
        0,
        x_init, x_final, (uint64_t)x_size,
        y_init, y_final, (uint64_t)y_size
        );
}

DSN_API dsn_task_t dsn_task_create(dsn_task_code_t code, dsn_task_handler_t cb, void* context, int hash, dsn_task_tracker_t tracker)
{
    DSN_C_GUARD_BEGIN
    auto sp = ::dsn::task_spec::get(code);
    if (code == ::dsn::TASK_CODE_INVALID || sp == nullptr || sp->type != TASK_TYPE_COMPUTE)
    {
        derror("dsn_task_create got invalid code = %d", code);
        return nullptr;
    }

    if (cb == nullptr)
    {
        derror("dsn_task_create got null callback");
        return nullptr;
    }

    auto node = ::dsn::task::get_current_node();
    auto t = new ::dsn::task_c(code, cb, context, nullptr, hash, node);
    t->set_tracker((dsn::task_tracker*)tracker);
    t->spec().on_task_create.execute(::dsn::task::get_current_task(), t);
    return t;
    DSN_C_GUARD_END(nullptr)
}

DSN_API dsn_task_t dsn_task_create_timer(dsn_task_code_t code, dsn_task_handler_t cb, 
    void* context, int hash, int interval_milliseconds, dsn_task_tracker_t tracker)
{
    DSN_C_GUARD_BEGIN
    auto sp = ::dsn::task_spec::get(code);
    if (code == ::dsn::TASK_CODE_INVALID || sp == nullptr || sp->type != TASK_TYPE_COMPUTE)
    {
        derror("dsn_task_create_timer got invalid code = %d", code);
        return nullptr;
    }

    if (cb == nullptr)
    {
        derror("dsn_task_create_timer got null callback");
        return nullptr;
    }

    if (interval_milliseconds < 0)
    {
        derror("dsn_task_create_timer got invalid interval_milliseconds = %d", interval_milliseconds);
        return nullptr;
    }

    auto t = new ::dsn::timer_task(code, cb, context, nullptr, interval_milliseconds, hash);
    t->set_tracker((dsn::task_tracker*)tracker);
    t->spec().on_task_create.execute(::dsn::task::get_current_task(), t);
    return t;
    DSN_C_GUARD_END(nullptr)
}

DSN_API dsn_task_t dsn_task_create_ex(dsn_task_code_t code, dsn_task_handler_t cb, 
    dsn_task_cancelled_handler_t on_cancel, void* context, int hash, dsn_task_tracker_t tracker)
{
    DSN_C_GUARD_BEGIN
    auto sp = ::dsn::task_spec::get(code);
    if (code == ::dsn::TASK_CODE_INVALID || sp == nullptr || sp->type != TASK_TYPE_COMPUTE)
    {
        derror("dsn_task_create_ex got invalid code = %d", code);
        return nullptr;
    }

    if (cb == nullptr)
    {
        derror("dsn_task_create_ex got null callback");
        return nullptr;
    }

    auto t = new ::dsn::task_c(code, cb, context, on_cancel, hash);
    t->set_tracker((dsn::task_tracker*)tracker);
    t->spec().on_task_create.execute(::dsn::task::get_current_task(), t);
    return t;
    DSN_C_GUARD_END(nullptr)
}

DSN_API dsn_task_t dsn_task_create_timer_ex(dsn_task_code_t code, dsn_task_handler_t cb,
    dsn_task_cancelled_handler_t on_cancel, 
    void* context, int hash, int interval_milliseconds, dsn_task_tracker_t tracker)
{
    DSN_C_GUARD_BEGIN
    auto sp = ::dsn::task_spec::get(code);
    if (code == ::dsn::TASK_CODE_INVALID || sp == nullptr || sp->type != TASK_TYPE_COMPUTE)
    {
        derror("dsn_task_create_timer_ex got invalid code = %d", code);
        return nullptr;
    }

    if (cb == nullptr)
    {
        derror("dsn_task_create_timer_ex got null callback");
        return nullptr;
    }

    if (interval_milliseconds < 0)
    {
        derror("dsn_task_create_timer_ex got invalid interval_milliseconds = %d",
               interval_milliseconds);
        return nullptr;
    }

    auto t = new ::dsn::timer_task(code, cb, context, on_cancel, interval_milliseconds, hash);
    t->set_tracker((dsn::task_tracker*)tracker);
    t->spec().on_task_create.execute(::dsn::task::get_current_task(), t);
    return t;
    DSN_C_GUARD_END(nullptr)
}

DSN_API dsn_task_tracker_t dsn_task_tracker_create(int task_bucket_count)
{
    DSN_C_GUARD_BEGIN
    if (task_bucket_count <= 0)
    {
        derror("dsn_task_tracker_create got invalid task_bucket_count = %d", task_bucket_count);
        return nullptr;
    }

    return (dsn_task_tracker_t)(new ::dsn::task_tracker(task_bucket_count));
    DSN_C_GUARD_END(nullptr)
}

DSN_API void dsn_task_tracker_destroy(dsn_task_tracker_t tracker)
{
    if (tracker == nullptr)
    {
        derror("dsn_task_tracker_destroy got null tracker");
        return;
    }

    delete ((::dsn::task_tracker*)tracker);
}

DSN_API void dsn_task_tracker_cancel_all(dsn_task_tracker_t tracker)
{
    if (tracker == nullptr)
    {
        derror("dsn_task_tracker_cancel_all got null tracker");
        return;
    }

    ((::dsn::task_tracker*)tracker)->cancel_outstanding_tasks();
}

DSN_API void dsn_task_tracker_wait_all(dsn_task_tracker_t tracker)
{
    if (tracker == nullptr)
    {
        derror("dsn_task_tracker_wait_all got null tracker");
        return;
    }

    ((::dsn::task_tracker*)tracker)->wait_outstanding_tasks();
}

DSN_API bool dsn_task_call(dsn_task_t task, int delay_milliseconds)
{
    DSN_C_GUARD_BEGIN
    if (task == nullptr)
    {
        derror("dsn_task_call got null task");
        return false;
    }

    if (delay_milliseconds < 0)
    {
        derror("dsn_task_call got invalid delay_milliseconds = %d", delay_milliseconds);
        return false;
    }

    auto t = (::dsn::task*)task;
    if (t->spec().type != TASK_TYPE_COMPUTE)
    {
        derror("dsn_task_call got non-compute task");
        return false;
    }

    t->set_delay(delay_milliseconds);
    t->enqueue();
    return true;
    DSN_C_GUARD_END(false)
}

DSN_API void dsn_task_add_ref(dsn_task_t task)
{
    if (task == nullptr)
    {
        derror("dsn_task_add_ref got null task");
        return;
    }

    ((::dsn::task*)(task))->add_ref();
}

DSN_API void dsn_task_release_ref(dsn_task_t task)
{
    if (task == nullptr)
    {
        derror("dsn_task_release_ref got null task");
        return;
    }

    ((::dsn::task*)(task))->release_ref();
}

DSN_API int dsn_task_get_ref(dsn_task_t task)
{
    if (task == nullptr)
    {
        derror("dsn_task_get_ref got null task");
        return 0;
    }

    return ((::dsn::task*)(task))->get_count();
}

DSN_API bool dsn_task_cancel(dsn_task_t task, bool wait_until_finished)
{
    if (task == nullptr)
    {
        derror("dsn_task_cancel got null task");
        return false;
    }

    return ((::dsn::task*)(task))->cancel(wait_until_finished);
}

DSN_API bool dsn_task_set_delay(dsn_task_t task, int delay_ms)
{
    if (task == nullptr)
    {
        derror("dsn_task_set_delay got null task");
        return false;
    }

    if (delay_ms < 0)
    {
        derror("dsn_task_set_delay got invalid delay_ms = %d", delay_ms);
        return false;
    }

    ((::dsn::task*)(task))->set_delay(delay_ms);
    return true;
}

DSN_API bool dsn_task_cancel2(dsn_task_t task, bool wait_until_finished, bool* finished)
{
    if (task == nullptr)
    {
        derror("dsn_task_cancel2 got null task");
        if (finished != nullptr)
        {
            *finished = false;
        }
        return false;
    }

    return ((::dsn::task*)(task))->cancel(wait_until_finished, finished);
}

DSN_API void dsn_task_cancel_current_timer()
{
    ::dsn::task* t = ::dsn::task::get_current_task();
    if (nullptr != t)
    {
        t->cancel(false);
    }
}

DSN_API void dsn_task_wait(dsn_task_t task)
{
    if (task == nullptr)
    {
        derror("dsn_task_wait got null task");
        return;
    }

    auto t = (::dsn::task*)task;
    auto r = t->wait();
    // dsn_task_wait returns void, so it has no channel to report a failed wait;
    // per design it keeps the dassert. For an infinite (no-timeout) wait the
    // only way r is false is a self-wait, which task::wait already logged.
    dassert(r,
        "dsn_task_wait failed: a task must not wait on itself (task_id = %016" PRIx64 ")",
        t->id()
        );
}

DSN_API bool dsn_task_wait_timeout(dsn_task_t task, int timeout_milliseconds)
{
    if (task == nullptr)
    {
        derror("dsn_task_wait_timeout got null task");
        return false;
    }

    return ((::dsn::task*)(task))->wait(timeout_milliseconds);
}

DSN_API dsn_error_t dsn_task_error(dsn_task_t task)
{
    if (task == nullptr)
    {
        derror("dsn_task_error got null task");
        return ::dsn::ERR_INVALID_PARAMETERS.get();
    }

    return ((::dsn::task*)(task))->error().get();
}

//------------------------------------------------------------------------------
//
// synchronization - concurrent access and coordination among threads
//
//------------------------------------------------------------------------------
namespace {

template <typename TProvider, typename TAspects>
::dsn::ilock* create_lock_chain(const char* api_name,
                                const char* factory_name,
                                const TAspects& aspects)
{
    TProvider* last = ::dsn::utils::factory_store<TProvider>::create(
        factory_name, ::dsn::PROVIDER_TYPE_MAIN, nullptr);
    if (last == nullptr)
    {
        derror("%s got null provider factory result for '%s'", api_name, factory_name);
        return nullptr;
    }

    // TODO: perf opt by saving the func ptrs somewhere
    for (auto& s : aspects)
    {
        TProvider* next = ::dsn::utils::factory_store<TProvider>::create(
            s.c_str(), ::dsn::PROVIDER_TYPE_ASPECT, last);
        if (next == nullptr)
        {
            derror("%s got null aspect factory result for '%s'", api_name, s.c_str());
            // Provider destructors own their inner_provider, so deleting the head
            // releases the whole chain built so far.
            delete last;
            return nullptr;
        }
        last = next;
    }

    return static_cast< ::dsn::ilock*>(last);
}

} // anonymous namespace

DSN_API dsn_handle_t dsn_exlock_create(bool recursive)
{
    DSN_C_GUARD_BEGIN
    const auto& spec = ::dsn::service_engine::fast_instance().spec();
    return recursive
        ? (dsn_handle_t)create_lock_chain< ::dsn::lock_provider>(
            __FUNCTION__, spec.lock_factory_name.c_str(), spec.lock_aspects)
        : (dsn_handle_t)create_lock_chain< ::dsn::lock_nr_provider>(
            __FUNCTION__, spec.lock_nr_factory_name.c_str(), spec.lock_nr_aspects);
    DSN_C_GUARD_END(nullptr)
}

DSN_API void dsn_exlock_destroy(dsn_handle_t l)
{
    if (l == nullptr)
    {
        derror("dsn_exlock_destroy got null lock");
        return;
    }

    delete (::dsn::ilock*)(l);
}

DSN_API void dsn_exlock_lock(dsn_handle_t l)
{
    if (l == nullptr)
    {
        derror("dsn_exlock_lock got null lock");
        return;
    }

    ((::dsn::ilock*)(l))->lock();
    ::dsn::lock_checker::zlock_exclusive_count++;
}

DSN_API bool dsn_exlock_try_lock(dsn_handle_t l)
{
    if (l == nullptr)
    {
        derror("dsn_exlock_try_lock got null lock");
        return false;
    }

    auto r = ((::dsn::ilock*)(l))->try_lock();
    if (r)
    {
        ::dsn::lock_checker::zlock_exclusive_count++;
    }
    return r;
}

DSN_API void dsn_exlock_unlock(dsn_handle_t l)
{
    if (l == nullptr)
    {
        derror("dsn_exlock_unlock got null lock");
        return;
    }

    ::dsn::lock_checker::zlock_exclusive_count--;
    ((::dsn::ilock*)(l))->unlock();
}

// non-recursive rwlock
DSN_API dsn_handle_t dsn_rwlock_nr_create()
{
    DSN_C_GUARD_BEGIN
    const auto& spec = ::dsn::service_engine::fast_instance().spec();
    ::dsn::rwlock_nr_provider* last = ::dsn::utils::factory_store< ::dsn::rwlock_nr_provider>::create(
        spec.rwlock_nr_factory_name.c_str(), ::dsn::PROVIDER_TYPE_MAIN, nullptr);
    if (last == nullptr)
    {
        derror("dsn_rwlock_nr_create got null provider factory result for '%s'",
               spec.rwlock_nr_factory_name.c_str());
        return nullptr;
    }

    // TODO: perf opt by saving the func ptrs somewhere
    for (auto& s : spec.rwlock_nr_aspects)
    {
        ::dsn::rwlock_nr_provider* next = ::dsn::utils::factory_store< ::dsn::rwlock_nr_provider>::create(
            s.c_str(), ::dsn::PROVIDER_TYPE_ASPECT, last);
        if (next == nullptr)
        {
            derror("dsn_rwlock_nr_create got null aspect factory result for '%s'", s.c_str());
            // Provider destructors own their inner_provider, so deleting the head
            // releases the whole chain built so far.
            delete last;
            return nullptr;
        }
        last = next;
    }
    return (dsn_handle_t)(last);
    DSN_C_GUARD_END(nullptr)
}

DSN_API void dsn_rwlock_nr_destroy(dsn_handle_t l)
{
    if (l == nullptr)
    {
        derror("dsn_rwlock_nr_destroy got null lock");
        return;
    }

    delete (::dsn::rwlock_nr_provider*)(l);
}

DSN_API void dsn_rwlock_nr_lock_read(dsn_handle_t l)
{
    if (l == nullptr)
    {
        derror("dsn_rwlock_nr_lock_read got null lock");
        return;
    }

    ((::dsn::rwlock_nr_provider*)(l))->lock_read();
    ::dsn::lock_checker::zlock_shared_count++;
}

DSN_API void dsn_rwlock_nr_unlock_read(dsn_handle_t l)
{
    if (l == nullptr)
    {
        derror("dsn_rwlock_nr_unlock_read got null lock");
        return;
    }

    ::dsn::lock_checker::zlock_shared_count--;
    ((::dsn::rwlock_nr_provider*)(l))->unlock_read();
}

DSN_API bool dsn_rwlock_nr_try_lock_read(dsn_handle_t l)
{
    if (l == nullptr)
    {
        derror("dsn_rwlock_nr_try_lock_read got null lock");
        return false;
    }

    auto r = ((::dsn::rwlock_nr_provider*)(l))->try_lock_read();
    if (r) ::dsn::lock_checker::zlock_shared_count++;
    return r;
}

DSN_API void dsn_rwlock_nr_lock_write(dsn_handle_t l)
{
    if (l == nullptr)
    {
        derror("dsn_rwlock_nr_lock_write got null lock");
        return;
    }

    ((::dsn::rwlock_nr_provider*)(l))->lock_write();
    ::dsn::lock_checker::zlock_exclusive_count++;
}

DSN_API void dsn_rwlock_nr_unlock_write(dsn_handle_t l)
{
    if (l == nullptr)
    {
        derror("dsn_rwlock_nr_unlock_write got null lock");
        return;
    }

    ::dsn::lock_checker::zlock_exclusive_count--;
    ((::dsn::rwlock_nr_provider*)(l))->unlock_write();
}

DSN_API bool dsn_rwlock_nr_try_lock_write(dsn_handle_t l)
{
    if (l == nullptr)
    {
        derror("dsn_rwlock_nr_try_lock_write got null lock");
        return false;
    }

    auto r = ((::dsn::rwlock_nr_provider*)(l))->try_lock_write();
    if (r) ::dsn::lock_checker::zlock_exclusive_count++;
    return r;
}

DSN_API dsn_handle_t dsn_semaphore_create(int initial_count)
{
    DSN_C_GUARD_BEGIN
    if (initial_count < 0)
    {
        derror("dsn_semaphore_create got invalid initial_count = %d", initial_count);
        return nullptr;
    }

    const auto& spec = ::dsn::service_engine::fast_instance().spec();
    ::dsn::semaphore_provider* last = ::dsn::utils::factory_store< ::dsn::semaphore_provider>::create(
        spec.semaphore_factory_name.c_str(), ::dsn::PROVIDER_TYPE_MAIN, initial_count, nullptr);
    if (last == nullptr)
    {
        derror("dsn_semaphore_create got null provider factory result for '%s'",
               spec.semaphore_factory_name.c_str());
        return nullptr;
    }

    // TODO: perf opt by saving the func ptrs somewhere
    for (auto& s : spec.semaphore_aspects)
    {
        ::dsn::semaphore_provider* next = ::dsn::utils::factory_store< ::dsn::semaphore_provider>::create(
            s.c_str(), ::dsn::PROVIDER_TYPE_ASPECT, initial_count, last);
        if (next == nullptr)
        {
            derror("dsn_semaphore_create got null aspect factory result for '%s'", s.c_str());
            // Provider destructors own their inner_provider, so deleting the head
            // releases the whole chain built so far.
            delete last;
            return nullptr;
        }
        last = next;
    }
    return (dsn_handle_t)(last);
    DSN_C_GUARD_END(nullptr)
}

DSN_API void dsn_semaphore_destroy(dsn_handle_t s)
{
    if (s == nullptr)
    {
        derror("dsn_semaphore_destroy got null semaphore");
        return;
    }

    delete (::dsn::semaphore_provider*)(s);
}

DSN_API void dsn_semaphore_signal(dsn_handle_t s, int count)
{
    if (s == nullptr)
    {
        derror("dsn_semaphore_signal got null semaphore");
        return;
    }

    if (count <= 0)
    {
        derror("dsn_semaphore_signal got invalid count = %d", count);
        return;
    }

    ((::dsn::semaphore_provider*)(s))->signal(count);
}

DSN_API void dsn_semaphore_wait(dsn_handle_t s)
{
    if (s == nullptr)
    {
        derror("dsn_semaphore_wait got null semaphore");
        return;
    }

    ::dsn::lock_checker::check_wait_safety();
    ((::dsn::semaphore_provider*)(s))->wait();
}

DSN_API bool dsn_semaphore_wait_timeout(dsn_handle_t s, int timeout_milliseconds)
{
    if (s == nullptr)
    {
        derror("dsn_semaphore_wait_timeout got null semaphore");
        return false;
    }

    return ((::dsn::semaphore_provider*)(s))->wait(timeout_milliseconds);
}

//------------------------------------------------------------------------------
//
// rpc
//
//------------------------------------------------------------------------------

// rpc calls
DSN_API dsn_address_t dsn_primary_address()
{
    auto rpc = ::dsn::task::get_current_rpc();
    if (rpc == nullptr)
    {
        derror("dsn_primary_address got null current rpc engine");
        return ::dsn::rpc_address().c_addr();
    }

    return rpc->primary_address().c_addr();
}

DSN_API bool dsn_rpc_register_handler(
    dsn_task_code_t code,
    const char* name, 
    dsn_rpc_request_handler_t cb, 
    void* param, 
    dsn_gpid gpid
    )
{
    DSN_C_GUARD_BEGIN
    auto sp = ::dsn::task_spec::get(code);
    if (code == ::dsn::TASK_CODE_INVALID || sp == nullptr || sp->type != TASK_TYPE_RPC_REQUEST)
    {
        derror("dsn_rpc_register_handler got invalid code = %d", code);
        return false;
    }

    if (name == nullptr || name[0] == '\0')
    {
        derror("dsn_rpc_register_handler got null or empty name");
        return false;
    }

    if (cb == nullptr)
    {
        derror("dsn_rpc_register_handler got null callback");
        return false;
    }

    ::dsn::rpc_handler_info* h(new ::dsn::rpc_handler_info(code));
    h->name = name;
    h->c_handler = cb;
    h->parameter = param;

    h->add_ref();

    bool r = ::dsn::task::get_current_node()->rpc_register_handler(h, gpid);
    if (!r)
    {
        delete h;
    }
    return r;
    DSN_C_GUARD_END(false)
}

DSN_API void* dsn_rpc_unregiser_handler(dsn_task_code_t code, dsn_gpid gpid)
{
    DSN_C_GUARD_BEGIN
    auto sp = ::dsn::task_spec::get(code);
    if (code == ::dsn::TASK_CODE_INVALID || sp == nullptr || sp->type != TASK_TYPE_RPC_REQUEST)
    {
        derror("dsn_rpc_unregiser_handler got invalid code = %d", code);
        return nullptr;
    }

    auto h = ::dsn::task::get_current_node()->rpc_unregister_handler(code, gpid);
    void* param = nullptr;

    if (nullptr != h)
    {
        param = h->parameter;
        if (1 == h->release_ref())
            delete h;
    }

    return param;
    DSN_C_GUARD_END(nullptr)
}

// Returns true iff `msg` can be used to construct an rpc_response_task: its rpc code is a registered
// TASK_TYPE_RPC_REQUEST with a valid paired TASK_TYPE_RPC_RESPONSE code. This mirrors the invariant
// asserted in rpc_response_task's constructor, letting the C API reject invalid caller input with an
// error return instead of tripping those asserts and aborting the process.
static bool is_valid_rpc_request_message(::dsn::message_ex* msg)
{
    if (msg == nullptr)
    {
        return false;
    }

    auto req_spec = ::dsn::task_spec::get(msg->local_rpc_code);
    if (req_spec == nullptr || req_spec->type != TASK_TYPE_RPC_REQUEST)
    {
        return false;
    }
    if (req_spec->rpc_paired_code == ::dsn::TASK_CODE_INVALID)
    {
        return false;
    }

    auto resp_spec = ::dsn::task_spec::get(req_spec->rpc_paired_code);
    return resp_spec != nullptr && resp_spec->type == TASK_TYPE_RPC_RESPONSE;
}

DSN_API dsn_task_t dsn_rpc_create_response_task(dsn_message_t request, dsn_rpc_response_handler_t cb, 
    void* context, int reply_thread_hash, dsn_task_tracker_t tracker)
{
    DSN_C_GUARD_BEGIN
    auto msg = ((::dsn::message_ex*)request);
    if (msg == nullptr)
    {
        derror("dsn_rpc_create_response_task got null request");
        return nullptr;
    }
    if (!is_valid_rpc_request_message(msg))
    {
        derror("dsn_rpc_create_response_task got an invalid request message: "
            "it is not a registered rpc request with a paired response code");
        return nullptr;
    }

    auto t = new ::dsn::rpc_response_task(msg, cb, context, nullptr, reply_thread_hash);
    t->set_tracker((dsn::task_tracker*)tracker);
    t->spec().on_task_create.execute(::dsn::task::get_current_task(), t);
    return t;
    DSN_C_GUARD_END(nullptr)
}

DSN_API dsn_task_t dsn_rpc_create_response_task_ex(dsn_message_t request, dsn_rpc_response_handler_t cb, 
    dsn_task_cancelled_handler_t on_cancel,
    void* context, int reply_thread_hash, dsn_task_tracker_t tracker)
{
    DSN_C_GUARD_BEGIN
    auto msg = ((::dsn::message_ex*)request);
    if (msg == nullptr)
    {
        derror("dsn_rpc_create_response_task_ex got null request");
        return nullptr;
    }
    if (!is_valid_rpc_request_message(msg))
    {
        derror("dsn_rpc_create_response_task_ex got an invalid request message: "
            "it is not a registered rpc request with a paired response code");
        return nullptr;
    }

    auto t = new ::dsn::rpc_response_task(msg, cb, context, on_cancel, reply_thread_hash);
    t->set_tracker((dsn::task_tracker*)tracker);
    t->spec().on_task_create.execute(::dsn::task::get_current_task(), t);
    return t;
    DSN_C_GUARD_END(nullptr)
}

DSN_API dsn_error_t dsn_rpc_call(dsn_address_t server, dsn_task_t rpc_call)
{
    DSN_C_GUARD_BEGIN
    ::dsn::rpc_response_task* task = (::dsn::rpc_response_task*)rpc_call;
    if (task == nullptr)
    {
        derror("dsn_rpc_call got null rpc_call");
        return ::dsn::ERR_INVALID_PARAMETERS.get();
    }

    if (task->spec().type != TASK_TYPE_RPC_RESPONSE)
    {
        derror("dsn_rpc_call got non-rpc-response task");
        return ::dsn::ERR_INVALID_PARAMETERS.get();
    }

    if (::dsn::rpc_address(server).is_invalid())
    {
        derror("dsn_rpc_call got invalid server address");
        return ::dsn::ERR_INVALID_PARAMETERS.get();
    }

    auto rpc = ::dsn::task::get_current_rpc();
    if (rpc == nullptr)
    {
        derror("dsn_rpc_call got null current rpc engine");
        return ::dsn::ERR_INVALID_STATE.get();
    }
    
    auto msg = task->get_request();
    if (msg == nullptr)
    {
        derror("dsn_rpc_call got null request message");
        return ::dsn::ERR_INVALID_PARAMETERS.get();
    }

    msg->server_address = server;
    rpc->call(msg, task);
    return ::dsn::ERR_OK.get();
    DSN_C_GUARD_END(::dsn::ERR_UNKNOWN.get())
}

DSN_API dsn_message_t dsn_rpc_call_wait(dsn_address_t server, dsn_message_t request)
{
    DSN_C_GUARD_BEGIN
    auto msg = ((::dsn::message_ex*)request);
    if (msg == nullptr)
    {
        derror("dsn_rpc_call_wait got null request");
        return nullptr;
    }
    if (!is_valid_rpc_request_message(msg))
    {
        derror("dsn_rpc_call_wait got an invalid request message: "
            "it is not a registered rpc request with a paired response code");
        return nullptr;
    }

    if (::dsn::rpc_address(server).is_invalid())
    {
        derror("dsn_rpc_call_wait got invalid server address");
        return nullptr;
    }

    auto rpc = ::dsn::task::get_current_rpc();
    if (rpc == nullptr)
    {
        derror("dsn_rpc_call_wait got null current rpc engine");
        return nullptr;
    }

    msg->server_address = server;

    ::dsn::rpc_response_task* rtask = new ::dsn::rpc_response_task(msg, nullptr, nullptr, 0);
    rtask->add_ref();
    rpc->call(msg, rtask);
    rtask->wait();
    if (rtask->error() == ::dsn::ERR_OK)
    {
        auto msg = rtask->get_response();
        msg->add_ref(); // released by callers
        rtask->release_ref(); // added above
        return msg;
    }
    else
    {
        rtask->release_ref(); // added above
        return nullptr;
    }
    DSN_C_GUARD_END(nullptr)
}

DSN_API dsn_error_t dsn_rpc_call_one_way(dsn_address_t server, dsn_message_t request)
{
    DSN_C_GUARD_BEGIN
    auto msg = ((::dsn::message_ex*)request);
    if (msg == nullptr)
    {
        derror("dsn_rpc_call_one_way got null request");
        return ::dsn::ERR_INVALID_PARAMETERS.get();
    }

    if (::dsn::rpc_address(server).is_invalid())
    {
        derror("dsn_rpc_call_one_way got invalid server address");
        return ::dsn::ERR_INVALID_PARAMETERS.get();
    }

    auto rpc = ::dsn::task::get_current_rpc();
    if (rpc == nullptr)
    {
        derror("dsn_rpc_call_one_way got null current rpc engine");
        return ::dsn::ERR_INVALID_STATE.get();
    }

    msg->server_address = server;

    rpc->call(msg, nullptr);
    return ::dsn::ERR_OK.get();
    DSN_C_GUARD_END(::dsn::ERR_UNKNOWN.get())
}

DSN_API dsn_error_t dsn_rpc_reply(dsn_message_t response, dsn_error_t err)
{
    DSN_C_GUARD_BEGIN
    auto msg = ((::dsn::message_ex*)response);
    if (msg == nullptr)
    {
        derror("dsn_rpc_reply got null response");
        return ::dsn::ERR_INVALID_PARAMETERS.get();
    }

    auto rpc = ::dsn::task::get_current_rpc();
    if (rpc == nullptr)
    {
        derror("dsn_rpc_reply got null current rpc engine");
        return ::dsn::ERR_INVALID_STATE.get();
    }

    rpc->reply(msg, err);
    return ::dsn::ERR_OK.get();
    DSN_C_GUARD_END(::dsn::ERR_UNKNOWN.get())
}

DSN_API dsn_error_t dsn_rpc_forward(dsn_message_t request, dsn_address_t addr)
{
    DSN_C_GUARD_BEGIN
    auto msg = (::dsn::message_ex*)(request);
    if (msg == nullptr)
    {
        derror("dsn_rpc_forward got null request");
        return ::dsn::ERR_INVALID_PARAMETERS.get();
    }

    auto target = ::dsn::rpc_address(addr);
    if (target.is_invalid())
    {
        derror("dsn_rpc_forward got invalid target address");
        return ::dsn::ERR_INVALID_PARAMETERS.get();
    }

    auto rpc = ::dsn::task::get_current_rpc();
    if (rpc == nullptr)
    {
        derror("dsn_rpc_forward got null current rpc engine");
        return ::dsn::ERR_INVALID_STATE.get();
    }

    rpc->forward(msg, target);
    return ::dsn::ERR_OK.get();
    DSN_C_GUARD_END(::dsn::ERR_UNKNOWN.get())
}

DSN_API dsn_message_t dsn_rpc_get_response(dsn_task_t rpc_call)
{
    ::dsn::rpc_response_task* task = (::dsn::rpc_response_task*)rpc_call;
    if (task == nullptr)
    {
        derror("dsn_rpc_get_response got null rpc_call");
        return nullptr;
    }

    if (task->spec().type != TASK_TYPE_RPC_RESPONSE)
    {
        derror("dsn_rpc_get_response got non-rpc-response task");
        return nullptr;
    }

    auto msg = task->get_response();
    if (nullptr != msg)
    {
        msg->add_ref(); // released by callers
        return msg;
    }
    else
        return nullptr;
}

DSN_API dsn_error_t dsn_rpc_enqueue_response(dsn_task_t rpc_call, dsn_error_t err, dsn_message_t response)
{
    DSN_C_GUARD_BEGIN
    ::dsn::rpc_response_task* task = (::dsn::rpc_response_task*)rpc_call;
    if (task == nullptr)
    {
        derror("dsn_rpc_enqueue_response got null rpc_call");
        return ::dsn::ERR_INVALID_PARAMETERS.get();
    }

    if (task->spec().type != TASK_TYPE_RPC_RESPONSE)
    {
        derror("dsn_rpc_enqueue_response got non-rpc-response task");
        return ::dsn::ERR_INVALID_PARAMETERS.get();
    }

    auto resp = ((::dsn::message_ex*)response);
    task->enqueue(err, resp);
    return ::dsn::ERR_OK.get();
    DSN_C_GUARD_END(::dsn::ERR_UNKNOWN.get())
}

//------------------------------------------------------------------------------
//
// file operations
//
//------------------------------------------------------------------------------

DSN_API dsn_handle_t dsn_file_open(const char* file_name, int flag, int pmode)
{
    DSN_C_GUARD_BEGIN
    if (file_name == nullptr || file_name[0] == '\0')
    {
        derror("dsn_file_open got null or empty file_name");
        return nullptr;
    }

    auto disk = ::dsn::task::get_current_disk();
    if (disk == nullptr)
    {
        derror("dsn_file_open got null current disk engine");
        return nullptr;
    }

    return disk->open(file_name, flag, pmode);
    DSN_C_GUARD_END(nullptr)
}

DSN_API dsn_error_t dsn_file_close(dsn_handle_t file)
{
    DSN_C_GUARD_BEGIN
    if (file == nullptr)
    {
        derror("dsn_file_close got null file handle");
        return ::dsn::ERR_INVALID_PARAMETERS.get();
    }

    auto disk = ::dsn::task::get_current_disk();
    if (disk == nullptr)
    {
        derror("dsn_file_close got null current disk engine");
        return ::dsn::ERR_INVALID_STATE.get();
    }

    return disk->close(file).get();
    DSN_C_GUARD_END(::dsn::ERR_UNKNOWN.get())
}

DSN_API dsn_error_t dsn_file_flush(dsn_handle_t file)
{
    DSN_C_GUARD_BEGIN
    if (file == nullptr)
    {
        derror("dsn_file_flush got null file handle");
        return ::dsn::ERR_INVALID_PARAMETERS.get();
    }

    auto disk = ::dsn::task::get_current_disk();
    if (disk == nullptr)
    {
        derror("dsn_file_flush got null current disk engine");
        return ::dsn::ERR_INVALID_STATE.get();
    }

    return disk->flush(file).get();
    DSN_C_GUARD_END(::dsn::ERR_UNKNOWN.get())
}

// native HANDLE: HANDLE for windows, int for non-windows
DSN_API void* dsn_file_native_handle(dsn_handle_t file)
{
    if (file == nullptr)
    {
        derror("dsn_file_native_handle got null file handle");
        return nullptr;
    }

    auto dfile = (::dsn::disk_file*)file;
    return dfile->native_handle();
}

DSN_API dsn_task_t dsn_file_create_aio_task(dsn_task_code_t code, dsn_aio_handler_t cb, void* context, int hash, dsn_task_tracker_t tracker)
{
    DSN_C_GUARD_BEGIN
    auto sp = ::dsn::task_spec::get(code);
    if (code == ::dsn::TASK_CODE_INVALID || sp == nullptr || sp->type != TASK_TYPE_AIO)
    {
        derror("dsn_file_create_aio_task got invalid code = %d", code);
        return nullptr;
    }

    auto t = new ::dsn::aio_task(code, cb, context, nullptr, hash);
    t->set_tracker((dsn::task_tracker*)tracker);
    t->spec().on_task_create.execute(::dsn::task::get_current_task(), t);
    return t;
    DSN_C_GUARD_END(nullptr)
}

DSN_API dsn_task_t dsn_file_create_aio_task_ex(dsn_task_code_t code, dsn_aio_handler_t cb, 
    dsn_task_cancelled_handler_t on_cancel,
    void* context, int hash, dsn_task_tracker_t tracker)
{
    DSN_C_GUARD_BEGIN
    auto sp = ::dsn::task_spec::get(code);
    if (code == ::dsn::TASK_CODE_INVALID || sp == nullptr || sp->type != TASK_TYPE_AIO)
    {
        derror("dsn_file_create_aio_task_ex got invalid code = %d", code);
        return nullptr;
    }

    auto t = new ::dsn::aio_task(code, cb, context, on_cancel, hash);
    t->set_tracker((dsn::task_tracker*)tracker);
    t->spec().on_task_create.execute(::dsn::task::get_current_task(), t);
    return t;
    DSN_C_GUARD_END(nullptr)
}

DSN_API dsn_error_t dsn_file_read(dsn_handle_t file, char* buffer, int count, uint64_t offset, dsn_task_t cb)
{
    DSN_C_GUARD_BEGIN
    if (file == nullptr)
    {
        derror("dsn_file_read got null file handle");
        return ::dsn::ERR_INVALID_PARAMETERS.get();
    }

    if (buffer == nullptr)
    {
        derror("dsn_file_read got null buffer");
        return ::dsn::ERR_INVALID_PARAMETERS.get();
    }

    if (count < 0)
    {
        derror("dsn_file_read got invalid count = %d", count);
        return ::dsn::ERR_INVALID_PARAMETERS.get();
    }

    ::dsn::aio_task* callback((::dsn::aio_task*)cb);
    if (callback == nullptr)
    {
        derror("dsn_file_read got null callback task");
        return ::dsn::ERR_INVALID_PARAMETERS.get();
    }

    auto disk = ::dsn::task::get_current_disk();
    if (disk == nullptr)
    {
        derror("dsn_file_read got null current disk engine");
        return ::dsn::ERR_INVALID_STATE.get();
    }

    callback->aio()->buffer = buffer;
    callback->aio()->buffer_size = count;
    callback->aio()->engine = nullptr;
    callback->aio()->file = file;
    callback->aio()->file_offset = offset;
    callback->aio()->type = ::dsn::AIO_Read;

    disk->read(callback);
    return ::dsn::ERR_OK.get();
    DSN_C_GUARD_END(::dsn::ERR_UNKNOWN.get())
}

DSN_API dsn_error_t dsn_file_write(dsn_handle_t file, const char* buffer, int count, uint64_t offset, dsn_task_t cb)
{
    DSN_C_GUARD_BEGIN
    if (file == nullptr)
    {
        derror("dsn_file_write got null file handle");
        return ::dsn::ERR_INVALID_PARAMETERS.get();
    }

    if (buffer == nullptr)
    {
        derror("dsn_file_write got null buffer");
        return ::dsn::ERR_INVALID_PARAMETERS.get();
    }

    if (count < 0)
    {
        derror("dsn_file_write got invalid count = %d", count);
        return ::dsn::ERR_INVALID_PARAMETERS.get();
    }

    ::dsn::aio_task* callback((::dsn::aio_task*)cb);
    if (callback == nullptr)
    {
        derror("dsn_file_write got null callback task");
        return ::dsn::ERR_INVALID_PARAMETERS.get();
    }

    auto disk = ::dsn::task::get_current_disk();
    if (disk == nullptr)
    {
        derror("dsn_file_write got null current disk engine");
        return ::dsn::ERR_INVALID_STATE.get();
    }

    callback->aio()->buffer = (char*)buffer;
    callback->aio()->buffer_size = count;
    callback->aio()->engine = nullptr;
    callback->aio()->file = file;
    callback->aio()->file_offset = offset;
    callback->aio()->type = ::dsn::AIO_Write;

    disk->write(callback);
    return ::dsn::ERR_OK.get();
    DSN_C_GUARD_END(::dsn::ERR_UNKNOWN.get())
}

DSN_API dsn_error_t dsn_file_write_vector(dsn_handle_t file, const dsn_file_buffer_t* buffers, int buffer_count, uint64_t offset, dsn_task_t cb)
{
    DSN_C_GUARD_BEGIN
    if (file == nullptr)
    {
        derror("dsn_file_write_vector got null file handle");
        return ::dsn::ERR_INVALID_PARAMETERS.get();
    }

    if (buffers == nullptr)
    {
        derror("dsn_file_write_vector got null buffers");
        return ::dsn::ERR_INVALID_PARAMETERS.get();
    }

    if (buffer_count <= 0)
    {
        derror("dsn_file_write_vector got invalid buffer_count = %d", buffer_count);
        return ::dsn::ERR_INVALID_PARAMETERS.get();
    }

    ::dsn::aio_task* callback((::dsn::aio_task*)cb);
    if (callback == nullptr)
    {
        derror("dsn_file_write_vector got null callback task");
        return ::dsn::ERR_INVALID_PARAMETERS.get();
    }

    auto disk = ::dsn::task::get_current_disk();
    if (disk == nullptr)
    {
        derror("dsn_file_write_vector got null current disk engine");
        return ::dsn::ERR_INVALID_STATE.get();
    }

    uint64_t total_buffer_size = 0;
    for (int i = 0; i < buffer_count; i++)
    {
        if (buffers[i].size < 0)
        {
            derror("dsn_file_write_vector got invalid buffer size = %d at index = %d",
                   buffers[i].size,
                   i);
            return ::dsn::ERR_INVALID_PARAMETERS.get();
        }

        if (buffers[i].buffer == nullptr)
        {
            derror("dsn_file_write_vector got null buffer at index = %d", i);
            return ::dsn::ERR_INVALID_PARAMETERS.get();
        }

        total_buffer_size += static_cast<uint32_t>(buffers[i].size);
        if (total_buffer_size > (std::numeric_limits<uint32_t>::max)())
        {
            derror("dsn_file_write_vector total buffer size exceeds %u bytes",
                   static_cast<unsigned int>((std::numeric_limits<uint32_t>::max)()));
            return ::dsn::ERR_INVALID_PARAMETERS.get();
        }
    }

    std::vector<dsn_file_buffer_t> write_buffers(buffers, buffers + buffer_count);
    callback->aio()->buffer = nullptr;
    callback->aio()->buffer_size = static_cast<uint32_t>(total_buffer_size);
    callback->aio()->engine = nullptr;
    callback->aio()->file = file;
    callback->aio()->file_offset = offset;
    callback->aio()->type = ::dsn::AIO_Write;
    callback->_unmerged_write_buffers = std::move(write_buffers);

    disk->write(callback);
    return ::dsn::ERR_OK.get();
    DSN_C_GUARD_END(::dsn::ERR_UNKNOWN.get())
}

DSN_API dsn_error_t dsn_file_copy_remote_directory(dsn_address_t remote, const char* source_dir,
    const char* dest_dir, bool overwrite, dsn_task_t cb)
{
    DSN_C_GUARD_BEGIN
    if (::dsn::rpc_address(remote).is_invalid())
    {
        derror("dsn_file_copy_remote_directory got invalid remote address");
        return ::dsn::ERR_INVALID_PARAMETERS.get();
    }

    if (source_dir == nullptr || source_dir[0] == '\0')
    {
        derror("dsn_file_copy_remote_directory got null or empty source_dir");
        return ::dsn::ERR_INVALID_PARAMETERS.get();
    }

    if (dest_dir == nullptr || dest_dir[0] == '\0')
    {
        derror("dsn_file_copy_remote_directory got null or empty dest_dir");
        return ::dsn::ERR_INVALID_PARAMETERS.get();
    }

    ::dsn::aio_task* callback((::dsn::aio_task*)cb);
    if (callback == nullptr)
    {
        derror("dsn_file_copy_remote_directory got null callback task");
        return ::dsn::ERR_INVALID_PARAMETERS.get();
    }

    auto nfs = ::dsn::task::get_current_nfs();
    if (nfs == nullptr)
    {
        derror("dsn_file_copy_remote_directory got null current nfs node");
        return ::dsn::ERR_INVALID_STATE.get();
    }

    std::shared_ptr< ::dsn::remote_copy_request> rci(new ::dsn::remote_copy_request());
    rci->source = remote;
    rci->source_dir = source_dir;
    rci->files.clear();
    rci->dest_dir = dest_dir;
    rci->overwrite = overwrite;

    nfs->call(rci, callback);
    return ::dsn::ERR_OK.get();
    DSN_C_GUARD_END(::dsn::ERR_UNKNOWN.get())
}

DSN_API dsn_error_t dsn_file_copy_remote_files(dsn_address_t remote, const char* source_dir, const char** source_files, const char* dest_dir, bool overwrite, dsn_task_t cb)
{
    DSN_C_GUARD_BEGIN
    if (::dsn::rpc_address(remote).is_invalid())
    {
        derror("dsn_file_copy_remote_files got invalid remote address");
        return ::dsn::ERR_INVALID_PARAMETERS.get();
    }

    if (source_dir == nullptr || source_dir[0] == '\0')
    {
        derror("dsn_file_copy_remote_files got null or empty source_dir");
        return ::dsn::ERR_INVALID_PARAMETERS.get();
    }

    if (source_files == nullptr)
    {
        derror("dsn_file_copy_remote_files got null source_files");
        return ::dsn::ERR_INVALID_PARAMETERS.get();
    }

    if (dest_dir == nullptr || dest_dir[0] == '\0')
    {
        derror("dsn_file_copy_remote_files got null or empty dest_dir");
        return ::dsn::ERR_INVALID_PARAMETERS.get();
    }

    ::dsn::aio_task* callback((::dsn::aio_task*)cb);
    if (callback == nullptr)
    {
        derror("dsn_file_copy_remote_files got null callback task");
        return ::dsn::ERR_INVALID_PARAMETERS.get();
    }

    auto nfs = ::dsn::task::get_current_nfs();
    if (nfs == nullptr)
    {
        derror("dsn_file_copy_remote_files got null current nfs node");
        return ::dsn::ERR_INVALID_STATE.get();
    }

    std::shared_ptr< ::dsn::remote_copy_request> rci(new ::dsn::remote_copy_request());
    rci->source = remote;
    rci->source_dir = source_dir;

    rci->files.clear();
    const char** p = source_files;
    while (*p != nullptr && **p != '\0')
    {
        rci->files.push_back(*p);
        p++;

        dinfo("copy remote file %s from %s", 
            *(p-1),
            rci->source.to_string()
            );
    }

    rci->dest_dir = dest_dir;
    rci->overwrite = overwrite;

    nfs->call(rci, callback);
    return ::dsn::ERR_OK.get();
    DSN_C_GUARD_END(::dsn::ERR_UNKNOWN.get())
}

DSN_API size_t dsn_file_get_io_size(dsn_task_t cb_task)
{
    ::dsn::task* task = (::dsn::task*)cb_task;
    if (task == nullptr)
    {
        derror("dsn_file_get_io_size got null callback task");
        return static_cast<size_t>(-1);
    }

    if (task->spec().type != TASK_TYPE_AIO)
    {
        derror("dsn_file_get_io_size got non-aio callback task");
        return static_cast<size_t>(-1);
    }

    return ((::dsn::aio_task*)task)->get_transferred_size();
}

DSN_API dsn_error_t dsn_file_task_enqueue(dsn_task_t cb_task, dsn_error_t err, size_t size)
{
    DSN_C_GUARD_BEGIN
    ::dsn::task* task = (::dsn::task*)cb_task;
    if (task == nullptr)
    {
        derror("dsn_file_task_enqueue got null callback task");
        return ::dsn::ERR_INVALID_PARAMETERS.get();
    }

    if (task->spec().type != TASK_TYPE_AIO)
    {
        derror("dsn_file_task_enqueue got non-aio callback task");
        return ::dsn::ERR_INVALID_PARAMETERS.get();
    }

    ((::dsn::aio_task*)task)->enqueue(err, size);
    return ::dsn::ERR_OK.get();
    DSN_C_GUARD_END(::dsn::ERR_UNKNOWN.get())
}

//------------------------------------------------------------------------------
//
// env
//
//------------------------------------------------------------------------------
DSN_API uint64_t dsn_now_ns()
{
    //return ::dsn::task::get_current_env()->now_ns();
    return ::dsn::service_engine::instance().env()->now_ns();
}

DSN_API uint64_t dsn_random64(uint64_t min, uint64_t max) // [min, max]
{
    return ::dsn::service_engine::instance().env()->random64(min, max);
}

//------------------------------------------------------------------------------
//
// system
//
//------------------------------------------------------------------------------

# if defined(_WIN32)
static BOOL SuspendAllThreads()
{
    std::map<uint32_t, HANDLE> threads;
    uint32_t dwCurrentThreadId = ::GetCurrentThreadId();
    uint32_t dwCurrentProcessId = ::GetCurrentProcessId();
    HANDLE hSnapshot = INVALID_HANDLE_VALUE;
    bool bChange = TRUE;

    while (bChange) 
    {
        hSnapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) 
        {
            derror("CreateToolhelp32Snapshot failed, err = %d", ::GetLastError());
            return FALSE;
        }

        THREADENTRY32 ti;
        ZeroMemory(&ti, sizeof(ti));
        ti.dwSize = sizeof(ti);
        bChange = FALSE;

        if (FALSE == ::Thread32First(hSnapshot, &ti)) 
        {
            derror("Thread32First failed, err = %d", ::GetLastError());
            goto err;
        }

        do 
        {
            if (ti.th32OwnerProcessID == dwCurrentProcessId &&
                ti.th32ThreadID != dwCurrentThreadId &&
                threads.find(ti.th32ThreadID) == threads.end()) 
                {
                    HANDLE hThread = ::OpenThread(THREAD_ALL_ACCESS, FALSE, ti.th32ThreadID);
                    if (hThread == nullptr)
                    {
                        derror("OpenThread failed, err = %d", ::GetLastError());
                        goto err;
                    }
                    if (::SuspendThread(hThread) == (DWORD)-1)
                    {
                        derror("SuspendThread failed, err = %d", ::GetLastError());
                        if (::CloseHandle(hThread) == FALSE)
                        {
                            derror("CloseHandle failed, err = %d", ::GetLastError());
                        }
                        goto err;
                    }
                    ddebug("thread %d find and suspended ...", ti.th32ThreadID);
                    threads.insert(std::make_pair(ti.th32ThreadID, hThread));
                    bChange = TRUE;
                }
        } while (::Thread32Next(hSnapshot, &ti));

        if (::CloseHandle(hSnapshot) == FALSE)
        {
            derror("CloseHandle failed, err = %d", ::GetLastError());
            return FALSE;
        }
    }

    return TRUE;

err:
    if ((hSnapshot != INVALID_HANDLE_VALUE) && (::CloseHandle(hSnapshot) == FALSE))
    {
        derror("CloseHandle failed, err = %d", ::GetLastError());
    }
    return FALSE;
}
# endif

NORETURN DSN_API void dsn_exit(int code)
{
    dinfo("dsn exit with code %d", code);
    ::dsn::tools::sys_exit.execute(::dsn::SYS_EXIT_NORMAL);

# if defined(_WIN32)
    // TODO: do not use std::map above, coz when suspend the other threads, they may stop
    // inside certain locks which causes deadlock
    // SuspendAllThreads();
    if (::TerminateProcess(::GetCurrentProcess(), code) == FALSE)
    {
        derror("TerminateProcess failed, err = %d", ::GetLastError());
        ::abort();
    }
# else    
    _exit(code);
 // kill(getpid(), SIGKILL);
# endif
}

DSN_API bool dsn_mimic_app(const char* app_name, int index)
{
    DSN_C_GUARD_BEGIN
    if (app_name == nullptr || app_name[0] == '\0')
    {
        derror("dsn_mimic_app got null or empty app_name");
        return false;
    }

    if (index <= 0)
    {
        derror("dsn_mimic_app got invalid index = %d", index);
        return false;
    }

    auto worker = ::dsn::task::get_current_worker2();
    if (worker != nullptr)
    {
        derror("cannot call dsn_mimic_app in rDSN threads");
        return false;
    }

    auto cnode = ::dsn::task::get_current_node2();
    if (cnode != nullptr)
    {
        const auto& name = cnode->spec().name;
        if (cnode->spec().role_name == ::dsn::safe_string(app_name)
            && cnode->spec().index == index)
        {
            return true;
        }
        else
        {
            derror("current thread is already attached to another rDSN app %s", name.c_str());
            return false;
        }
    }

    auto nodes = ::dsn::service_engine::instance().get_all_nodes();
    for (auto& n : nodes)
    {
        if (n.second->spec().role_name == ::dsn::safe_string(app_name)
            && n.second->spec().index == index)
        {
            ::dsn::task::set_tls_dsn_context(n.second, nullptr, nullptr);
            return true;
        }
    }

    derror("cannot find host app %s with index %d", app_name, index);
    return false;
    DSN_C_GUARD_END(false)
}

DSN_API const char* dsn_get_app_data_dir(dsn_gpid gpid)
{
    DSN_C_GUARD_BEGIN
    if (gpid.value != 0 && (gpid.u.app_id <= 0 || gpid.u.partition_index < 0))
    {
        derror("dsn_get_app_data_dir got invalid gpid = %d.%d",
               gpid.u.app_id,
               gpid.u.partition_index);
        return nullptr;
    }

    auto info = dsn_get_app_info_ptr(gpid);
    return info ? info->data_dir : nullptr;
    DSN_C_GUARD_END(nullptr)
}

DSN_API bool dsn_get_current_app_info(/*out*/ dsn_app_info* app_info)
{
    if (app_info == nullptr)
    {
        derror("dsn_get_current_app_info got null app_info");
        return false;
    }

    auto info = dsn_get_app_info_ptr(dsn_gpid());
    if (info)
    {
        memcpy(app_info, info, sizeof(*info));
        return true;
    }
    else
        return false;
}

DSN_API dsn_app_info* dsn_get_app_info_ptr(dsn_gpid gpid)
{
    DSN_C_GUARD_BEGIN
    if (gpid.value != 0 && (gpid.u.app_id <= 0 || gpid.u.partition_index < 0))
    {
        derror("dsn_get_app_info_ptr got invalid gpid = %d.%d",
               gpid.u.app_id,
               gpid.u.partition_index);
        return nullptr;
    }

    auto cnode = ::dsn::task::get_current_node2();
    if (cnode != nullptr)
    {
        if (gpid.value == 0)
            return cnode->get_l1_info();
        else
        {
            return cnode->get_l2_handler().get_app_info(gpid);
        }
    }
    else
        return nullptr;
    DSN_C_GUARD_END(nullptr)
}

::dsn::utils::notify_event s_loader_event;
DSN_API void dsn_app_loader_signal()
{
    s_loader_event.notify();
}

DSN_API void dsn_app_loader_wait()
{
    s_loader_event.wait();
}
