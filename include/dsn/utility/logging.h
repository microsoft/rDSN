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
 *     A very small logging facade for the foundational (utility) layer.
 *
 *     The utility layer sits *below* dsn.core in the library dependency graph
 *     (dsn.core links dsn.dev.utility, not the other way around), so utility
 *     code cannot call the real logger (dsn_logv / dinfo/dwarn/derror) directly
 *     without creating a reverse library dependency. Historically it therefore
 *     hard-wired diagnostics to fprintf(stderr), which both bypasses the
 *     configured logger and scatters an un-replaceable sink across the code.
 *
 *     This facade fixes that: low-level components log through dutil_info /
 *     dutil_warn / dutil_error, which forward to a pluggable sink. dsn.core
 *     installs a sink (set_logging_sink) that routes everything to dsn_logv, so
 *     utility diagnostics flow through the same configurable, replaceable logger
 *     as the rest of rDSN. Until a sink is installed (very early bootstrap, or
 *     when the utility library is used standalone) the default is stderr, so
 *     stdout stays clean for machine-readable program output.
 *
 * Revision history:
 *     2026-07-07, unify rDSN diagnostics onto the logger, first version
 */

# pragma once

# include <dsn/utility/dlib.h>
# include <cstdarg>

namespace dsn {
namespace utils {

// Low-level logging severities. These deliberately mirror the core
// dsn_log_level_t values one-to-one (INFORMATION=0 .. FATAL=4) so the core sink
// can map them with a plain cast, while keeping the utility layer free of any
// dependency on the C logging API header.
enum log_level_t
{
    LOG_LEVEL_UTIL_INFORMATION = 0,
    LOG_LEVEL_UTIL_DEBUG,
    LOG_LEVEL_UTIL_WARNING,
    LOG_LEVEL_UTIL_ERROR,
    LOG_LEVEL_UTIL_FATAL
};

// A sink that dsn.core installs so utility-layer diagnostics are routed through
// the real, replaceable process logger. The message is already formatted.
typedef void (*logging_sink)(
    log_level_t level, const char* file, const char* function, int line, const char* msg);

// Install (or clear, with nullptr) the sink. Intended to be called once during
// core initialization, before worker threads start; not synchronized against
// concurrent logging.
extern DSN_API void set_logging_sink(logging_sink sink);

// Format and emit a single diagnostic. Routed to the installed sink, or to
// stderr when none is installed.
extern DSN_API void logv(
    log_level_t level, const char* file, const char* function, int line, const char* fmt, va_list args);
extern DSN_API void logf(
    log_level_t level, const char* file, const char* function, int line, const char* fmt, ...);

}} // namespace dsn::utils

// Convenience macros for the utility layer. Named dutil_* so they never collide
// with the core dinfo/ddebug/dwarn/derror/dfatal macros, which the utility layer
// must not use (that would pull in the logger it sits below).
# define dutil_logf(level, ...) \
    ::dsn::utils::logf((level), __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
# define dutil_info(...)  dutil_logf(::dsn::utils::LOG_LEVEL_UTIL_INFORMATION, __VA_ARGS__)
# define dutil_debug(...) dutil_logf(::dsn::utils::LOG_LEVEL_UTIL_DEBUG,       __VA_ARGS__)
# define dutil_warn(...)  dutil_logf(::dsn::utils::LOG_LEVEL_UTIL_WARNING,     __VA_ARGS__)
# define dutil_error(...) dutil_logf(::dsn::utils::LOG_LEVEL_UTIL_ERROR,       __VA_ARGS__)
# define dutil_fatal(...) dutil_logf(::dsn::utils::LOG_LEVEL_UTIL_FATAL,       __VA_ARGS__)
