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

# if defined(_WIN32)
# include <winsock2.h>
# include <windows.h>
# else
# include <unistd.h>
# endif
# include <dsn/cpp/utils.h>
# include <dsn/cpp/blob.h>
# include <dsn/cpp/address.h>
# include <dsn/cpp/auto_codes.h>
# include <dsn/utility/singleton.h>
# include <cerrno>
# include <chrono>
# include <cstring>
# include <stdexcept>
# include <ctime>
# include <sys/types.h>
# include <sys/stat.h>
# include <random>

# if defined(DSN_USE_THRIFT_SERIALIZATION)
# include <dsn/cpp/serialization_helper/thrift_helper.h>
# endif

# if defined(__linux__)
# include <sys/syscall.h>
# elif defined(__FreeBSD__)
# include <sys/thr.h>
# elif defined(__APPLE__)
# include <pthread.h>
# endif

# if defined(__TITLE__)
# undef __TITLE__
# endif
# define __TITLE__ "dsn.utils"

namespace dsn {
    namespace utils {

        __thread tls_tid s_tid;
        int get_current_tid_internal()
        {
# if defined(_WIN32)
            return static_cast<int>(::GetCurrentThreadId());
# elif defined(__linux__)
            //return static_cast<int>(gettid());
            return static_cast<int>(syscall(SYS_gettid));
# elif defined(__FreeBSD__)
            long lwpid;
            thr_self(&lwpid);
            return static_cast<int>(lwpid);
# elif defined(__APPLE__)
            return static_cast<int>(pthread_mach_thread_np(pthread_self()));
# else
# error not implemented yet
# endif 
        }

        std::string get_last_component(const std::string& input, const char splitters[])
        {
            if (splitters == nullptr)
            {
                derror("get_last_component got null splitters");
                return input;
            }

            int index = -1;
            const char* s = splitters;

            while (*s != 0)
            {
                auto pos = input.find_last_of(*s);
                if (pos != std::string::npos && (static_cast<int>(pos) > index))
                    index = static_cast<int>(pos);
                s++;
            }

            if (index != -1)
                return input.substr(index + 1);
            else
                return input;
        }

        void split_args(const char* args, /*out*/ std::vector<std::string>& sargs, char splitter)
        {
            sargs.clear();
            if (args == nullptr)
            {
                derror("split_args got null args");
                return;
            }

            std::string v(args);

            int lastPos = 0;
            while (true)
            {
                auto pos = v.find(splitter, lastPos);
                if (pos != std::string::npos)
                {
                    std::string s = v.substr(lastPos, pos - lastPos);
                    if (s.length() > 0)
                    {
                        std::string s2 = trim_string((char*)s.c_str());
                        if (s2.length() > 0)
                            sargs.push_back(s2);
                    }
                    lastPos = static_cast<int>(pos + 1);
                }
                else
                {
                    std::string s = v.substr(lastPos);
                    if (s.length() > 0)
                    {
                        std::string s2 = trim_string((char*)s.c_str());
                        if (s2.length() > 0)
                            sargs.push_back(s2);
                    }
                    break;
                }
            }
        }

        void split_args(const char* args, /*out*/ safe_vector<safe_string>& sargs, char splitter)
        {
            sargs.clear();
            if (args == nullptr)
            {
                derror("split_args got null args");
                return;
            }

            safe_string v(args);

            int lastPos = 0;
            while (true)
            {
                auto pos = v.find(splitter, lastPos);
                if (pos != safe_string::npos)
                {
                    safe_string s = v.substr(lastPos, pos - lastPos);
                    if (s.length() > 0)
                    {
                        safe_string s2 = trim_string((char*)s.c_str());
                        if (s2.length() > 0)
                            sargs.push_back(s2);
                    }
                    lastPos = static_cast<int>(pos + 1);
                }
                else
                {
                    safe_string s = v.substr(lastPos);
                    if (s.length() > 0)
                    {
                        safe_string s2 = trim_string((char*)s.c_str());
                        if (s2.length() > 0)
                            sargs.push_back(s2);
                    }
                    break;
                }
            }
        }
        
        void split_args(const char* args, /*out*/ std::list<std::string>& sargs, char splitter)
        {
            sargs.clear();
            if (args == nullptr)
            {
                derror("split_args got null args");
                return;
            }

            std::string v(args);

            int lastPos = 0;
            while (true)
            {
                auto pos = v.find(splitter, lastPos);
                if (pos != std::string::npos)
                {
                    std::string s = v.substr(lastPos, pos - lastPos);
                    if (s.length() > 0)
                    {
                        std::string s2 = trim_string((char*)s.c_str());
                        if (s2.length() > 0)
                            sargs.push_back(s2);
                    }
                    lastPos = static_cast<int>(pos + 1);
                }
                else
                {
                    std::string s = v.substr(lastPos);
                    if (s.length() > 0)
                    {
                        std::string s2 = trim_string((char*)s.c_str());
                        if (s2.length() > 0)
                            sargs.push_back(s2);
                    }
                    break;
                }
            }
        }

