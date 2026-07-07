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
 *     Implementation of the low-level (utility-layer) logging facade.
 *
 * Revision history:
 *     2026-07-07, unify rDSN diagnostics onto the logger, first version
 */

# include <dsn/utility/logging.h>
# include <cstdio>

namespace dsn {
namespace utils {

// The installed sink, or nullptr when none has been installed yet. Set once
// during core initialization (before worker threads start); reads are plain
// loads of a pointer that is constant for the rest of the process lifetime.
static logging_sink s_logging_sink = nullptr;

void set_logging_sink(logging_sink sink)
{
    s_logging_sink = sink;
}

void logv(log_level_t level, const char* file, const char* function, int line, const char* fmt, va_list args)
{
    char buffer[2048];
    int n = vsnprintf(buffer, sizeof(buffer), fmt, args);
    if (n < 0)
    {
        buffer[0] = '\0';
    }

    logging_sink sink = s_logging_sink;
    if (sink != nullptr)
    {
        sink(level, file, function, line, buffer);
    }
    else
    {
        // No sink installed yet (very early bootstrap before dsn.core wires one
        // up, or the utility library used standalone). Diagnostics go to stderr
        // so that stdout stays clean for machine-readable program output.
        static const char s_level_char[] = "IDWEF";
        char lc = (level >= LOG_LEVEL_UTIL_INFORMATION && level <= LOG_LEVEL_UTIL_FATAL)
                    ? s_level_char[level] : '?';
        fprintf(stderr, "%c %s:%d:%s(): %s\n", lc, file, line, function, buffer);
    }
}

void logf(log_level_t level, const char* file, const char* function, int line, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    logv(level, file, function, line, fmt, args);
    va_end(args);
}

}} // namespace dsn::utils
