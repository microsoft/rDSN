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

#include "simple_kv.server.impl.h"
#include <dsn/cpp/utils.h>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cstring>
#include <utility>

# ifdef __TITLE__
# undef __TITLE__
# endif
# define __TITLE__ "simple.kv"

using namespace ::dsn::service;

namespace dsn {
    namespace replication {
        namespace application {

            static bool parse_checkpoint_version(const std::string& name, int64_t& version)
            {
                const char *prefix = "checkpoint.";
                if (name.substr(0, strlen(prefix)) != std::string(prefix))
                {
                    return false;
                }

                if (!::dsn::utils::lexical_cast_integer<int64_t>(name.substr(strlen(prefix)), version))
                {
                    return false;
                }

                return true;
            }

            static bool read_checkpoint_exact(std::ifstream& is, char* buffer, size_t size, size_t& remaining)
            {
                if (remaining < size)
                {
                    return false;
                }

                is.read(buffer, size);
                if (!is)
                {
                    return false;
                }

                remaining -= size;
                return true;
            }

            static bool read_checkpoint_string(std::ifstream& is, std::string& value, size_t& remaining)
            {
                uint32_t sz;
                if (!read_checkpoint_exact(is, reinterpret_cast<char*>(&sz), sizeof(sz), remaining))
                {
                    return false;
                }

                if (remaining < static_cast<size_t>(sz))
                {
                    return false;
                }

                if (sz == 0)
                {
                    value.clear();
                    return true;
                }

                value.resize(sz);
                return read_checkpoint_exact(is, &value[0], sz, remaining);
            }
            
            simple_kv_service_impl::simple_kv_service_impl(dsn_gpid gpid)
                : ::dsn::replicated_service_app_type_1(gpid), _lock(true)
            {
                _test_file_learning = false;
                _last_durable_decree = 0;
            }

            // RPC_SIMPLE_KV_READ
            void simple_kv_service_impl::on_read(const std::string& key, ::dsn::rpc_replier<std::string>& reply)
            {
                std::string r;
                {
                    zauto_lock l(_lock);

                    auto it = _store.find(key);
                    if (it != _store.end())
                    {
                        r = it->second;
                    }
                }
             
                dinfo("read %s", r.c_str());
                reply(r);
            }

            // RPC_SIMPLE_KV_WRITE
            void simple_kv_service_impl::on_write(const kv_pair& pr, ::dsn::rpc_replier<int32_t>& reply)
            {
                {
                    zauto_lock l(_lock);
                    _store[pr.key] = pr.value;
                }                

                dinfo("write %s", pr.key.c_str());
                reply(0);
            }

            // RPC_SIMPLE_KV_APPEND
            void simple_kv_service_impl::on_append(const kv_pair& pr, ::dsn::rpc_replier<int32_t>& reply)
            {
                {
                    zauto_lock l(_lock);
                    auto it = _store.find(pr.key);
                    if (it != _store.end())
                        it->second.append(pr.value);
                    else
                        _store[pr.key] = pr.value;
                }

                dinfo("append %s", pr.key.c_str());
                reply(0);
            }
            
            ::dsn::error_code simple_kv_service_impl::start(int argc, char** argv)
            {
                _data_dir = dsn_get_app_data_dir(get_gpid());

                {
                    zauto_lock l(_lock);
                    set_last_durable_decree(0);
                    auto err = recover();
                    if (err != ERR_OK)
                    {
                        return err;
                    }
                }

                open_service(get_gpid());
                return ERR_OK;
            }

            ::dsn::error_code simple_kv_service_impl::stop(bool clear_state)
            {
                close_service(get_gpid());

                {
                    zauto_lock l(_lock);
                    if (clear_state)
                    {
                        dsn_get_app_data_dir(get_gpid());

                        if (!dsn::utils::filesystem::remove_path(data_dir()))
                        {
                            // A cleanup failure (directory busy, permission, transient FS
                            // error) must not abort the process during shutdown; log and
                            // continue so stop() completes.
                            derror("Fail to delete directory %s.", data_dir());
                        }
                    }
                }
                
                return ERR_OK;
            }

