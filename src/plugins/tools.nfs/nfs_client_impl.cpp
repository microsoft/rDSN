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
# include "nfs_client_impl.h"
# include <dsn/cpp/utils.h>
# include <dsn/tool-api/nfs.h>
# include <queue>
# include <utility>
# include <chrono>

namespace dsn {
    namespace service {


        void nfs_client_impl::begin_remote_copy(std::shared_ptr<remote_copy_request>& rci, aio_task* nfs_task)
        {
            user_request* req = new user_request();
            req->file_size_req.source = rci->source;
            req->file_size_req.dst_dir = rci->dest_dir.c_str();
            for (auto& f : rci->files)
            {
                req->file_size_req.file_list.emplace_back(f.c_str());
            }
            req->file_size_req.source_dir = rci->source_dir.c_str();
            req->file_size_req.overwrite = rci->overwrite;
            req->nfs_task = nfs_task;
            req->is_finished = false;

            get_file_size(
                    req->file_size_req,
                    [=](error_code err, get_file_size_response&& resp)
                    {
                        end_get_file_size(err, std::move(resp), req);
                    },
                    std::chrono::milliseconds(0),
                    0,
                    0,
                    0,
                    req->file_size_req.source
                    );
        }

        void nfs_client_impl::end_get_file_size(
            ::dsn::error_code err,
            const ::dsn::service::get_file_size_response& resp,
            void* context)
        {
            user_request* ureq = (user_request*)context;

            if (err != ::dsn::ERR_OK)
            {
                derror("remote copy request failed");
                ureq->nfs_task->enqueue(err, 0);
                delete ureq;
                return;
            }

            err = resp.error;
            if (err != ::dsn::ERR_OK)
            {
                derror("remote copy request failed");
                ureq->nfs_task->enqueue(err, 0);
                delete ureq;
                return;
            }

            // size_list and file_list are parallel arrays in the (remote, untrusted)
            // response, but the loop below iterates size_list while indexing file_list.
            // A malformed or corrupted response with mismatched lengths would otherwise
            // read file_list out of bounds. Validate the lengths match and fail the
            // request through the existing nfs_task error channel.
            if (resp.size_list.size() != resp.file_list.size())
            {
                derror("invalid get file size response: size_list (%d) and file_list (%d) "
                       "have mismatched lengths",
                       (int)resp.size_list.size(),
                       (int)resp.file_list.size());
                ureq->nfs_task->enqueue(ERR_INVALID_DATA, 0);
                delete ureq;
                return;
            }

            if (resp.file_list.empty())
            {
                handle_completion(ureq, ERR_OK);
                return;
            }

            // file_list comes from the (untrusted) remote server and is combined with the local
            // dst_dir below to form the path this client creates and writes. Reject any name that
            // escapes dst_dir (see nfs_path_has_parent_ref) up front -- before any file_context is
            // allocated -- so a malicious server cannot make the client create or overwrite files
            // outside the destination directory.
            for (size_t i = 0; i < resp.file_list.size(); i++)
            {
                if (nfs_path_has_parent_ref(resp.file_list[i]))
                {
                    derror("nfs: rejecting get_file_size response with parent-directory reference in file name '%s'",
                        resp.file_list[i].c_str());
                    ureq->nfs_task->enqueue(ERR_INVALID_DATA, 0);
                    delete ureq;
                    return;
                }

                // size_list is a list<i64> filled in by the (untrusted) remote server. Below it is
                // assigned to an unsigned counter (uint64_t size = resp.size_list[i]) that drives a
                // loop pre-creating one copy_request_ex per nfs_copy_block_bytes block. A negative
                // size wraps to a huge unsigned value, and an absurdly large positive size needs an
                // enormous number of blocks, so either would make the client allocate an unbounded
                // number of request objects (bad_alloc -> std::terminate) from a single small
                // response. Reject a negative size, or one that would exceed the configured
                // per-file block-count limit, through the existing nfs_task error channel.
                int64_t fsize = resp.size_list[i];
                if (fsize < 0)
                {
                    derror("nfs: rejecting get_file_size response with negative size %lld for file '%s'",
                        (long long)fsize, resp.file_list[i].c_str());
                    ureq->nfs_task->enqueue(ERR_INVALID_DATA, 0);
                    delete ureq;
                    return;
                }
                if (_opts.nfs_copy_block_bytes > 0)
                {
                    uint64_t block = _opts.nfs_copy_block_bytes;
                    uint64_t req_count = (static_cast<uint64_t>(fsize) + block - 1) / block;
                    if (req_count > static_cast<uint64_t>(_opts.max_copy_request_count_per_file))
                    {
                        derror("nfs: rejecting get_file_size response: file '%s' size %lld needs %llu "
                               "copy blocks, exceeding the limit of %d",
                            resp.file_list[i].c_str(), (long long)fsize, (unsigned long long)req_count,
                            _opts.max_copy_request_count_per_file);
                        ureq->nfs_task->enqueue(ERR_INVALID_DATA, 0);
                        delete ureq;
                        return;
                    }
                }
            }

            for (size_t i = 0; i < resp.size_list.size(); i++) // file list
            {
                file_context *filec;
                uint64_t size = resp.size_list[i];

                filec = new file_context(ureq, resp.file_list[i], resp.size_list[i]);
                ureq->file_context_map.insert(std::pair<std::string, file_context*>(
                    utils::filesystem::path_combine(ureq->file_size_req.dst_dir, resp.file_list[i]), 
                    filec
                    ));

                //dinfo("this file size is %d, name is %s", size, resp.file_list[i].c_str());

                // new all the copy requests                

                uint64_t req_offset = 0;
                uint32_t req_size;
                if (size > _opts.nfs_copy_block_bytes)
                    req_size = _opts.nfs_copy_block_bytes;
                else
                    req_size = static_cast<uint32_t>(size);

                int idx = 0;
                for (;;) // send one file with multi-round rpc
                {
                    auto req = dsn::ref_ptr<copy_request_ex>(new copy_request_ex(filec, idx++));
                    filec->copy_requests.push_back(req);

                    {
                        zauto_lock l(_copy_requests_lock);
                        _copy_requests.push(req);
                    }

                    req->copy_req.source = ureq->file_size_req.source;
                    req->copy_req.file_name = resp.file_list[i];
                    req->copy_req.offset = req_offset;
                    req->copy_req.size = req_size;
                    req->copy_req.dst_dir = ureq->file_size_req.dst_dir;
                    req->copy_req.source_dir = ureq->file_size_req.source_dir;
                    req->copy_req.overwrite = ureq->file_size_req.overwrite;
                    req->copy_req.is_last = (size <= req_size);

                    req_offset += req_size;
                    size -= req_size;
                    if (size <= 0)
                    {
                        dassert(size == 0, "last request must read exactly the remaing size of the file");
                        break;
                    }

                    if (size > _opts.nfs_copy_block_bytes)
                        req_size = _opts.nfs_copy_block_bytes;
                    else
                        req_size = static_cast<uint32_t>(size);
                }
            }

            continue_copy(0);
        }