        void split_args(const char* args, /*out*/ safe_list<safe_string>& sargs, char splitter)
        {
            sargs.clear();
            if (args == nullptr)
            {
                derror("split_args got null args");
                return;
            }

            safe_string v(args);

            int lastPos = 0;
            while (true)
            {
                auto pos = v.find(splitter, lastPos);
                if (pos != safe_string::npos)
                {
                    safe_string s = v.substr(lastPos, pos - lastPos);
                    if (s.length() > 0)
                    {
                        safe_string s2 = trim_string((char*)s.c_str());
                        if (s2.length() > 0)
                            sargs.push_back(s2);
                    }
                    lastPos = static_cast<int>(pos + 1);
                }
                else
                {
                    safe_string s = v.substr(lastPos);
                    if (s.length() > 0)
                    {
                        safe_string s2 = trim_string((char*)s.c_str());
                        if (s2.length() > 0)
                            sargs.push_back(s2);
                    }
                    break;
                }
            }
        }
        
        std::string replace_string(std::string subject, const std::string& search, const std::string& replace) 
        {
            // Fast path for inputs that cannot change the result. An empty
            // `search` matches at every position, so the loop below would never
            // terminate (empty `replace` -> infinite loop; non-empty `replace`
            // -> unbounded growth / memory exhaustion). An empty `subject` has
            // nothing to scan. In both cases the answer is `subject` unchanged.
            // NOTE: an empty `replace` is deliberately NOT short-circuited here
            // -- it means "delete every occurrence of search", so it must still
            // go through the loop.
            if (search.empty() || subject.empty())
            {
                return subject;
            }

            size_t pos = 0;
            while ((pos = subject.find(search, pos)) != std::string::npos) {
                subject.replace(pos, search.length(), replace);
                pos += replace.length();
            }
            return subject;
        }

        char* trim_string(char* s)
        {
            if (s == nullptr)
            {
                derror("trim_string got null string");
                return nullptr;
            }

            while (*s != '\0' && (*s == ' ' || *s == '\t')) { s++; }
            char* r = s;
            s += strlen(s);
            while (s >= r && (*s == '\0' || *s == ' ' || *s == '\t' || *s == '\r' || *s == '\n')) { *s = '\0';  s--; }
            return r;
        }

        uint64_t get_current_physical_time_ns()
        {
            auto now = std::chrono::high_resolution_clock::now();
            auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
            return nanos;
        }

        void time_ms_to_string(uint64_t ts_ms, char* str, size_t str_size)
        {
            static const size_t kTimeStringBufferSize = sizeof("00:00:00.000");

            if (str == nullptr)
            {
                derror("time_ms_to_string got null output string");
                return;
            }

            if (str_size < kTimeStringBufferSize)
            {
                derror("time_ms_to_string got insufficient output string size = %zu", str_size);
                if (str_size > 0)
                {
                    str[0] = '\0';
                }
                return;
            }

            auto t = (time_t)(ts_ms / 1000);
# if defined(_WIN32)
            auto ret = localtime(&t);
# else
            struct tm tmp;
            auto ret = localtime_r(&t, &tmp);
# endif
            if (ret == nullptr)
            {
                if (str_size > 0)
                {
                    str[0] = '\0';
                }
                return;
            }
            auto ms = static_cast<uint32_t>(ts_ms % 1000);

            int len = snprintf(str, str_size, "%02d:%02d:%02d.%03u", ret->tm_hour, ret->tm_min, ret->tm_sec, ms);
            dassert(len >= 0 && static_cast<size_t>(len) < str_size, "time string buffer is too small");
        }
    }
}

namespace dsn
{
    namespace utils
    {
        namespace asio
        {
            std::string host_name()
            {
                char name[1024];

                int result = ::gethostname(name, sizeof(name));
                if (result != 0)
                {
                    derror("gethostname failed, err = %s", strerror(errno));
                    return std::string();
                }
                name[sizeof(name) - 1] = '\0';
                return std::string(name);
            }
        }
    }
}

namespace dsn
{
    binary_reader::binary_reader(const blob& blob)
    {
        init(blob);
    }

    void binary_reader::init(const blob& bb)
    {
        _blob = bb;
        _size = bb.length();
        _ptr = bb.data();
        _remaining_size = _size;
    }

