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
 *     Shared helpers for suppressing expected GoogleTest output on success
 *     while preserving it for failed tests.
 */

#pragma once

#include <gtest/gtest.h>
#include <cstdio>

// Keep expected diagnostics out of successful test output, but replay them when
// GoogleTest records a failure in the current test.
class scoped_test_stderr
{
public:
    scoped_test_stderr()
    {
#if GTEST_HAS_STREAM_REDIRECTION
        ::testing::internal::CaptureStderr();
#endif
    }

    ~scoped_test_stderr()
    {
#if GTEST_HAS_STREAM_REDIRECTION
        const auto output = ::testing::internal::GetCapturedStderr();
        if (::testing::Test::HasFailure())
        {
            fprintf(stderr, "%s", output.c_str());
        }
#endif
    }

    scoped_test_stderr(const scoped_test_stderr&) = delete;
    scoped_test_stderr& operator=(const scoped_test_stderr&) = delete;
};
