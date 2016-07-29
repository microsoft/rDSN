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

#pragma once

# include <dsn/service_api_c.h>
# include <memory>
# include <vector>
# include <cstring>

#ifdef DSN_USE_THRIFT_SERIALIZATION
# include <thrift/protocol/TProtocol.h>
#endif

namespace dsn 
{
    template <typename T>
    std::shared_ptr<T> make_shared_array(size_t size)
    {
        return std::shared_ptr<T>(new T[size], std::default_delete<T[]>());
    }

    class blob
    {
    public:
        blob() : _buffer(nullptr), _data(nullptr), _length(0) {}

        blob(const std::shared_ptr<char>& buffer, unsigned int length)
            : _holder(buffer), _buffer(_holder.get()), _data(_holder.get()), _length(length)
        {}

        blob(std::shared_ptr<char>&& buffer, unsigned int length)
            : _holder(std::move(buffer)), _buffer(_holder.get()), _data(_holder.get()), _length(length)
        {}

        blob(const std::shared_ptr<char>& buffer, int offset, unsigned int length)
            : _holder(buffer), _buffer(_holder.get()), _data(_holder.get() + offset), _length(length)
        {}

        blob(std::shared_ptr<char>&& buffer, int offset, unsigned int length)
            : _holder(std::move(buffer)), _buffer(_holder.get()), _data(_holder.get() + offset), _length(length)
        {}

        blob(const char* buffer, int offset, unsigned int length)
            : _buffer(buffer), _data(buffer + offset), _length(length)
        {}

        blob(const blob& source)
            : _holder(source._holder), _buffer(source._buffer), _data(source._data), _length(source._length)
        {}

        blob(blob&& source)
            : _holder(std::move(source._holder)), _buffer(source._buffer), _data(source._data), _length(source._length)
        {
            source._buffer = nullptr;
            source._data = nullptr;
            source._length = 0;
        }

        blob& operator = (const blob& that)
        {
            _holder = that._holder;
            _buffer = that._buffer;
            _data = that._data;
            _length = that._length;
            return *this;
        }

        blob& operator = (blob&& that)
        {
            _holder = std::move(that._holder);
            _buffer = that._buffer;
            _data = that._data;
            _length = that._length;
            that._buffer = nullptr;
            that._data = nullptr;
            that._length = 0;
            return *this;
        }

        void assign(const std::shared_ptr<char>& buffer, int offset, unsigned int length)
        {
            _holder = buffer;
            _buffer = _holder.get();
            _data = _holder.get() + offset;
            _length = length;
        }

        void assign(std::shared_ptr<char>&& buffer, int offset, unsigned int length)
        {
            _holder = std::move(buffer);
            _buffer = (_holder.get());
            _data = (_holder.get() + offset);
            _length = length;
        }

        void assign(const char* buffer, int offset, unsigned int length)
        {
            _holder = nullptr;
            _buffer = buffer;
            _data = buffer + offset;
            _length = length;
        }

        const char* data() const { return _data; }

        unsigned int length() const { return _length; }

        std::shared_ptr<char> buffer() const { return _holder; }

        bool has_holder() const { return _holder.get() != nullptr; }

        const char* buffer_ptr() const { return _holder.get(); }

        // offset can be negative for buffer dereference
        blob range(int offset) const
        {
            dassert(offset <= static_cast<int>(_length), "offset cannot exceed the current length value");

            blob temp = *this;
            temp._data += offset;
            temp._length -= offset;
            return temp;
        }

        blob range(int offset, unsigned int len) const
        {
            dassert(offset <= static_cast<int>(_length), "offset cannot exceed the current length value");

            blob temp = *this;
            temp._data += offset;
            temp._length -= offset;
            dassert(temp._length >= len, "buffer length must exceed the required length");
            temp._length = len;
            return temp;
        }

        bool operator == (const blob& r) const
        {
            dassert(false, "not implemented");
            return false;
        }

#ifdef DSN_USE_THRIFT_SERIALIZATION
        uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
        uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;
#endif
    private:
        friend class binary_writer;
        std::shared_ptr<char>  _holder;
        const char*            _buffer;
        const char*            _data;
        unsigned int           _length; // data length
    };

    class binary_reader
    {
    public:
        // given bb on ctor
        binary_reader(const blob& blob);

        // or delayed init
        binary_reader() {}

        virtual ~binary_reader() {}

        void init(const blob& bb);