        void nfs_client_impl::continue_copy(int done_count)
        {
            if (done_count > 0)
            {
                _concurrent_copy_request_count -= done_count;
            }

            if (++_concurrent_copy_request_count > _opts.max_concurrent_remote_copy_requests)
            {
                --_concurrent_copy_request_count;
                return;
            }

            dsn::ref_ptr<copy_request_ex> req = nullptr;
            while (true)
            {
                {
                    zauto_lock l(_copy_requests_lock);
                    if (!_copy_requests.empty())
                    {
                        req = _copy_requests.front();
                        _copy_requests.pop();
                    }
                    else
                    {
                        --_concurrent_copy_request_count;
                        break;
                    }
                }

                {
                    zauto_lock l(req->lock);
                    if (req->is_valid)
                    {
                        req->add_ref();
                        req->remote_copy_task = copy(
                            req->copy_req,
                            [=](error_code err, copy_response&& resp)
                            {
                                end_copy(err, std::move(resp), req.get());
                            },
                            std::chrono::milliseconds(0),
                            0,
                            0,
                            0,
                            req->file_ctx->user_req->file_size_req.source);

                        if (++_concurrent_copy_request_count > _opts.max_concurrent_remote_copy_requests)
                        {
                            --_concurrent_copy_request_count;
                            break;
                        }
                    }
                }
            }
        }

