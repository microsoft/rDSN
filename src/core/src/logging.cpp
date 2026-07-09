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
 * Description:
 *     What is this file about?
 *
 * Revision history:
 *     xxxx-xx-xx, author, first version
 *     xxxx-xx-xx, author, fix bug about xxx
 */

# include <dsn/service_api_c.h>
# include <dsn/tool-api/command.h>
# include <dsn/tool-api/logging_provider.h>
# include <dsn/tool_api.h>
# include "service_engine.h"
# include <dsn/cpp/auto_codes.h>
# include <cstdio>
# include <cinttypes>

DSN_API dsn_log_level_t dsn_log_start_level = dsn_log_level_t::LOG_LEVEL_INFORMATION;

static void log_on_sys_exit(::dsn::sys_exit_type)
{
    ::dsn::logging_provider* logger = ::dsn::service_engine::fast_instance().logging();
    if (logger != nullptr)
    {
        logger->flush();
    }
}

bool dsn_log_init()
{
    dsn_log_start_level = enum_from_string(
        dsn_config_get_value_string("core", "logging_start_level", enum_to_string(dsn_log_start_level),
            "logs with level below this will not be logged"),
        dsn_log_level_t::LOG_LEVEL_INVALID
        );

    if (dsn_log_start_level == dsn_log_level_t::LOG_LEVEL_INVALID)
    {
        fprintf(stderr, "invalid [core] logging_start_level specified\n");
        return false;
    }

    // register log flush on exit
    bool logging_flush_on_exit = dsn_config_get_value_bool("core", "logging_flush_on_exit",
        true, "flush log when exit system");
    if (logging_flush_on_exit)
    {
        ::dsn::tools::sys_exit.put_back(log_on_sys_exit, "log.flush");
    }

    // register command for tail logging
    ::dsn::register_command("flush-log",
        "flush-log - flush log to stderr or log file",
        "flush-log - flush log to stderr or log file",
        [](const ::dsn::safe_vector< ::dsn::safe_string>& args)
        {
            ::dsn::logging_provider* logger = ::dsn::service_engine::fast_instance().logging();
            if (logger != nullptr)
            {
                logger->flush();
            }
            return ::dsn::safe_string("Flush done.");
        }
    );
    return true;
}

DSN_API dsn_log_level_t dsn_log_get_start_level()
{
    return dsn_log_start_level;
}

// shared, unified log-line header used by all logging providers (screen_logger,
// simple_logger, ...) and by the no-provider fallback in dsn_logv below, so that
// the on-screen log format stays consistent across all of them.
void dsn::logging_provider::print_header(FILE* fp, dsn_log_level_t log_level)
{
    static char s_level_char[] = "IDWEF";

    uint64_t ts = 0;
    if (::dsn::tools::is_engine_ready())
    {
        ts = dsn_now_ns();
    }

    char str[24];
    ::dsn::utils::time_ms_to_string(ts / 1000000, str, sizeof(str));

    int tid = ::dsn::utils::get_current_tid();

    fprintf(fp, "%c%s (%" PRIu64 " %04x) ", s_level_char[log_level], str, ts, tid);

    auto t = ::dsn::task::get_current_task_id();
    auto worker = ::dsn::task::get_current_worker2();
    if (t)
    {
        if (nullptr != worker)
        {
            fprintf(fp, "%6s.%7s%d.%016" PRIx64 ": ",
                ::dsn::task::get_current_node_name(),
                worker->pool_spec().name.c_str(),
                worker->index(),
                t);
        }
        else
        {
            fprintf(fp, "%6s.%7s.%05d.%016" PRIx64 ": ",
                ::dsn::task::get_current_node_name(),
                "io-thrd",
                tid,
                t);
        }
    }
    else
    {
        if (nullptr != worker)
        {
            fprintf(fp, "%6s.%7s%u: ",
                ::dsn::task::get_current_node_name(),
                worker->pool_spec().name.c_str(),
                worker->index());
        }
        else
        {
            fprintf(fp, "%6s.%7s.%05d: ",
                ::dsn::task::get_current_node_name(),
                "io-thrd",
                tid);
        }
    }
}

DSN_API void dsn_logv(const char *file, const char *function, const int line, dsn_log_level_t log_level, const char* title, const char* fmt, va_list args)
{
    if (file == nullptr)
    {
        fprintf(stderr, "ERROR: dsn_logv got null file\n");
        return;
    }

    if (function == nullptr)
    {
        fprintf(stderr, "ERROR: dsn_logv got null function\n");
        return;
    }

    if (log_level < LOG_LEVEL_INFORMATION || log_level >= LOG_LEVEL_COUNT)
    {
        fprintf(stderr, "ERROR: dsn_logv got invalid log_level = %d\n", log_level);
        return;
    }

    if (title == nullptr)
    {
        fprintf(stderr, "ERROR: dsn_logv got null title\n");
        return;
    }

    if (fmt == nullptr)
    {
        fprintf(stderr, "ERROR: dsn_logv got null fmt\n");
        return;
    }

    ::dsn::logging_provider* logger = ::dsn::service_engine::instance().logging();
    if (logger != nullptr)
    {
        logger->dsn_logv(file, function, line, log_level, title, fmt, args);
    }
    else
    {
        // no logging provider is installed yet (e.g. during early bootstrap before
        // init_before_toollets creates it); write diagnostics to stderr so that stdout
        // stays clean for machine-readable program output (metrics, cli results, etc.),
        // reusing the shared provider header so the log format stays consistent.
        ::dsn::logging_provider::print_header(stderr, log_level);
        fprintf(stderr, "%s:%d:%s(): ", title, line, function);
        vfprintf(stderr, fmt, args);
        fprintf(stderr, "\n");
    }
}

DSN_API void dsn_logf(const char *file, const char *function, const int line, dsn_log_level_t log_level, const char* title, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    dsn_logv(file, function, line, log_level, title, fmt, ap);
    va_end(ap);
}

DSN_API void dsn_log(const char *file, const char *function, const int line, dsn_log_level_t log_level, const char* title)
{
    dsn_logf(file, function, line, log_level, title, "");
}