        template<typename T> int read_pod(/*out*/ T& val);
        template<typename T> int read(/*out*/ T& val) { dassert(false, "read of this type is not implemented"); return 0; }
        int read(/*out*/ int8_t& val) { return read_pod(val); }
        int read(/*out*/ uint8_t& val) { return read_pod(val); }
        int read(/*out*/ int16_t& val) { return read_pod(val); }
        int read(/*out*/ uint16_t& val) { return read_pod(val); }
        int read(/*out*/ int32_t& val) { return read_pod(val); }
        int read(/*out*/ uint32_t& val) { return read_pod(val); }
        int read(/*out*/ int64_t& val) { return read_pod(val); }
        int read(/*out*/ uint64_t& val) { return read_pod(val); }
        int read(/*out*/ bool& val) { return read_pod(val); }

        int read(/*out*/ std::string& s);
        int read(char* buffer, int sz);
        int read(blob& blob);

        bool next(const void** data, int* size);
        bool skip(int count);
        bool backup(int count);

        blob get_buffer() const { return _blob; }
        blob get_remaining_buffer() const { return _blob.range(static_cast<int>(_ptr - _blob.data())); }
        bool is_eof() const { return _ptr >= _blob.data() + _size; }
        int  total_size() const { return _size; }
        int  get_remaining_size() const { return _remaining_size; }

    private:
        blob        _blob;
        int         _size;
        const char* _ptr;
        int         _remaining_size;
    };

    class binary_writer
    {
    public:
        binary_writer(int reserved_buffer_size = 0);
        binary_writer(blob& buffer);
        virtual ~binary_writer();

        virtual void flush();

        template<typename T> void write_pod(const T& val);
        template<typename T> void write(const T& val) { dassert(false, "write of this type is not implemented"); }
        void write(const int8_t& val) { write_pod(val); }
        void write(const uint8_t& val) { write_pod(val); }
        void write(const int16_t& val) { write_pod(val); }
        void write(const uint16_t& val) { write_pod(val); }
        void write(const int32_t& val) { write_pod(val); }
        void write(const uint32_t& val) { write_pod(val); }
        void write(const int64_t& val) { write_pod(val); }
        void write(const uint64_t& val) { write_pod(val); }
        void write(const bool& val) { write_pod(val); }

        void write(const std::string& val);
        void write(const char* buffer, int sz);
        void write(const blob& val);
        void write_empty(int sz);

        bool next(void** data, int* size);
        bool backup(int count);

        void get_buffers(/*out*/ std::vector<blob>& buffers);
        int  get_buffer_count() const { return static_cast<int>(_buffers.size()); }
        blob get_buffer();
        blob get_current_buffer(); // without commit, write can be continued on the last buffer
        blob get_first_buffer() const;

        int total_size() const { return _total_size; }

    protected:
        // bb may have large space than size
        void create_buffer(size_t size);
        void commit();
        virtual void create_new_buffer(size_t size, /*out*/blob& bb);

    private:
        std::vector<blob>  _buffers;

        char*              _current_buffer;
        int                _current_offset;
        int                _current_buffer_length;

        int                _total_size;
        int                _reserved_size_per_buffer;
        static int         _reserved_size_per_buffer_static;
    };

    //--------------- inline implementation -------------------
    template<typename T>
    inline int binary_reader::read_pod(/*out*/ T& val)
    {
        if (sizeof(T) <= get_remaining_size())
        {
            memcpy((void*)&val, _ptr, sizeof(T));
            _ptr += sizeof(T);
            _remaining_size -= sizeof(T);
            return static_cast<int>(sizeof(T));
        }
        else
        {
            dassert(false, "read beyond the end of buffer");
            return 0;
        }
    }

    template<typename T>
    inline void binary_writer::write_pod(const T& val)
    {
        write((char*)&val, static_cast<int>(sizeof(T)));
    }

    inline void binary_writer::get_buffers(/*out*/ std::vector<blob>& buffers)
    {
        commit();
        buffers = _buffers;
    }

    inline blob binary_writer::get_first_buffer() const
    {
        return _buffers[0];
    }

    inline void binary_writer::write(const std::string& val)
    {
        int len = static_cast<int>(val.length());
        write((const char*)&len, sizeof(int));
        if (len > 0) write((const char*)&val[0], len);
    }

    inline void binary_writer::write(const blob& val)
    {
        // TODO: optimization by not memcpy
        int len = val.length();
        write((const char*)&len, sizeof(int));
        if (len > 0) write((const char*)val.data(), len);
    }
}