        void nfs_client_impl::end_copy(
            ::dsn::error_code err,
            const copy_response& resp,
            void* context)
        {
            //dinfo("*** call RPC_NFS_COPY end, return (%d, %d) with %s", resp.offset, resp.size, err.to_string());

            dsn::ref_ptr<copy_request_ex> reqc;
            reqc = (copy_request_ex*)context;
            reqc->release_ref();

            continue_copy(1);
            
            if (err == ERR_OK)
            {
                err = resp.error;
            }

            if (err != ::dsn::ERR_OK)
            {
                handle_completion(reqc->file_ctx->user_req, err);
                return;
            }

            // response.size and response.file_content are independent fields filled in by the
            // (untrusted) remote server. The local write issues
            // file::write(response.file_content.data(), response.size, ...), so a response whose
            // declared size is negative or larger than the actual file_content buffer would make
            // that write read past the end of the buffer -- corrupting the destination file with
            // adjacent heap memory (and disclosing it into the replicated data) or crashing. This
            // is the client-side counterpart of the size validation on_copy already performs on
            // the request. A zero size is legal (empty-file segment) and is handled specially in
            // continue_write. On the honest path file_content is the full block buffer and size is
            // the valid prefix, so size <= file_content.length() always holds.
            if (resp.size < 0 || static_cast<uint64_t>(resp.size) > resp.file_content.length())
            {
                derror("nfs: invalid copy response for file %s: declared size %d exceeds "
                       "content buffer length %u",
                       reqc->file_ctx->file_name.c_str(),
                       resp.size,
                       (uint32_t)resp.file_content.length());
                handle_completion(reqc->file_ctx->user_req, ERR_INVALID_DATA);
                return;
            }

            // offset and size are echoed back by the (untrusted) remote server, but this client
            // already knows exactly which block it asked for (reqc->copy_req): the honest server
            // always echoes the request's offset/size unchanged. The local write in
            // continue_write() uses response.offset as the destination file offset and
            // response.size as the byte count, so a malicious or compromised server that returns
            // a different offset could make the client write the block at an arbitrary position
            // in the destination file -- e.g. a huge offset inflates it into a multi-terabyte
            // sparse file (local disk-exhaustion DoS), and a wrong offset/size scrambles the file
            // layout. Since the client controls the requested block, require the response to
            // describe exactly that block and reject any mismatch.
            if (resp.offset != reqc->copy_req.offset || resp.size != reqc->copy_req.size)
            {
                derror("nfs: copy response for file %s does not match the requested block: "
                       "requested offset=%lld size=%d, got offset=%lld size=%d",
                       reqc->file_ctx->file_name.c_str(),
                       (long long)reqc->copy_req.offset,
                       (int)reqc->copy_req.size,
                       (long long)resp.offset,
                       (int)resp.size);
                handle_completion(reqc->file_ctx->user_req, ERR_INVALID_DATA);
                return;
            }

            reqc->response = resp;
            reqc->response.error.end_tracking(); // always ERR_OK
            reqc->is_ready_for_write = true;

            auto& fc = reqc->file_ctx;

            // check write availability
            {
                zauto_lock l(fc->user_req->user_req_lock);
                if (fc->current_write_index != reqc->index - 1)
                    return;
            }

            // check readies for local writes
            {
                zauto_lock l(fc->user_req->user_req_lock);
                for (int i = reqc->index; i < (int)(fc->copy_requests.size()); i++)
                {
                    if (fc->copy_requests[i]->is_ready_for_write)
                    {
                        fc->current_write_index++;

                        {
                            zauto_lock l(_local_writes_lock);
                            _local_writes.push(fc->copy_requests[i]);
                        }
                    }
                    else
                        break;
                }
            }

            continue_write();
        }

