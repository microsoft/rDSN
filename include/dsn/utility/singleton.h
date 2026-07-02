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

# include <mutex>
# include <atomic>

namespace dsn { namespace utils {

template<typename T>
class singleton
{
public:
    singleton() {}

    static T& instance()
    {
        T* instance = _instance.load(std::memory_order_acquire);
        if (nullptr == instance)
        {
            // lock 
            while (0 != _l.exchange(1, std::memory_order_acquire))
            {
                while (_l.load(std::memory_order_acquire) == 1)
                {
                }
            }

            // Release _l on every exit path -- including when T's constructor
            // throws (e.g. std::bad_alloc under memory pressure). Otherwise the
            // lock stays held forever and any later instance() call spins on _l
            // for good. That is especially dangerous for a re-entrant call on
            // this same thread -- e.g. the failure is reported through a path
            // that logs, and logging itself calls instance() -- which would
            // self-deadlock. The guard turns the failure into a clean, retryable
            // throw (fail-stop) instead of a hang.
            struct unlocker
            {
                std::atomic<int>& l;
                ~unlocker() { l.store(0, std::memory_order_release); }
            } unlock_guard{_l};

            // re-check and assign
            instance = _instance.load(std::memory_order_acquire);
            if (nullptr == instance)
            {
                instance = new T();
                _instance.store(instance, std::memory_order_release);
            }
        }
        return *instance;
    }

    static T& fast_instance()
    {
        return *_instance.load(std::memory_order_acquire);
    }

    static bool is_instance_created()
    {
        return nullptr != _instance.load(std::memory_order_acquire);
    }
    
protected:
    static std::atomic<T*> _instance;
    static std::atomic<int> _l;
    
private:
    singleton(const singleton&);
    singleton& operator=(const singleton&);
};

// ----- inline implementations -------------------------------------------------------------------

template<typename T> std::atomic<T*>  singleton<T>::_instance(nullptr);
template<typename T> std::atomic<int>  singleton<T>::_l(0);

}} // end namespace dsn::utils