    int binary_reader::read(/*out*/ std::string& s)
    {
        int len;
        int ret = read(len);
        if (0 == ret)
        {
            // something error happened, return 0 to indicate error.
            return 0;
        }

        // now ret should be sizeof(len).
        if (len < 0)
        {
            throw std::invalid_argument("binary_reader::read got negative string length");
        }
        else if (len == 0)
        {
            s.clear();
        }
        else if (len <= get_remaining_size())
        {
            s.resize(len, 0);
            // read should not fail since we have checked the size, so we can directly return the size of string read.
            // Even if read fails, it returns 0, therefore, ret actually unchanges, which is also correct.
            ret += read((char*)&s[0], len);
        }
        else
        {
            throw std::out_of_range("binary_reader::read string beyond the end of buffer");
        }
        
        return ret;
    }

    int binary_reader::read(blob& blob)
    {
        int len;
        int ret = read(len);
        if (0 == ret)
        {
            // something error happened, return 0 to indicate error.
            return 0;
        }

        // now ret should be sizeof(len).
        if (len < 0)
        {
            throw std::invalid_argument("binary_reader::read got negative blob length");
        }
        else if (len == 0)
        {
            blob = ::dsn::blob();
        }
        else if (len <= get_remaining_size())
        {
            blob = _blob.range(static_cast<int>(_ptr - _blob.data()), len);

            // optimization: zero-copy
            if (!blob.buffer_ptr())
            {
                std::shared_ptr<char> buffer(::dsn::make_shared_array<char>(len));
                memcpy(buffer.get(), blob.data(), blob.length());
                blob = ::dsn::blob(buffer, 0, blob.length());
            }
            
            _ptr += len;
            _remaining_size -= len;
            ret += len;
        }
        else
        {
            throw std::out_of_range("binary_reader::read blob beyond the end of buffer");
        }

        return ret;
    }

    int binary_reader::read(char* buffer, int sz)
    {
        if (sz < 0)
        {
            throw std::invalid_argument("binary_reader::read got negative size");
        }
        else if (sz == 0)
        {
            // 0 bytes read.
            return 0;
        }
        else if (sz <= get_remaining_size())
        {
            if (buffer == nullptr)
            {
                throw std::invalid_argument("binary_reader::read got null buffer");
            }

            memcpy((void*)buffer, _ptr, sz);
            _ptr += sz;
            _remaining_size -= sz;
            return sz;
        }
        else
        {
            throw std::out_of_range("binary_reader::read beyond the end of buffer");
        }
    }

    bool binary_reader::next(const void** data, int* size)
    {
        dassert(data != nullptr && size != nullptr, "binary_reader::next got null output parameter");

        if (get_remaining_size() > 0)
        {
            *data = (const void*)_ptr;
            *size = _remaining_size;

            _ptr += _remaining_size;
            _remaining_size = 0;
            return true;
        }
        else
            return false;
    }

    bool binary_reader::backup(int count)
    {
        if (0 <= count && count <= static_cast<int>(_ptr - _blob.data()))
        {
            // count could be 0. In this case, we return true without changing _ptr and _remaining_size.
            _ptr -= count;
            _remaining_size += count;
            return true;
        }
        else
            return false;
    }

    bool binary_reader::skip(int count)
    {
        if (count < 0)
        {
            throw std::invalid_argument("binary_reader::skip got negative count");
        }
        else if (count <= get_remaining_size())
        {
            // count could be 0. In this case, we return true without changing _ptr and _remaining_size.
            _ptr += count;
            _remaining_size -= count;
            return true;
        }
        else
        {
            throw std::out_of_range("read beyond the end of buffer");
        }
    }

    int binary_writer::_reserved_size_per_buffer_static = 256;                

    binary_writer::binary_writer(int reserveBufferSize)
    {
        if (reserveBufferSize < 0)
        {
            throw std::invalid_argument("binary_writer got negative reserve buffer size");
        }
        _total_size = 0;
        _buffers.reserve(1);
        _reserved_size_per_buffer =
            (reserveBufferSize == 0) ? _reserved_size_per_buffer_static : reserveBufferSize;
        _current_buffer = nullptr;
        _current_offset = 0;
        _current_buffer_length = 0;
    }

    binary_writer::binary_writer(blob& buffer)
    {
        _total_size = 0;
        _buffers.reserve(1);
        _reserved_size_per_buffer = _reserved_size_per_buffer_static;

        _buffers.push_back(buffer);
        _current_buffer = (char*)buffer.data();
        _current_offset = 0;
        _current_buffer_length = buffer.length();
    }

    binary_writer::~binary_writer()
    {
    }

    void binary_writer::flush()
    {
        commit();
    }

    void binary_writer::create_buffer(size_t size)
    {
        commit();

        blob bb;
        create_new_buffer(size, bb);
        _buffers.push_back(bb);

        _current_buffer = (char*)bb.data();        
        _current_buffer_length = bb.length();
    }

    void binary_writer::create_new_buffer(size_t size, /*out*/blob& bb)
    {
        bb.assign(::dsn::make_shared_array<char>(size), 0, (int)size);
    }

