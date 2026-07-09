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

# include "transient_memory.h"

# include <cstddef>
# include <cstdint>
# include <cstdlib>
# include <limits>
# include <memory>

# if defined(__TITLE__)
# undef __TITLE__
# endif
# define __TITLE__ "transient_memory"

namespace dsn 
{
    __thread tls_transient_memory_t tls_trans_memory;

    static size_t tls_trans_mem_default_block_bytes = 1024 * 1024; // 1 MB

    // The single alignment invariant for transient memory: every pointer handed out by
    // tls_trans_mem_next is aligned to alignof(std::max_align_t) -- the strictest
    // alignment any scalar type needs. Enforcing it in this one low-level place makes
    // every structure later overlaid on transient memory naturally aligned, with no
    // alignment logic at the call sites:
    //   * message_header on the send path (message_ex::prepare_buffer_header / write_next),
    //   * the std::shared_ptr bookkeeping tls_trans_malloc stores below its user pointer,
    //   * and the pointer returned by the public dsn_transient_malloc, whose malloc-style
    //     contract requires max_align_t alignment.
    // Fresh blocks come from make_shared_array<char> (operator new[]), which is already
    // max_align_t-aligned, so tls_trans_mem_next only has to re-align the bump pointer
    // after each exact-size commit.
    static const size_t tls_trans_alignment = alignof(std::max_align_t);

    // tls_trans_malloc stores [ std::shared_ptr<char> ][ pad ][ uint32_t magic ] in the
    // tls_trans_malloc_header_size bytes immediately below the pointer it returns.
    // header_size is rounded up to tls_trans_alignment so that, given an aligned base from
    // tls_trans_mem_next, both the shared_ptr (placed at the base) and the returned user
    // pointer (base + header_size) are aligned. tls_trans_malloc and tls_trans_free share
    // these constants so the write and free layouts can never drift apart.
    static const size_t tls_trans_malloc_magic_size = sizeof(uint32_t);
    static const size_t tls_trans_malloc_header_size =
        ((sizeof(std::shared_ptr<char>) + tls_trans_malloc_magic_size + tls_trans_alignment - 1)
            / tls_trans_alignment) * tls_trans_alignment;

    void tls_trans_mem_init(size_t default_per_block_bytes)
    {
        tls_trans_mem_default_block_bytes = default_per_block_bytes;
    }

    void tls_trans_mem_alloc(size_t min_size)
    {
        // release last buffer if necessary
        if (tls_trans_memory.magic == 0xdeadbeef)
        {
            tls_trans_memory.block->reset();
        }
        else
        {
            tls_trans_memory.magic = 0xdeadbeef;
            tls_trans_memory.block = new(tls_trans_memory.block_ptr_buffer) std::shared_ptr<char>();
            tls_trans_memory.committed = true;
        }

        tls_trans_memory.remain_bytes = (min_size > tls_trans_mem_default_block_bytes ? 
                min_size : tls_trans_mem_default_block_bytes);
        *tls_trans_memory.block = ::dsn::make_shared_array<char>(tls_trans_memory.remain_bytes);
        tls_trans_memory.next = tls_trans_memory.block->get();
    }

    void tls_trans_mem_next(void** ptr, size_t* sz, size_t min_size)
    {
        if (tls_trans_memory.magic != 0xdeadbeef)
            tls_trans_mem_alloc(min_size);
        else
        {
            dassert(tls_trans_memory.committed == true,
                "tls_trans_mem_next and tls_trans_mem_commit must be called in pair");

            // Round the bump pointer up to tls_trans_alignment before handing it out.
            // Commits advance next by exact use sizes, so without this every allocation
            // after the first would start at an arbitrary offset and misalign the
            // structures overlaid on it (e.g. message_header in prepare_buffer_header).
            const uintptr_t cur = reinterpret_cast<uintptr_t>(tls_trans_memory.next);
            const size_t pad =
                (tls_trans_alignment - (cur & (tls_trans_alignment - 1))) & (tls_trans_alignment - 1);
            if (pad <= tls_trans_memory.remain_bytes)
            {
                tls_trans_memory.next += pad;
                tls_trans_memory.remain_bytes -= pad;
            }
            else
            {
                // not enough room left even to align; force a fresh (aligned) block below
                tls_trans_memory.remain_bytes = 0;
            }

            if (min_size > tls_trans_memory.remain_bytes)
                tls_trans_mem_alloc(min_size);
        }

        *ptr = static_cast<void*>(tls_trans_memory.next);
        *sz = tls_trans_memory.remain_bytes;
        tls_trans_memory.committed = false;
    }

