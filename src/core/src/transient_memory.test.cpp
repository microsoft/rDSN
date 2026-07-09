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
 *     Unit-test for transient memory.
 *
 * Revision history:
 *     Nov., 2015, @qinzuoyan (Zuoyan Qin), first version
 *     xxxx-xx-xx, author, fix bug about xxx
 */

# include "transient_memory.h"
# include <gtest/gtest.h>
# include <cstddef>
# include <cstdint>

using namespace ::dsn;

namespace
{
    // tls_trans_mem_next hands out every pointer aligned to alignof(std::max_align_t)
    // (see tls_trans_alignment in transient_memory.cpp): after a commit of an unaligned
    // size, the next acquire rounds the bump pointer up before returning it. Rounding an
    // offset the same way lets the assertions below track the bump pointer across the
    // unaligned commits this test performs.
    inline size_t align_up(size_t v)
    {
        const size_t a = alignof(std::max_align_t);
        return (v + a - 1) / a * a;
    }
}

TEST(core, transient_memory)
{
    tls_trans_mem_init(1024);

    void* ptr;
    size_t sz;
    tls_trans_mem_next(&ptr, &sz, 100);
    tls_trans_mem_commit(100);
    tls_trans_mem_next(&ptr, &sz, 10240);
    tls_trans_mem_commit(100);

    // malloc 10240
    tls_trans_mem_alloc(10240);
    ASSERT_EQ(0xdeadbeef, tls_trans_memory.magic);
    ASSERT_EQ(10240u, tls_trans_memory.remain_bytes);
    ASSERT_EQ((void*)tls_trans_memory.block_ptr_buffer, (void*)tls_trans_memory.block);
    ASSERT_EQ((void*)tls_trans_memory.next, (void*)tls_trans_memory.block->get());
    ASSERT_TRUE(tls_trans_memory.committed);

    // malloc 100
    tls_trans_mem_alloc(100);
    ASSERT_EQ(1024u, tls_trans_memory.remain_bytes);
    ASSERT_TRUE(tls_trans_memory.committed);

    // acquire 100
    tls_trans_mem_next(&ptr, &sz, 100);
    ASSERT_EQ((void*)tls_trans_memory.next, (void*)ptr);
    ASSERT_EQ(1024u, sz);
    ASSERT_EQ((void*)tls_trans_memory.next, (void*)tls_trans_memory.block->get());
    ASSERT_EQ(1024u, tls_trans_memory.remain_bytes);
    ASSERT_FALSE(tls_trans_memory.committed);

    // commit 100
    tls_trans_mem_commit(100);
    ASSERT_EQ((void*)tls_trans_memory.next, (void*)(tls_trans_memory.block->get() + 100));
    ASSERT_EQ(924u, tls_trans_memory.remain_bytes);
    ASSERT_TRUE(tls_trans_memory.committed);

    // acquire 200
    tls_trans_mem_next(&ptr, &sz, 200);
    ASSERT_EQ((void*)tls_trans_memory.next, (void*)ptr);
    // the previous commit left next at offset 100; acquiring rounds it up to align_up(100),
    // consuming the padding from the remaining bytes.
    ASSERT_EQ(1024u - align_up(100), sz);
    ASSERT_EQ((void*)tls_trans_memory.next, (void*)(tls_trans_memory.block->get() + align_up(100)));
    ASSERT_EQ(1024u - align_up(100), tls_trans_memory.remain_bytes);
    ASSERT_FALSE(tls_trans_memory.committed);

    // commit 300
    tls_trans_mem_commit(300);
    ASSERT_EQ((void*)tls_trans_memory.next, (void*)(tls_trans_memory.block->get() + align_up(100) + 300));
    ASSERT_EQ(1024u - align_up(100) - 300, tls_trans_memory.remain_bytes);
    ASSERT_TRUE(tls_trans_memory.committed);

    // acquire 10240
    tls_trans_mem_next(&ptr, &sz, 10240);
    ASSERT_EQ((void*)tls_trans_memory.next, (void*)ptr);
    ASSERT_EQ(10240u, sz);
    ASSERT_EQ((void*)tls_trans_memory.next, (void*)tls_trans_memory.block->get());
    ASSERT_EQ(10240u, tls_trans_memory.remain_bytes);
    ASSERT_FALSE(tls_trans_memory.committed);

    // commit 0
    tls_trans_mem_commit(0);
    ASSERT_EQ((void*)tls_trans_memory.next, (void*)(tls_trans_memory.block->get()));
    ASSERT_EQ(10240u, tls_trans_memory.remain_bytes);
    ASSERT_TRUE(tls_trans_memory.committed);

    tls_trans_mem_init(1024 * 1024); // restore
}

TEST(core, transient_memory_alignment)
{
    const size_t align = alignof(std::max_align_t);

    // Every pointer tls_trans_mem_next returns must be max_align_t-aligned, even right
    // after a commit of an unaligned size. This is the invariant that keeps a message_header
    // (or any other structure) overlaid on the next allocation naturally aligned.
    void* ptr;
    size_t sz;
    const size_t unaligned_commits[] = { 1, 7, 13, 100, 4095 };
    for (size_t i = 0; i < sizeof(unaligned_commits) / sizeof(unaligned_commits[0]); ++i)
    {
        tls_trans_mem_next(&ptr, &sz, 8);
        ASSERT_EQ(0u, reinterpret_cast<uintptr_t>(ptr) % align);
        tls_trans_mem_commit(unaligned_commits[i]);
    }

    // dsn_transient_malloc must honor its malloc-style contract of returning
    // max_align_t-aligned memory regardless of the requested size, and the shared_ptr
    // bookkeeping it stores below the returned pointer must itself be aligned (otherwise
    // its destructor in dsn_transient_free would be an unaligned access).
    const uint32_t malloc_sizes[] = { 1, 3, 17, 64, 1000 };
    for (size_t i = 0; i < sizeof(malloc_sizes) / sizeof(malloc_sizes[0]); ++i)
    {
        void* p = dsn_transient_malloc(malloc_sizes[i]);
        ASSERT_NE(nullptr, p);
        ASSERT_EQ(0u, reinterpret_cast<uintptr_t>(p) % align);
        dsn_transient_free(p);
    }

    tls_trans_mem_init(1024 * 1024); // restore
}

TEST(core, dsn_transient_memory_invalid_parameters)
{
    dsn_transient_free(nullptr);
    dsn_free(nullptr);
}
