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
 *     Goal: give low-level modules a single, uniform, dlog-like logging API
 *     (dutil_info / dutil_debug / dutil_warn / dutil_error / dutil_fatal) so
 *     they stop scattering ad-hoc fprintf() calls. This is about a consistent
 *     interface, not about the destination -- the utility layer sits *below*
 *     dsn.core in the library dependency graph (dsn.core links dsn.dev.utility,
 *     not the other way around), so utility code cannot route into the real
 *     process logger (dsn_logv / dinfo/dwarn/derror) without creating a reverse
 *     library dependency. The facade therefore writes to stderr. stderr (not
 *     stdout) keeps stdout clean for machine-readable program output.
 *
 *     These helpers are internal plumbing for rDSN's foundational layer, not
 *     part of the public API; they are intentionally not exported from any
 *     shared library.
 *
 * Revision history:
 *     2026-07-07, unify rDSN diagnostics onto the logger, first version
 *     2026-07-09, drop the pluggable sink; utility logs directly to stderr
 */

# pragma once

namespace dsn {
namespace utils {

// Low-level logging severities. These deliberately mirror the core
// dsn_log_level_t values one-to-one (INFORMATION=0 .. FATAL=4), keeping the
// utility layer free of any dependency on the C logging API header.
enum log_level_t
{
    LOG_LEVEL_UTIL_INFORMATION = 0,
    LOG_LEVEL_UTIL_DEBUG,
    LOG_LEVEL_UTIL_WARNING,
    LOG_LEVEL_UTIL_ERROR,
    LOG_LEVEL_UTIL_FATAL
};

// Format and emit a single diagnostic to stderr. This is the only entry point
// the dutil_* macros use. It is internal to the foundational layer and
// deliberately not exported from any shared library (utility code must not
// depend on dsn.core, so it cannot route through the configured logger).
extern void logf(
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
