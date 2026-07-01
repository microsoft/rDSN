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
        const size_t header_size = sizeof(std::shared_ptr<char>) + sizeof(uint32_t);
        const size_t alignment = alignof(std::max_align_t);
        // Guard against size_t overflow when padding the request for the header and alignment.
        // Without this, a ~4GB request on a 32-bit build would wrap alloc_size to a tiny value,
        // and the subsequent writes would run past the end of the allocation (heap overflow).
        if (user_size > (std::numeric_limits<std::size_t>::max)() - (header_size + alignment - 1))
        {
            derror("tls_trans_malloc: requested size %zu is too large", user_size);
            return nullptr;
        }
        const size_t alloc_size = user_size + header_size + alignment - 1;
        void* ptr;
        size_t sz2;
        tls_trans_mem_next(&ptr, &sz2, alloc_size);

        char* const raw_ptr = static_cast<char*>(ptr);
        const uintptr_t unaligned_user_ptr = reinterpret_cast<uintptr_t>(raw_ptr) + header_size;
        const uintptr_t aligned_user_ptr =
            ((unaligned_user_ptr + alignment - 1) / alignment) * alignment;
        char* const user_ptr = reinterpret_cast<char*>(aligned_user_ptr);
        char* const header_ptr = user_ptr - header_size;

        // add ref
        new (header_ptr) std::shared_ptr<char>(*::dsn::tls_trans_memory.block);

        // add magic
        *(uint32_t*)(user_ptr - sizeof(uint32_t)) = 0xdeadbeef;

        tls_trans_mem_commit(static_cast<size_t>(user_ptr + user_size - raw_ptr));

        return user_ptr;
    }

    void tls_trans_free(void* ptr)
    {
        ptr = (void*)((char*)ptr - sizeof(uint32_t));
        dassert(*(uint32_t*)(ptr) == 0xdeadbeef, "invalid transient memory block");

        ptr = (void*)((char*)ptr - sizeof(std::shared_ptr<char>));
        ((std::shared_ptr<char>*)(ptr))->~shared_ptr<char>();
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