        void nfs_client_impl::continue_write()
        {
            // check write quota
            if (++_concurrent_local_write_count > _opts.max_concurrent_local_writes)
            {
                --_concurrent_local_write_count;
                return;
            }

            // Loop rather than recurse on per-file failures: a persistent error
            // (e.g. a full disk that fails every transfer) would otherwise make
            // continue_write() invoke itself once per failed request and could
            // grow the call stack without bound.
            while (true)
            {
                // get write
                dsn::ref_ptr<copy_request_ex> reqc;
                while (true)
                {
                    {
                        zauto_lock l(_local_writes_lock);
                        if (!_local_writes.empty())
                        {
                            reqc = _local_writes.front();
                            _local_writes.pop();
                        }
                        else
                        {
                            reqc = nullptr;
                            break;
                        }
                    }

                    {
                        zauto_lock l(reqc->lock);
                        if (reqc->is_valid)
                            break;
                    }
                }

                if (nullptr == reqc)
                {
                    --_concurrent_local_write_count;
                    return;
                }

                // real write
                std::string file_path = dsn::utils::filesystem::path_combine(reqc->copy_req.dst_dir, reqc->file_ctx->file_name);
                std::string path = dsn::utils::filesystem::remove_file_name(file_path.c_str());
                if (!dsn::utils::filesystem::create_directory(path))
                {
                    derror("create directory %s failed", path.c_str());
                    error_code err = ERR_FILE_OPERATION_FAILED;
                    handle_completion(reqc->file_ctx->user_req, err);
                    continue;
                }

                dsn_handle_t hfile = reqc->file_ctx->file.load();
                if (!hfile)
                {
                    zauto_lock l(reqc->file_ctx->user_req->user_req_lock);
                    hfile = reqc->file_ctx->file.load();
                    if (!hfile)
                    {
                        int open_flags =
                            nfs_client_detail::destination_open_flags(
                                reqc->copy_req.overwrite);
                        hfile = dsn_file_open(file_path.c_str(), open_flags, 0666);
                        reqc->file_ctx->file = hfile;
                    }
                }

                if (!hfile)
                {
                    derror("file open %s failed", file_path.c_str());
                    error_code err = ERR_FILE_OPERATION_FAILED;
                    handle_completion(reqc->file_ctx->user_req, err);
                    continue;
                }

                // An empty source file yields a copy request with response.size == 0. The
                // destination file has already been opened with the requested creation
                // policy, so there is nothing to write: a zero-length file::write would
                // complete with ERR_HANDLE_EOF and be reported as a failure. Finalize this
                // segment as a success inline -- mirroring the success bookkeeping in
                // local_write_callback -- and continue the loop (no recursion) instead of
                // issuing the zero-length write.
                if (reqc->response.size == 0)
                {
                    reqc->response.file_content = blob();

                    bool completed = false;
                    {
                        zauto_lock l(reqc->file_ctx->user_req->user_req_lock);
                        if (++reqc->file_ctx->finished_segments == (int)reqc->file_ctx->copy_requests.size())
                        {
                            if (++reqc->file_ctx->user_req->finished_files == (int)reqc->file_ctx->user_req->file_context_map.size())
                            {
                                completed = true;
                            }
                        }
                    }

                    if (completed)
                    {
                        handle_completion(reqc->file_ctx->user_req, ERR_OK);
                    }
                    continue;
                }

                {
                    zauto_lock l(reqc->lock);
                    auto& reqc_save = *reqc.get();
                    reqc_save.local_write_task = file::write(
                        hfile,
                        reqc_save.response.file_content.data(),
                        reqc_save.response.size,
                        reqc_save.response.offset,
                        LPC_NFS_WRITE,
                        this,
                        [this, reqc_cap = std::move(reqc)] (error_code err, int sz)
                        {
                            local_write_callback(err, sz, std::move(reqc_cap));
                        }
                    );
                }
                return;
            }
        }