    void binary_writer::commit()
    {
        if (_current_offset > 0)
        {
            *_buffers.rbegin() = _buffers.rbegin()->range(0, _current_offset);

            _current_offset = 0;
            _current_buffer_length = 0;
        }
    }
    
    blob binary_writer::get_buffer()
    {
        commit();

        if (_buffers.size() == 1)
        {
            return _buffers[0];
        }
        else if (_total_size == 0)
        {
            return blob();
        }
        else
        {
            std::shared_ptr<char> bptr(::dsn::make_shared_array<char>(_total_size));
            blob bb(bptr, _total_size);
            const char* ptr = bb.data();

            for (int i = 0; i < static_cast<int>(_buffers.size()); i++)
            {
                memcpy((void*)ptr, (const void*)_buffers[i].data(), (size_t)_buffers[i].length());
                ptr += _buffers[i].length();
            }
            return bb;
        }
    }

    blob binary_writer::get_current_buffer()
    {
        if (_buffers.size() == 1)
        {
            return _current_offset > 0 ? _buffers[0].range(0, _current_offset) : _buffers[0];
        }
        else
        {
            std::shared_ptr<char> bptr(::dsn::make_shared_array<char>(_total_size));
            blob bb(bptr, _total_size);
            const char* ptr = bb.data();

            for (int i = 0; i < static_cast<int>(_buffers.size()); i++)
            {
                size_t len = (size_t)_buffers[i].length();
                if (_current_offset > 0 && i + 1 == (int)_buffers.size())
                {
                    len = _current_offset;
                }

                memcpy((void*)ptr, (const void*)_buffers[i].data(), len);
                ptr += _buffers[i].length();
            }
            return bb;
        }
    }

    void binary_writer::write_empty(int sz)
    {
        if (sz < 0)
        {
            throw std::invalid_argument("binary_writer::write_empty got negative size");
        }
        if (sz == 0)
        {
            return;
        }

        int sz0 = sz;
        int rem_size = _current_buffer_length - _current_offset;
        if (rem_size >= sz)
        {
            _current_offset += sz;
        }
        else
        {
            _current_offset += rem_size;
            sz -= rem_size;

            int allocSize = _reserved_size_per_buffer;
            if (sz > allocSize)
                allocSize = sz;

            create_buffer(allocSize);
            if (_current_buffer == nullptr || _current_buffer_length <= 0)
            {
                throw std::runtime_error("binary_writer::write_empty failed to create buffer");
            }
            _current_offset += sz;
        }

        _total_size += sz0;
    }

    void binary_writer::write(const char* buffer, int sz)
    {
        if (sz < 0)
        {
            throw std::invalid_argument("binary_writer::write got negative size");
        }
        if (sz == 0)
        {
            return;
        }
        if (buffer == nullptr)
        {
            throw std::invalid_argument("binary_writer::write got null buffer");
        }

        int rem_size = _current_buffer_length - _current_offset;
        if (rem_size >= sz)
        {
            memcpy((void*)(_current_buffer + _current_offset), buffer, (size_t)sz);
            _current_offset += sz;
            _total_size += sz;
        }
        else
        {
            if (rem_size > 0)
            {
                memcpy((void*)(_current_buffer + _current_offset), buffer, (size_t)rem_size);
                _current_offset += rem_size;
                _total_size += rem_size;
                sz -= rem_size;
            }            

            int allocSize = _reserved_size_per_buffer;
            if (sz > allocSize)
                allocSize = sz;

            create_buffer(allocSize);
            if (_current_buffer == nullptr || _current_buffer_length <= 0)
            {
                throw std::runtime_error("binary_writer::write failed to create buffer");
            }
            memcpy((void*)(_current_buffer + _current_offset), buffer + rem_size, (size_t)sz);
            _current_offset += sz;
            _total_size += sz;
        }
    }

    bool binary_writer::next(void** data, int* size)
    {
        if (data == nullptr || size == nullptr)
        {
            throw std::invalid_argument("binary_writer::next got null output parameter");
        }

        int rem_size = _current_buffer_length - _current_offset;
        if (rem_size == 0)
        {
            create_buffer(_reserved_size_per_buffer);
            rem_size = _current_buffer_length;
            if (_current_buffer == nullptr || rem_size <= 0)
            {
                throw std::runtime_error("binary_writer::next failed to create buffer");
            }
        }

        *size = rem_size;
        *data = (void*)(_current_buffer + _current_offset);
        _current_offset = _current_buffer_length;
        _total_size += rem_size;
        return true;
    }

    bool binary_writer::backup(int count)
    {
        if (count < 0 || count > _current_offset)
        {
            throw std::invalid_argument("currently we don't support backup before the last buffer's header");
        }
        _current_offset -= count;
        _total_size -= count;
        return true;
    }
} // end namespace dsn
