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
#pragma once

# ifdef _WIN32

# include <dsn/tool_api.h>
# include <dsn/internal/synchronize.h>

namespace dsn {
    namespace tools {
        class native_win_aio_provider : public aio_provider
        {
        public:
            native_win_aio_provider(disk_engine* disk, aio_provider* inner_provider);
            ~native_win_aio_provider();

            virtual dsn_handle_t open(const char* file_name, int flag, int pmode);
            virtual error_code close(dsn_handle_t hFile);
            virtual void    aio(aio_task* aio);            
            virtual disk_aio_ptr prepare_aio_context(aio_task* tsk);

        protected:
            error_code aio_internal(aio_task* aio, bool async, __out_param uint32_t* pbytes = nullptr);

        private:
            void worker();
            std::thread *_worker_thr;
            HANDLE       _iocp;
        };
    }
}

# endif