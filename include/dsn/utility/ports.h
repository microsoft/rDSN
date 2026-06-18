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

# pragma once

# if defined(_WIN32)

# include <windows.h>

__pragma(warning(disable:4127))

# define __thread __declspec(thread)
# define __selectany __declspec(selectany) extern 

# elif defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__)

# include <unistd.h>

# define __selectany __attribute__((weak)) extern 

# if !defined(O_BINARY)
# define O_BINARY 0
# endif

# else

# error "unsupported platform"
# endif

// stl headers
# include <string>
# include <memory>
# include <map>
# include <unordered_map>
# include <set>
# include <unordered_set>
# include <vector>
# include <list>
# include <algorithm>

// common c headers
# include <cinttypes>
# include <cassert>
# include <cstring>
# include <cstdlib>
# include <fcntl.h> // for file open flags
# include <cstdio>
# include <climits>
# include <cerrno>
# include <cstdint>

// common utilities
# include <atomic>

// common macros and data structures
# if !defined(FIELD_OFFSET)
# define FIELD_OFFSET(s, field)  (((size_t)&((s *)(10))->field) - 10)
# endif

# if !defined(CONTAINING_RECORD)
# define CONTAINING_RECORD(address, type, field) \
    ((type *)((char*)(address)-FIELD_OFFSET(type, field)))
# endif

# if !defined(MAX_COMPUTERNAME_LENGTH)
# define MAX_COMPUTERNAME_LENGTH 32
# endif

# if !defined(ARRAYSIZE)
# define ARRAYSIZE(a) \
    ((sizeof(a) / sizeof(*(a))) / static_cast<size_t>(!(sizeof(a) % sizeof(*(a)))))
# endif

# if !defined(snprintf_p)
# if defined(_WIN32)
# define snprintf_p sprintf_s
# else
# define snprintf_p std::snprintf
# endif
# endif

# if defined(_WIN32)

// make sure to include <winsock2.h> before the usage

# if !defined(_WINSOCK2API_)
template <typename TResult, typename TArg>
inline TResult require_winsock2_for_byte_order(TArg)
{
    static_assert(sizeof(TArg) == 0,
        "include <winsock2.h> before using Windows byte-order macros");
    return TResult();
}
# endif

# if !defined(be16toh)
# if defined(_WINSOCK2API_)
# define be16toh(x) ntohs(x)
# else
# define be16toh(x) ::require_winsock2_for_byte_order<uint16_t>(x)
# endif
# endif

# if !defined(htobe16)
# if defined(_WINSOCK2API_)
# define htobe16(x) htons(x)
# else
# define htobe16(x) ::require_winsock2_for_byte_order<uint16_t>(x)
# endif
# endif

static_assert (sizeof(uint32_t) == sizeof(unsigned long),
    "sizeof(uint32_t) == sizeof(unsigned long) for use of ntohl");

# if !defined(be32toh)
# if defined(_WINSOCK2API_)
# define be32toh(x) static_cast<uint32_t>(ntohl(static_cast<unsigned long>(x)))
# else
# define be32toh(x) ::require_winsock2_for_byte_order<uint32_t>(x)
# endif
# endif

# if !defined(htobe32)
# if defined(_WINSOCK2API_)
# define htobe32(x) static_cast<uint32_t>(htonl(static_cast<unsigned long>(x)))
# else
# define htobe32(x) ::require_winsock2_for_byte_order<uint32_t>(x)
# endif
# endif

# if !defined(be64toh)
# if defined(_WINSOCK2API_)
# define be64toh(x) ( static_cast<uint64_t>(be32toh(static_cast<uint32_t>((x) >> 32))) | ( static_cast<uint64_t>(be32toh(static_cast<uint32_t>(x))) << 32 ) )
# else
# define be64toh(x) ::require_winsock2_for_byte_order<uint64_t>(x)
# endif
# endif


# endif // _WIN32

# if defined(__APPLE__)

# include <libkern/OSByteOrder.h>

# if !defined(be16toh)
# define be16toh(x) OSSwapBigToHostInt16(x)
# endif

# if !defined(htobe16)
# define htobe16(x) OSSwapHostToBigInt16(x)
# endif

# if !defined(be32toh)
# define be32toh(x) OSSwapBigToHostInt32(x)
# endif

# if !defined(htobe32)
# define htobe32(x) OSSwapHostToBigInt32(x)
# endif

# if !defined(be64toh)
# define be64toh(x) OSSwapBigToHostInt64(x)
# endif

# if !defined(htobe64)
# define htobe64(x) OSSwapHostToBigInt64(x)
# endif

# endif
