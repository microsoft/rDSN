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
 *     define safe string that can across 
 *
 * Revision history:
 *     July, 2016, @imzhenyu (Zhenyu Guo), first version
 *     xxxx-xx-xx, author, fix bug about xxx
 */

#pragma once

# include <dsn/service_api_c.h>
# include <dsn/cpp/callocator.h>
# include <vector>
# include <list>
# include <cstring>
# include <string>
# include <sstream>
# include <map>
# include <unordered_map>

namespace dsn
{
    template<typename T>
    using safe_allocator = callocator<T, dsn_malloc, dsn_free>;

    template <class T, class U>
    bool operator==(const safe_allocator<T>&, const safe_allocator<U>&)
    {
        return true;
    }
    template <class T, class U>
    bool operator!=(const safe_allocator<T>&, const safe_allocator<U>&)
    {
        return false;
    }

    template<typename T>
    using safe_vector = ::std::vector<T, safe_allocator<T> >;

    template<typename T>
    using safe_list = ::std::list<T, safe_allocator<T> >;

    template<typename TKey, typename TValue>
    using safe_map = ::std::map<TKey, TValue, safe_allocator<TKey> >;

    template<typename TKey, typename TValue>
    using safe_unordered_map = ::std::unordered_map<TKey, TValue, safe_allocator<TKey> >;

    using safe_string = ::std::basic_string<char, ::std::char_traits<char>, safe_allocator<char> >;

    using safe_sstream = ::std::basic_stringstream<char, ::std::char_traits<char>,
        safe_allocator<char> >;

    /*class safe_string
    {
    public:
        safe_string()
        {
            _content = nullptr;
            _length = 0;
        }

        ~safe_string()
        {
            reset();
        }

        safe_string(const char* s)
        {
            _length = static_cast<int>(strlen(s));
            fill_string(s, _length);
        }

        safe_string(const std::string& s)
        {
            _length = static_cast<int>(s.length());
            fill_string(s.c_str(), _length);
        }

        safe_string(const safe_string& s)
        {
            _length = s._length;
            fill_string(s.c_str(), _length);
        }


        safe_string(safe_string&& r)
        {
            _length = r._length;
            _content = r._content;

            r._length = 0;
            r._content = nullptr;
        }

        safe_string& operator = (const char* s)
        {
            _length = static_cast<int>(strlen(s));
            fill_string(s, _length);
            return *this;
        }

        safe_string& operator = (const safe_string& obj)
        {
            _length = obj._length;
            fill_string(obj.c_str(), _length);
            return *this;
        }

        safe_string& operator = (safe_string&& obj)
        {
            _length = obj._length;
            _content = obj._content;

            obj._length = 0;
            obj._content = nullptr;

            return *this;
        }

        bool operator == (const safe_string& obj)
        {
            return _length > 0
                && _length == obj._length
                && strcmp(_content, obj._content) == 0;
        }

        bool operator != (const safe_string& obj)
        {
            return !(*this == obj);
        }

        bool is_empty() const
        {
            return _content == nullptr;
        }

        int length() const
        {
            return _length;
        }

        const char* c_str() const
        {
            return _content;
        }

        void clear()
        {
            reset();
        }

    private:
        void fill_string(const char* s, int length)
        {
            _content = (char*)dsn_malloc(length + 1);
            memcpy(_content, s, length + 1);
        }

        void reset()
        {
            if (_content)
                dsn_free(_content);
            _content = nullptr;
            _length = 0;
        }

    private:
        char* _content;
        int   _length;
    };*/
}