            // checkpoint related
            // Fault injection showed checkpoint files can be left truncated or malformed when
            // writes fail. Do not trust the newest checkpoint blindly: scan checkpoint candidates
            // from newest to oldest, validate the complete file before loading it into _store, and
            // only report corruption when every checkpoint candidate is invalid.
            ::dsn::error_code simple_kv_service_impl::recover()
            {
                zauto_lock l(_lock);

                _store.clear();

                std::vector<std::string> sub_list;
                std::string path = data_dir();
                if (!dsn::utils::filesystem::get_subfiles(path, sub_list, false))
                {
                    derror("Fail to get subfiles in %s.", path.c_str());
                    return ERR_FILE_OPERATION_FAILED;
                }
                if (sub_list.empty())
                {
                    ddebug("simple_kv_service_impl found no checkpoint files in %s.", path.c_str());
                    return ERR_OK;
                }

                typedef std::pair<int64_t, std::string> checkpoint_info;
                std::vector<checkpoint_info> checkpoints;
                for (auto& fpath : sub_list)
                {
                    auto&& s = dsn::utils::filesystem::get_file_name(fpath);
                    int64_t version = 0;
                    if (!parse_checkpoint_version(s, version))
                    {
                        continue;
                    }

                    checkpoints.push_back(
                        checkpoint_info(version, std::string(data_dir()) + "/" + s));
                }
                sub_list.clear();
                if (checkpoints.empty())
                {
                    ddebug("simple_kv_service_impl found no checkpoint files in %s.", path.c_str());
                    return ERR_OK;
                }

                std::sort(checkpoints.rbegin(), checkpoints.rend());
                for (const auto &checkpoint : checkpoints)
                {
                    if (recover(checkpoint.second, checkpoint.first))
                    {
                        set_last_durable_decree(checkpoint.first);
                        return ERR_OK;
                    }

                    derror("simple_kv_service_impl ignored invalid checkpoint %s",
                           checkpoint.second.c_str());
                }

                return ERR_CORRUPTION;
            }

            bool simple_kv_service_impl::recover(const std::string& name, int64_t version)
            {
                zauto_lock l(_lock);

                int64_t file_size = 0;
                if (!dsn::utils::filesystem::file_size(name, file_size) || file_size < 0)
                {
                    derror("simple_kv_service_impl get checkpoint file size failed: %s",
                           name.c_str());
                    return false;
                }
                size_t remaining = static_cast<size_t>(file_size);

                std::ifstream is(name.c_str(), std::ios::binary);
                if (!is.is_open())
                {
                    derror("simple_kv_service_impl open checkpoint failed: %s", name.c_str());
                    return false;
                }
                
                simple_kv store;

                uint64_t count;
                int magic;
                
                if (!read_checkpoint_exact(is, reinterpret_cast<char *>(&count), sizeof(count), remaining) ||
                    !read_checkpoint_exact(is, reinterpret_cast<char *>(&magic), sizeof(magic), remaining) ||
                    magic != 0xdeadbeef)
                {
                    derror("simple_kv_service_impl invalid checkpoint header: %s", name.c_str());
                    return false;
                }

                for (uint64_t i = 0; i < count; i++)
                {
                    std::string key;
                    std::string value;

                    if (!read_checkpoint_string(is, key, remaining) ||
                        !read_checkpoint_string(is, value, remaining))
                    {
                        derror("simple_kv_service_impl invalid checkpoint body: %s",
                               name.c_str());
                        return false;
                    }

                    store[key] = value;
                }

                _store.swap(store);
                return true;
            }

            ::dsn::error_code simple_kv_service_impl::sync_checkpoint(int64_t last_commit)
            {
                char name[256];
                int len = snprintf(name, sizeof(name), "%s/checkpoint.%" PRId64, data_dir(), last_commit);
                if (len < 0 || static_cast<size_t>(len) >= sizeof(name))
                {
                    derror("checkpoint path is too long for data dir %s", data_dir());
                    return ERR_FILE_OPERATION_FAILED;
                }

                zauto_lock l(_lock);

                if (last_commit == last_durable_decree())
                {
                    dassert(utils::filesystem::file_exists(name), 
                        "checkpoint file %s is missing!",
                        name
                        );
                    return ERR_OK;
                }

                std::string tmp_name = std::string(name) + ".tmp";
                dsn::utils::filesystem::remove_path(tmp_name);
                std::ofstream os(tmp_name.c_str(), std::ios::binary | std::ios::trunc);
                if (!os.is_open())
                {
                    derror("simple_kv_service_impl open checkpoint failed: %s",
                           tmp_name.c_str());
                    return ERR_CHECKPOINT_FAILED;
                }

                uint64_t count = (uint64_t)_store.size();
                int magic = 0xdeadbeef;
                
                os.write((const char*)&count, (uint32_t)sizeof(count));
                os.write((const char*)&magic, (uint32_t)sizeof(magic));

                for (auto it = _store.begin(); it != _store.end(); ++it)
                {
                    const std::string& k = it->first;
                    uint32_t sz = (uint32_t)k.length();

                    os.write((const char*)&sz, (uint32_t)sizeof(sz));
                    if (sz > 0)
                    {
                        os.write((const char*)&k[0], sz);
                    }

                    const std::string& v = it->second;
                    sz = (uint32_t)v.length();

                    os.write((const char*)&sz, (uint32_t)sizeof(sz));
                    if (sz > 0)
                    {
                        os.write((const char*)&v[0], sz);
                    }
                }
                
                os.flush();
                bool write_succeed = os.good();
                os.close();
                write_succeed = write_succeed && os.good();
                if (!write_succeed)
                {
                    derror("simple_kv_service_impl write checkpoint failed: %s",
                           tmp_name.c_str());
                    dsn::utils::filesystem::remove_path(tmp_name);
                    return ERR_CHECKPOINT_FAILED;
                }

                if (!utils::filesystem::rename_path(tmp_name, name))
                {
                    derror("simple_kv_service_impl publish checkpoint failed: %s", name);
                    dsn::utils::filesystem::remove_path(tmp_name);
                    return ERR_CHECKPOINT_FAILED;
                }

                // TODO: gc checkpoints
                set_last_durable_decree(last_commit);
                return ERR_OK;
            }