    void tls_trans_mem_commit(size_t use_size)
    {
        dbg_dassert(tls_trans_memory.magic == 0xdeadbeef
            && !tls_trans_memory.committed
            && use_size <= tls_trans_memory.remain_bytes,
            "invalid use or parameter of tls_trans_mem_commit");

        tls_trans_memory.next += use_size;
        tls_trans_memory.remain_bytes -= use_size;
        tls_trans_memory.committed = true;
    }

    blob tls_trans_mem_alloc_blob(size_t sz)
    {
        void* ptr;
        size_t sz2;
        tls_trans_mem_next(&ptr, &sz2, sz);

        ::dsn::blob buffer(
            (*::dsn::tls_trans_memory.block),
            (int)((char*)(ptr)-::dsn::tls_trans_memory.block->get()),
            (int)sz
            );

        tls_trans_mem_commit(sz);
        return buffer;
    }

    void* tls_trans_malloc(size_t sz)
    {
        const size_t user_size = sz;
        const size_t header_size = tls_trans_malloc_header_size;
        // Guard against size_t overflow when padding the request for the header. Without
        // this, a ~4GB request on a 32-bit build would wrap alloc_size to a tiny value and
        // the subsequent writes would run past the end of the allocation (heap overflow).
        if (user_size > (std::numeric_limits<std::size_t>::max)() - header_size)
        {
            derror("tls_trans_malloc: requested size %zu is too large", user_size);
            return nullptr;
        }
        const size_t alloc_size = user_size + header_size;
        void* ptr;
        size_t sz2;
        tls_trans_mem_next(&ptr, &sz2, alloc_size);

        // tls_trans_mem_next hands back a max_align_t-aligned base and header_size is a
        // multiple of that alignment, so the shared_ptr placed at the base and the returned
        // user pointer are both aligned without any per-allocation rounding here.
        char* const raw_ptr = static_cast<char*>(ptr);
        char* const user_ptr = raw_ptr + header_size;

        // add ref
        new (raw_ptr) std::shared_ptr<char>(*::dsn::tls_trans_memory.block);

        // add magic
        *(uint32_t*)(user_ptr - tls_trans_malloc_magic_size) = 0xdeadbeef;

        tls_trans_mem_commit(alloc_size);

        return user_ptr;
    }

    void tls_trans_free(void* ptr)
    {
        // Mirror the layout written by tls_trans_malloc: the magic sits just below the user
        // pointer, and the shared_ptr sits header_size below it (header_size is padded so the
        // shared_ptr was stored on an aligned address).
        char* const user_ptr = static_cast<char*>(ptr);
        dassert(*(uint32_t*)(user_ptr - tls_trans_malloc_magic_size) == 0xdeadbeef,
            "invalid transient memory block");

        std::shared_ptr<char>* const holder =
            reinterpret_cast<std::shared_ptr<char>*>(user_ptr - tls_trans_malloc_header_size);
        holder->~shared_ptr<char>();
    }
}

DSN_API void* dsn_transient_malloc(uint32_t size)
{
    return ::dsn::tls_trans_malloc((size_t)size);
}

DSN_API void dsn_transient_free(void* ptr)
{
    if (ptr == nullptr)
    {
        derror("dsn_transient_free got null ptr");
        return;
    }

    return ::dsn::tls_trans_free(ptr);
}

DSN_API void* dsn_malloc(uint32_t size)
{
    return malloc((size_t)size);
}

DSN_API void dsn_free(void* ptr)
{
    if (ptr == nullptr)
    {
        derror("dsn_free got null ptr");
        return;
    }

    return free(ptr);
}
