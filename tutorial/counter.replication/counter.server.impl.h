/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Microsoft Corporation
 * 
 * -=- Robust Distributed System Nucleus(rDSN) -=- 
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

# pragma once

# include "counter.server.h"
# include <dsn/cpp/replicated_service_app.h>

namespace dsn {
    namespace example {        
        class counter_service_impl :
            public counter_service,
            public replicated_service_app_type_1
        {
        public:
            counter_service_impl(dsn_gpid gpid);

            virtual void on_add(const ::dsn::example::count_op& op, ::dsn::rpc_replier<int32_t>& reply) override;
            virtual void on_read(const std::string& name, ::dsn::rpc_replier<int32_t>& reply) override;

            virtual ::dsn::error_code start(int argc, char** argv) override;

            virtual ::dsn::error_code stop(bool cleanup = false) override;

            virtual ::dsn::error_code sync_checkpoint(int64_t last_commit) override;

            virtual int64_t get_last_checkpoint_decree() override { return last_durable_decree(); }

            virtual ::dsn::error_code get_checkpoint(
                int64_t learn_start,
                int64_t local_commit,
                void*   learn_request,
                int     learn_request_size,
                app_learn_state& state
            ) override;

            virtual ::dsn::error_code apply_checkpoint(
                dsn_chkpt_apply_mode mode,
                int64_t local_commit,
                const dsn_app_learn_state& state
            ) override;

        private:
            void recover();
            void recover(const std::string& name, int64_t version);
            const char* data_dir() const { return _data_dir.c_str(); }
            int64_t last_durable_decree() const { return _last_durable_decree; }
            void set_last_durable_decree(int64_t d) { _last_durable_decree = d; }

        private:
            ::dsn::service::zlock _lock;
            std::map<std::string, int32_t> _counters;
            bool      _test_file_learning;

            std::string _data_dir;
            int64_t     _last_durable_decree;
        };

    }
}