            // helper routines to accelerate learning
            ::dsn::error_code simple_kv_service_impl::get_checkpoint(
                int64_t learn_start,
                int64_t local_commit,
                void*   learn_request,
                int     learn_request_size,
                app_learn_state& state)
            {
                if (last_durable_decree() > 0)
                {
                    char name[256];
                    int len = snprintf(name, sizeof(name), "%s/checkpoint.%" PRId64,
                        data_dir(),
                        last_durable_decree()
                        );
                    if (len < 0 || static_cast<size_t>(len) >= sizeof(name))
                    {
                        derror("checkpoint path is too long for data dir %s", data_dir());
                        return ERR_FILE_OPERATION_FAILED;
                    }
                    
                    state.from_decree_excluded = 0;
                    state.to_decree_included = last_durable_decree();
                    state.files.push_back(std::string(name));
                    return ERR_OK;
                }
                else
                {
                    state.from_decree_excluded = 0;
                    state.to_decree_included = 0;
                    return ERR_OBJECT_NOT_FOUND;
                }
            }

            ::dsn::error_code simple_kv_service_impl::apply_checkpoint(
                dsn_chkpt_apply_mode mode,
                int64_t commit,
                const dsn_app_learn_state& state)
            {
                if (mode == DSN_CHKPT_LEARN)
                {
                    if (state.file_state_count <= 0 ||
                        !recover(state.files[0], state.to_decree_included))
                    {
                        derror("simple_kv_service_impl learn checkpoint failed");
                        return ERR_CHECKPOINT_FAILED;
                    }

                    return ERR_OK;
                }
                else
                {
                    dassert(DSN_CHKPT_COPY == mode, "invalid mode %d", (int)mode);

                    // The COPY branch dereferences state.files[0] below. The sibling LEARN
                    // branch above guards file_state_count <= 0 before touching files[0];
                    // do the same here so an empty/malformed learn state is rejected instead
                    // of indexing a null/absent file entry (out-of-bounds / null deref).
                    if (state.file_state_count <= 0)
                    {
                        derror("simple_kv_service_impl copy checkpoint failed: no checkpoint files provided");
                        return ERR_CHECKPOINT_FAILED;
                    }

                    // A checkpoint whose decree is not newer than the current durable state
                    // is a recoverable protocol/ordering condition, not an internal invariant.
                    // Reject it gracefully rather than aborting the whole process.
                    if (state.to_decree_included <= last_durable_decree())
                    {
                        derror("simple_kv_service_impl copy checkpoint failed: checkpoint decree %" PRId64
                               " is not greater than current durable decree %" PRId64,
                               state.to_decree_included, last_durable_decree());
                        return ERR_CHECKPOINT_FAILED;
                    }

                    char name[256];
                    int len = snprintf(name, sizeof(name), "%s/checkpoint.%" PRId64,
                        data_dir(),
                        state.to_decree_included
                        );
                    if (len < 0 || static_cast<size_t>(len) >= sizeof(name))
                    {
                        derror("checkpoint path is too long for data dir %s", data_dir());
                        return ERR_FILE_OPERATION_FAILED;
                    }
                    std::string lname(name);

                    if (!utils::filesystem::rename_path(state.files[0], lname))
                        return ERR_CHECKPOINT_FAILED;
                    else
                    {
                        set_last_durable_decree(state.to_decree_included);
                        return ERR_OK;
                    }
                }
            }

        }
    }
} // namespace
