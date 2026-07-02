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
 *     Unit-test for dsn::utils::singleton, focusing on the guarantee that the
 *     internal initialization lock is released when T's constructor throws
 *     (e.g. std::bad_alloc under memory pressure), so instance() fails-stop and
 *     stays retryable instead of dead-spinning on the lock forever.
 *
 * Revision history:
 *     2026-07-02, first version
 */

# include <dsn/utility/singleton.h>
# include <gtest/gtest.h>
# include <stdexcept>

namespace {

// A well-behaved singleton used to check the normal (no-throw) path.
struct good_singleton : public dsn::utils::singleton<good_singleton>
{
    good_singleton() : value(42) {}
    int value;

    static int lock_state() { return _l.load(std::memory_order_acquire); }
};

// A singleton whose constructor throws on demand, to simulate a failure during
// "new T()". lock_state() exposes the otherwise-internal spinlock so the test
// can prove the RAII unlock guard released it even on the throwing path.
struct throwing_singleton : public dsn::utils::singleton<throwing_singleton>
{
    throwing_singleton()
    {
        if (s_should_throw)
        {
            throw std::runtime_error("simulated constructor failure");
        }
    }

    static bool s_should_throw;

    static int lock_state() { return _l.load(std::memory_order_acquire); }
};

bool throwing_singleton::s_should_throw = true;

} // anonymous namespace

TEST(core, singleton_basic)
{
    ASSERT_FALSE(good_singleton::is_instance_created());

    good_singleton& a = good_singleton::instance();
    EXPECT_EQ(42, a.value);
    EXPECT_TRUE(good_singleton::is_instance_created());

    // instance() is idempotent: same object, lock released after construction.
    good_singleton& b = good_singleton::instance();
    EXPECT_EQ(&a, &b);
    EXPECT_EQ(&a, &good_singleton::fast_instance());
    EXPECT_EQ(0, good_singleton::lock_state());
}

TEST(core, singleton_ctor_throw_releases_lock)
{
    // Fresh state: nothing built, lock free.
    ASSERT_FALSE(throwing_singleton::is_instance_created());
    ASSERT_EQ(0, throwing_singleton::lock_state());

    // 1) The constructor throws -> instance() must propagate the exception
    //    (fail-stop), not swallow it and not hang.
    throwing_singleton::s_should_throw = true;
    EXPECT_THROW(throwing_singleton::instance(), std::runtime_error);

    // 2) The RAII unlock guard must have released the internal spinlock even
    //    though the constructor threw. Use ASSERT (non-local return) so that a
    //    regression which leaks the lock stops the test HERE, instead of letting
    //    the retry below dead-spin forever on the busy-wait lock.
    ASSERT_EQ(0, throwing_singleton::lock_state());

    // 3) No half-built instance was published.
    EXPECT_FALSE(throwing_singleton::is_instance_created());

    // 4) A subsequent call succeeds: the failure was cleanly retryable, proving
    //    there is no permanent deadlock.
    throwing_singleton::s_should_throw = false;
    throwing_singleton* built = nullptr;
    EXPECT_NO_THROW(built = &throwing_singleton::instance());
    EXPECT_NE(nullptr, built);
    EXPECT_TRUE(throwing_singleton::is_instance_created());
    EXPECT_EQ(0, throwing_singleton::lock_state());
}