        void nfs_client_impl::local_write_callback(error_code err, size_t sz, dsn::ref_ptr<copy_request_ex> reqc)
        {
            //dassert(reqc->local_write_task == task::get_current_task(), "");
            --_concurrent_local_write_count;

            auto write_err =
                nfs_client_detail::local_write_result(err, sz, reqc->response.size);
            if (err == ERR_OK && write_err != ERR_OK)
            {
                derror("short write for nfs file '%s': expected %u bytes, wrote %zu",
                       reqc->file_ctx->file_name.c_str(),
                       static_cast<unsigned int>(reqc->response.size),
                       sz);
            }
            err = write_err;

            // clear all content to release memory quickly
            reqc->response.file_content = blob();

            continue_write();

            bool completed = false;
            if (err != ERR_OK)
            {
                completed = true;
            }
            else
            {
                zauto_lock l(reqc->file_ctx->user_req->user_req_lock);
                if (++reqc->file_ctx->finished_segments == (int)reqc->file_ctx->copy_requests.size())
                {
                    if (++reqc->file_ctx->user_req->finished_files == (int)reqc->file_ctx->user_req->file_context_map.size())
                    {
                        completed = true;
                    }
                }
            }

            if (completed)
            {   
                handle_completion(reqc->file_ctx->user_req, err);
            }
        }
        
        void nfs_client_impl::handle_completion(user_request *req, error_code err)
        {
            {
                zauto_lock l(req->user_req_lock);
                if (req->is_finished)
                    return;

                req->is_finished = true;
            }

            size_t total_size = 0;
            for (auto& f : req->file_context_map)
            {
                total_size += f.second->file_size;
                
                for (auto& rc : f.second->copy_requests)
                {
                    ::dsn::task_ptr ctask, wtask;
                    {
                        zauto_lock l(rc->lock);
                        rc->is_valid = false;
                        ctask = rc->remote_copy_task;
                        wtask = rc->local_write_task;

                        rc->remote_copy_task = nullptr;
                        rc->local_write_task = nullptr;
                    }

                    if (err != ERR_OK)
                    {
                        if (ctask != nullptr)
                        {
                            if (ctask->cancel(true))
                            {
                                _concurrent_copy_request_count--;
                                rc->release_ref();
                            }
                        }

                        if (wtask != nullptr)
                        {
                            if (wtask->cancel(true))
                            {
                                _concurrent_local_write_count--;
                            }
                        }
                    }
                }
                                
                if (f.second->file)
                {
                    auto err2 = dsn_file_close(f.second->file);
                    if (err2 != ERR_OK)
                    {
                        dwarn("dsn_file_close failed, err = %s", dsn_error_to_string(err2));
                    }

                    f.second->file = nullptr;

                    const bool is_incomplete =
                        f.second->finished_segments != (int)f.second->copy_requests.size();
                    const bool rollback_preserving_copy =
                        err != ERR_OK && !req->file_size_req.overwrite;
                    if (is_incomplete || rollback_preserving_copy)
                    {
                        // A preserving copy creates files with O_EXCL, so every opened file
                        // belongs to this request. Roll all of them back when any file fails;
                        // otherwise completed siblings would make a retry fail with O_EXCL.
                        const std::string& path = f.first;
                        if (!::dsn::utils::filesystem::remove_path(path))
                        {
                            dwarn("failed to remove transfer file %s during cleanup", path.c_str());
                        }
                    }

                    f.second->copy_requests.clear();
                }

                delete f.second;
            }

            req->file_context_map.clear();
            req->nfs_task->enqueue(err, err == ERR_OK ? total_size : 0);

            delete req;

            // clear out all canceled requests
            if (err != ERR_OK)
            {
                continue_copy(0);
                continue_write();
            }
        }

    }
}
