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


# include "daemon.server.h"
# include "daemon.h"
# include <dsn/cpp/utils.h>
# include <dsn/tool_api.h>
# include <dsn/utility/module_init.cpp.h>
# include <fstream>

# if defined(__linux__)
# include <sys/prctl.h>
# include <sys/wait.h>
# endif

using namespace ::dsn::replication;
extern void daemon_register_test_server();

MODULE_INIT_BEGIN(daemon)
    dsn::register_app< ::dsn::dist::daemon>("daemon");
    daemon_register_test_server();
MODULE_INIT_END

namespace dsn
{
    namespace dist
    {
# if defined(__linux__)
        static daemon_s_service* s_single_daemon = nullptr;
        void daemon_s_service::on_exit(::dsn::sys_exit_type st)
        {
            int pid = getpid();
            kill(-pid, SIGTERM);
            sleep(2);
            kill(-pid, SIGKILL);
        }
# endif

        daemon_s_service::daemon_s_service() 
            : ::dsn::serverlet<daemon_s_service>("daemon_s"), _online(false)
        {
# if defined(__linux__)
            s_single_daemon = this;
            ::dsn::tools::sys_exit.put_back(daemon_s_service::on_exit, "daemon.exit");
            setpgid(0, 0);
# endif

            _working_dir = utils::filesystem::path_combine(dsn_get_app_data_dir(), "apps");
            _package_server = rpc_address(
                dsn_config_get_value_string("apps.daemon", "package_server_host", "", "the host name of the app store where to download package"),
                (uint16_t)dsn_config_get_value_uint64("apps.daemon", "package_server_port", 26788, "the port of the app store where to download package")
                );
            _package_dir_on_package_server = dsn_config_get_value_string("apps.daemon", "package_dir", "", "the dir on the app store where to download package");
            _app_port_min = (uint32_t)dsn_config_get_value_uint64("apps.daemon", "app_port_min", 59001, "the minimum port that can be assigned to app");
            _app_port_max = (uint32_t)dsn_config_get_value_uint64("apps.daemon", "app_port_max", 60001, "the maximum port that can be assigned to app");
            _config_sync_interval_seconds = (uint32_t)dsn_config_get_value_uint64("apps.daemon", "config_sync_interval_seconds", 5, "sync configuration with meta server for every how many seconds");

# ifdef _WIN32
            _unzip_format_string = dsn_config_get_value_string(
                "apps.daemon", 
                "unzip_format_string", 
                "powershell.exe -nologo -noprofile -command \"& { Add-Type -A 'System.IO.Compression.FileSystem'; [IO.Compression.ZipFile]::ExtractToDirectory('%s.zip', '%s'); }\"",
                "unzip cmd used as fprintf(fmt-str, src-package-name, dst-dir)"
            );
# else
            _unzip_format_string = dsn_config_get_value_string(
                "apps.daemon",
                "unzip_format_string",
                "tar zxvf %s.tar.gz -C %s",
                "unzip cmd used as fprintf(fmt-str, src-package-name, dst-dir)"
            );
# endif

            if (!utils::filesystem::directory_exists(_working_dir))
            {
                utils::filesystem::create_directory(_working_dir);
            }

# ifdef _WIN32
            _job = CreateJobObjectA(NULL, NULL);
            dassert(_job != NULL, "create windows job failed, err = %d", ::GetLastError());

            JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli = { 0 };

            // Configure all child processes associated with the job to terminate when the
            jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;

            if (0 == SetInformationJobObject(_job, JobObjectExtendedLimitInformation, &jeli, sizeof(jeli)))
            {
                dassert(false, "Could not SetInformationJobObject, err = %d", ::GetLastError());
            }
# else
# endif
        }

        daemon_s_service::~daemon_s_service()
        {
        }

        void daemon_s_service::on_config_proposal(const ::dsn::replication::configuration_update_request& proposal)
        {
            dinfo("receive config proposal for %s with type %s at ballot %" PRId64,
                proposal.info.app_name.c_str(),
                enum_to_string(proposal.type)
                );

            switch (proposal.type)
            {
            case config_type::CT_ADD_SECONDARY:
            case config_type::CT_ADD_SECONDARY_FOR_LB:
                on_add_app(proposal);
                break;

            case config_type::CT_REMOVE:
                on_remove_app(proposal);
                break;

            default:
                dwarn("not supported configuration type %s received", enum_to_string(proposal.type));
                break;
            }
        }

        DEFINE_TASK_CODE(LPC_DAEMON_APPS_CHECK_TIMER, TASK_PRIORITY_COMMON, THREAD_POOL_DEFAULT)

        void daemon_s_service::open_service()
        {
            std::vector<rpc_address> meta_servers;
            dsn::replication::replica_helper::load_meta_servers(meta_servers);
            _fd.reset(new slave_failure_detector_with_multimaster(meta_servers, 
                [=]() { this->on_master_disconnected(); },
                [=]() { this->on_master_connected(); }
                ));

            this->register_rpc_handler(RPC_CONFIG_PROPOSAL, "config_proposal", &daemon_s_service::on_config_proposal);

            auto err = _fd->start(
                5,
                3,
                14,
                15
                );

            dassert(ERR_OK == err, "failure detector start failed, err = %s", err.to_string());

            _fd->register_master(_fd->current_server_contact());

            _app_check_timer = tasking::enqueue_timer(
                LPC_DAEMON_APPS_CHECK_TIMER,
                this,
                [this]{ this->check_apps(); },
                std::chrono::milliseconds(
                    (int)dsn_config_get_value_uint64("apps.daemon", "local_app_check_period_ms", 
                        500, 
                        "periodically check local apps' healthy status with this period")
                        )
                );


            _config_sync_timer = tasking::enqueue_timer(
                LPC_QUERY_CONFIGURATION_ALL,
                this,
                [this] {
                query_configuration_by_node();
                },
                std::chrono::seconds(_config_sync_interval_seconds)
                );

            _cli_kill_partition = dsn_cli_app_register(
                "kill_partition",
                "kill_partition app_id partition_index",
                "kill partition with its global partition id",
                (void*)this,
                [](void *context, int argc, const char **argv, dsn_cli_reply *reply)
                {
                    auto this_ = (daemon_s_service*)context;
                    this_->on_kill_app_cli(context, argc, argv, reply);
                },
                [](dsn_cli_reply reply)
                {
                    std::string* s = (std::string*)reply.context;
                    delete s;
                }
                );

        }

        void daemon_s_service::query_configuration_by_node()
        {
            if (!_online)
            {
                return;
            }

            dsn_message_t msg = dsn_msg_create_request(RPC_CM_QUERY_NODE_PARTITIONS);

            configuration_query_by_node_request req;
            req.node = primary_address();
            ::dsn::marshall(msg, req);

            rpc_address target(_fd->get_servers());
            rpc::call(
                target,
                msg,
                this,
                [this](error_code err, dsn_message_t request, dsn_message_t resp)
                {
                    on_node_query_reply(err, request, resp);
                }
            );
        }

        void daemon_s_service::on_node_query_reply(error_code err, dsn_message_t request, dsn_message_t response)
        {
            ddebug(
                "%s: node view replied, err = %s",
                primary_address().to_string(),
                err.to_string()
                );

            if (err != ERR_OK)
            {
                // retry when the timer fires again later
                return;
            }
            else
            {
                if (!_online)
                    return;

                configuration_query_by_node_response resp;
                ::dsn::unmarshall(response, resp);

                if (resp.err != ERR_OK || resp.err == ERR_BUSY)
                    return;

                std::unordered_map<dsn::gpid, std::shared_ptr< app_internal> > apps;

                _lock.lock_read();
                for (auto& pkg : _apps)
                {
                    for (auto& app : pkg.second->apps)
                    {
                        apps.emplace(app.second->configuration.pid, app.second);
                    }
                }
                _lock.unlock_read();

                
                // find apps on meta server but not on local daemon
                rpc_address host = primary_address();
                for (auto appc : resp.partitions)
                {
                    int i;
                    for (i = 0; i < (int)appc.config.secondaries.size(); i++)
                    {
                        // host nodes stored in secondaries
                        if (appc.config.secondaries[i] == host)
                        {
                            // worker nodes stored in last-drops
                            break;
                        }
                    }

                    dassert(i < (int)appc.config.secondaries.size(),
                        "host address %s must exist in secondary list of partition %d.%d",
                        host.to_string(), appc.config.pid.get_app_id(), appc.config.pid.get_partition_index()
                    );
                    
                    bool found = false;
                    auto it = apps.find(appc.config.pid);
                    if (it != apps.end())
                    {
                        found = true;

                        // update ballot when necessary
                        if (appc.config.ballot > it->second->configuration.ballot)
                        {
                            it->second->configuration = appc.config;
                        }

                        apps.erase(it);
                    }

                    if (!found)
                    {
                        configuration_update_request req;
                        req.info = appc.info;
                        req.config = appc.config;
                        req.host_node = host;

                        // for stateful services, we restart it here
                        if (appc.info.is_stateful)
                        {
                            on_add_app(req);
                        }

                        // for stateless services, we simply remove it
                        else
                        {
                            req.type = config_type::CT_REMOVE;

                            // worker nodes stored in last-drops
                            req.node = appc.config.last_drops[i];

                            std::shared_ptr<app_internal> app(new app_internal(req));
                            app->exited = true;
                            app->working_port = req.node.port();

                            update_configuration_on_meta_server(req.type, std::move(app));
                        }
                    }
                }

                // find apps on local daemon but not on meta server
                for (auto& app : apps)
                {
                    auto cap = app.second;
                    kill_app(std::move(cap));
                }
            }
        }

        void daemon_s_service::on_kill_app_cli(void *context, int argc, const char **argv, dsn_cli_reply *reply)
        {
            error_code err = ERR_INVALID_PARAMETERS;
            if (argc >= 2)
            {
                gpid gpid;
                gpid.set_app_id(atoi(argv[0]));
                gpid.set_partition_index(atoi(argv[1]));
                std::shared_ptr<app_internal> app = nullptr;
                {
                    ::dsn::service::zauto_write_lock l(_lock);
                    for (auto& pkg : _apps)
                    {
                        auto it = pkg.second->apps.find(gpid);
                        if (it != pkg.second->apps.end())
                        {
                            app = it->second;
                            break;
                        }
                    }
                }

                if (app == nullptr)
                {
                    err = ERR_OBJECT_NOT_FOUND;
                }
                else
                {
                    kill_app(std::move(app));
                    err = ERR_OK;
                }
            }

            std::string* resp_json = new std::string();
            *resp_json = err.to_string();
            reply->context = resp_json;
            reply->message = (const char*)resp_json->c_str();
            reply->size = resp_json->size();
            return;
        }

        void daemon_s_service::close_service()
        {
            _app_check_timer->cancel(true);
            _fd->stop();
            this->unregister_rpc_handler(RPC_CONFIG_PROPOSAL);

            dsn_cli_deregister(_cli_kill_partition);
            _cli_kill_partition = nullptr;
        }

        void daemon_s_service::on_master_connected()
        {
            _online = true;
            dinfo("master is connected");
        }
        
        void daemon_s_service::on_master_disconnected()
        {       
            _online = false;

            // TODO: fail-over
           /* {
                ::dsn::service::zauto_read_lock l(_lock);
                for (auto& app : _apps)
                {
                    kill_app(std::move(app.second));
                }

                _apps.clear();
            }*/

            dinfo("master is disconnected");
        }

        DEFINE_TASK_CODE_AIO(LPC_DAEMON_DOWNLOAD_PACKAGE, TASK_PRIORITY_COMMON, THREAD_POOL_DEFAULT)

        void daemon_s_service::on_add_app(const ::dsn::replication::configuration_update_request & proposal)
        {
            std::shared_ptr<package_internal> ppackage;
            std::shared_ptr<app_internal> app;
            bool resource_ready = false;
            bool is_resource_downloading = false;

            {
                ::dsn::service::zauto_write_lock l(_lock);

                // check package exists or not
                {
                    auto it = _apps.find(proposal.info.app_type);

                    // app is running with the same package
                    if (it != _apps.end())
                    {
                        ppackage = it->second;
                    }
                    else
                    {
                        ppackage.reset(new package_internal());
                        ppackage->package_dir = utils::filesystem::path_combine(_working_dir, proposal.info.app_type);

                        // each package is required with a config.deploy.ini in it for run as: dsn.svchost config.ini -cargs port=%port%;envs
                        ppackage->config_file = utils::filesystem::path_combine(ppackage->package_dir,
                            "config.deploy.ini");

                        _apps.emplace(proposal.info.app_type, ppackage);
                    }
                }

                // check app exists or not
                {
                    resource_ready = ppackage->resource_ready;
                    is_resource_downloading = ppackage->downloading;
                    if (!is_resource_downloading)
                        ppackage->downloading = true; // done later in this call

                    auto it = ppackage->apps.find(proposal.config.pid);

                    // app is running with the same package
                    if (it != ppackage->apps.end())
                    {
                        // update config when necessary
                        if (proposal.config.ballot > it->second->configuration.ballot)
                        {
                            it->second->configuration = proposal.config;
                        }

                        return;
                    }
                    else
                    {
                        app.reset(new app_internal(proposal));
                        ppackage->apps.emplace(proposal.config.pid, app);
                    }
                }
            }
            
            // check and start
            if (resource_ready)
            {                
                start_app(std::move(app), std::move(ppackage));
            }

            // download package first if necesary
            else if (!is_resource_downloading)
            {
                // TODO: better way to download package from app store 
# ifdef _WIN32
                std::vector<std::string> files{ proposal.info.app_type + ".zip" };
# else
                std::vector<std::string> files{ proposal.info.app_type + ".tar.gz" };
# endif

                dinfo("start downloading package %s from %s to %s",
                    proposal.info.app_type.c_str(),
                    _package_server.to_string(),
                    _working_dir.c_str()
                    );

                utils::filesystem::remove_path(files[0]);

                file::copy_remote_files(
                    _package_server,
                    _package_dir_on_package_server,
                    files,
                    _working_dir,
                    true,
                    LPC_DAEMON_DOWNLOAD_PACKAGE,
                    this,
                    [this, pkg = std::move(ppackage), capp = std::move(app)](error_code err, size_t sz) mutable
                    {
                        pkg->downloading = false;
                        if (err == ERR_OK)
                        {
                            dinfo("download app %s OK ...",
                                capp->info.app_type.c_str()
                            );

                            // TODO: using zip lib instead
                            char command[1024];
                            snprintf_p(command, sizeof(command), 
                                _unzip_format_string.c_str(),
                                (_working_dir + '/' + capp->info.app_type).c_str(),
                                _working_dir.c_str()
                                );
                            
                            // decompress when completed
                            int serr = system(command);
                            if (serr != 0)
                            {
                                derror("extract package %s with cmd '%s' failed, err = %d, errno = %d",
                                    pkg->package_dir.c_str(),
                                    command,
                                    serr,
                                    errno
                                );

                                err = ERR_UNKNOWN;

                                utils::filesystem::remove_path(pkg->package_dir);
                                {
                                    ::dsn::service::zauto_write_lock l(_lock);
                                    _apps.erase(capp->info.app_type);
                                }
                            }
                            else
                            {
                                if (utils::filesystem::file_exists(pkg->config_file))
                                {
                                    same_package_apps apps;
                                    {
                                        ::dsn::service::zauto_write_lock l(_lock);
                                        apps = pkg->apps;
                                        pkg->resource_ready = true;                                        
                                    }

                                    for (auto& app : apps)
                                    {
                                        auto pkg2 = pkg;
                                        start_app(std::move(app.second), std::move(pkg2));
                                    }
                                }
                                else
                                {
                                    derror("package %s does not contain config file '%s' in it",
                                        pkg->package_dir.c_str(),
                                        pkg->config_file.c_str()
                                    );

                                    err = ERR_OBJECT_NOT_FOUND;

                                    utils::filesystem::remove_path(pkg->package_dir);
                                    {
                                        ::dsn::service::zauto_write_lock l(_lock);
                                        _apps.erase(capp->info.app_type);
                                    }
                                }
                            }
                        }
                        else
                        {

                            derror("download app %s failed, err = %s ...",
                                capp->info.app_type.c_str(),
                                err.to_string()
                            );

                            utils::filesystem::remove_path(pkg->package_dir);
                            {
                                ::dsn::service::zauto_write_lock l(_lock);

                                // TODO: try multiple times for timeouts
                                _apps.erase(capp->info.app_type);
                            }
                        }
                    }
                    );
            }
        }

        void daemon_s_service::on_remove_app(const ::dsn::replication::configuration_update_request & proposal)
        {
            // check app exists or not
            std::shared_ptr<app_internal> app;

            {
                ::dsn::service::zauto_read_lock l(_lock);
                auto it = _apps.find(proposal.info.app_type);

                // app is running with the same package
                if (it != _apps.end())
                {
                    auto it2 = it->second->apps.find(proposal.config.pid);

                    if (it2 != it->second->apps.end())
                    {
                        // proposal's package is older or the same
                        if (proposal.config.ballot <= it2->second->configuration.ballot)
                            return;
                        else
                        {
                            app = std::move(it2->second);
                            app->configuration = proposal.config;

                            it->second->apps.erase(it2);
                            if (it->second->apps.empty() && !it->second->downloading)
                            {
                                _apps.erase(proposal.info.app_type);
                            }
                        }
                    }
                }
            }

            if (nullptr != app)
            {
                auto cap_app = app;
                kill_app(std::move(cap_app));

                update_configuration_on_meta_server(config_type::CT_REMOVE, std::move(app));
            }
        }

        bool daemon_s_service::app_internal::is_exited()
        {
            if (process_handle)
            {

# ifdef _WIN32
                return WAIT_OBJECT_0 == ::WaitForSingleObject(process_handle, 0);
# else
                int child = (int)(uint64_t)process_handle;

                // see if the process exits
                int return_status;
                return child == waitpid(child, &return_status, WNOHANG);
# endif
            }
            else
                return true;
        }

        int daemon_s_service::app_internal::close()
        {
            int exit_code = 0;
# ifdef _WIN32
            if (nullptr != process_handle)
            {
                ::GetExitCodeProcess(process_handle, (LPDWORD)&exit_code);
                ::TerminateProcess(process_handle, 0);
                ::CloseHandle(process_handle);
                process_handle = nullptr;
            }
# else
            if (nullptr != process_handle)
            {
                int child = (int)(uint64_t)process_handle;
                kill(child, SIGKILL);

                waitpid(child, &exit_code, 0);
                process_handle = nullptr;
            }
# endif
            return exit_code;
        }

        void daemon_s_service::start_app(std::shared_ptr<app_internal> && app, std::shared_ptr<package_internal>&& pkg)
        {
            dassert(nullptr == app->process_handle, "app handle must be empty at this point");

            // set port and run
            for (int i = 0; i < 10; i++)
            {
                uint32_t port = (uint32_t)dsn_random32(_app_port_min, _app_port_max);

                // set up working dir as _working_dir/package-id/gpid.port
                {
                    std::stringstream ss;
                    ss << app->configuration.pid.get_app_id() << "." << app->configuration.pid.get_partition_index() << "." << port;
                    app->working_dir = utils::filesystem::path_combine(pkg->package_dir, ss.str());

                    if (utils::filesystem::directory_exists(app->working_dir))
                        continue;

                    utils::filesystem::create_directory(app->working_dir);
                }
                                
                std::string config_file = pkg->config_file;

                // developers parameter the default config file using RDSN_TARGET_CONFIG env var
                auto envs = app->info.envs;
                auto it = envs.find("RDSN_TARGET_CONFIG");
                if (it != envs.end())
                {
                    config_file = utils::filesystem::path_combine(pkg->package_dir, it->second);
                    if (!utils::filesystem::file_exists(config_file))
                    {
                        derror("package %s does not contain config file '%s' in it",
                            pkg->package_dir.c_str(),
                            config_file.c_str()
                            );

                        kill_app(std::move(app));
                        return;
                    }
                    envs.erase(it);
                }

                // developers overwrite the config file using RDSN_OVERWRITES 
                std::string overwrites = "";
                it = envs.find("RDSN_OVERWRITES");
                if (it != envs.end())
                {
                    overwrites = std::move(it->second);
                    envs.erase(it);
                }

                // add deployment path as DSN_DEPLOYMENT_PATH
# ifdef _WIN32
                char exe_path[1024];
                ::GetModuleFileNameA(nullptr, exe_path, 1024);
# else
                char exe_path[1024];
                if (readlink("/proc/self/exe", exe_path, 1024) == -1)
                {
                    dassert(false, "read /proc/self/exe failed");
                }
# endif
                std::string host_name = utils::filesystem::get_file_name(exe_path);
                dassert(host_name.substr(0, strlen("dsn.svchost")) == "dsn.svchost",
                    "invalid daemon exe name %s vs dsn.svchost",
                    host_name.c_str()
                );

                std::string deployment_dir = exe_path;
                deployment_dir = deployment_dir.substr(0, deployment_dir.length() - host_name.length() - 1);
                envs["DSN_DEPLOYMENT_PATH"] = deployment_dir;

                app->working_port = port;

# ifdef _WIN32
                std::stringstream ss; 
                ss << exe_path << " " << config_file << " -cargs port=" << port;
                for (auto& kv : envs)
                {
                    ss << ";" << kv.first << "=" << kv.second;
                }

                if (overwrites.length() > 0)
                {
                    ss << " -overwrite " << overwrites;
                }

                std::string command = ss.str();
                dwarn("try start app %s with command %s at working dir %s ...",
                    app->info.app_type.c_str(),
                    command.c_str(),
                    app->working_dir.c_str()
                );

                STARTUPINFOA si;
                PROCESS_INFORMATION pi;
                SECURITY_ATTRIBUTES sa;

                sa.nLength = sizeof(sa);
                sa.lpSecurityDescriptor = NULL;
                sa.bInheritHandle = TRUE;

                ZeroMemory(&si, sizeof(si));
                si.cb = sizeof(si);
                ZeroMemory(&pi, sizeof(pi));

                si.hStdError = ::CreateFileA(
                    utils::filesystem::path_combine(app->working_dir, "foo.err").c_str(),
                    FILE_APPEND_DATA,
                    FILE_SHARE_WRITE | FILE_SHARE_READ,
                    &sa,
                    OPEN_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL
                );

                si.hStdOutput = ::CreateFileA(
                    utils::filesystem::path_combine(app->working_dir, "foo.out").c_str(),
                    FILE_APPEND_DATA,
                    FILE_SHARE_WRITE | FILE_SHARE_READ,
                    &sa,
                    OPEN_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL
                );

                si.hStdInput = nullptr;
                si.dwFlags |= STARTF_USESTDHANDLES;

                if (::CreateProcessA(NULL, (LPSTR)command.c_str(), NULL, NULL, TRUE, CREATE_NEW_CONSOLE, 
                    NULL, // env
                    (LPSTR)app->working_dir.c_str(), &si, &pi))
                {
                    if (0 == ::AssignProcessToJobObject(_job, pi.hProcess))
                    {
                        // dassert(false, "cannot attach process to job, err = %d", ::GetLastError());
                    }

                    app->process_handle = pi.hProcess;
                    CloseHandle(pi.hThread);  
                    CloseHandle(si.hStdError);
                    CloseHandle(si.hStdOutput);
                }
                else
                {
                    CloseHandle(si.hStdError);
                    CloseHandle(si.hStdOutput);

                    derror("create process (CreateProcess) failed, err = %d", ::GetLastError());
                    break;
                }
# else
                int child = fork();
                if (-1 == child)
                {
                    derror("create process (fork) failed, err = %d", errno);
                    break;
                }

                // child process
                else if (child == 0)
                {
                    // redirect std output and err
                    int serr = open(
                        utils::filesystem::path_combine(app->working_dir, "foo.err").c_str(),
                        O_RDWR | O_CREAT, S_IRUSR | S_IWUSR
                    );

                    int sout = open(
                        utils::filesystem::path_combine(app->working_dir, "foo.out").c_str(),
                        O_RDWR | O_CREAT, S_IRUSR | S_IWUSR
                    );

                    dup2(sout, 1);
                    dup2(serr, 2);

                    close(serr);
                    close(sout);

                    // set up envs
                    chdir(app->working_dir.c_str());
                                        
                    std::string libs_new = 
                        pkg->package_dir + ":" + 
                        deployment_dir + ":" + 
                        getenv("LD_LIBRARY_PATH")
                        ;

                    setenv("LD_LIBRARY_PATH", libs_new.c_str(), 1);

                    std::stringstream ss;
                    ss << "port=" << port;
                    for (auto& kv : envs)
                    {
                        ss << ";" << kv.first << "=" << kv.second;
                    }
                    std::string cargs = ss.str();

                    dwarn("try start app %s with command %s %s -cargs %s -overwrite %s at working dir %s ...",
                        app->info.app_type.c_str(),
                        exe_path,
                        config_file.c_str(),
                        cargs.c_str(),
                        overwrites.c_str(),
                        app->working_dir.c_str()
                    );

                    // run command
                    if (overwrites.length() == 0)
                    {
                        char* const argv[] = {
                            (char*)"dsn.svchost",
                            (char*)config_file.c_str(),
                            (char*)"-cargs",
                            (char*)cargs.c_str(),
                            nullptr
                        };
                        execve(exe_path, argv, environ);
                    }
                    else
                    {
                        char* const argv[] = {
                            (char*)"dsn.svchost",
                            (char*)config_file.c_str(),
                            (char*)"-cargs",
                            (char*)cargs.c_str(),
                            (char*)"-overwrite",
                            (char*)overwrites.c_str(),
                            nullptr
                        };
                        execve(exe_path, argv, environ);
                    }
                    exit(0);
                }
                else
                {
                    app->process_handle = (dsn_handle_t)(uint64_t)child;
                }
# endif

                // sleep a while to see whether the port is usable
                std::this_thread::sleep_for(std::chrono::milliseconds(50));

                if (!app->is_exited())
                {
                    break;
                }
                else
                {
                    app->close();
                }
            }

            // register to meta server if successful
            if (!app->exited && app->process_handle)
            {
                update_configuration_on_meta_server(config_type::CT_ADD_SECONDARY, std::move(app));
            }

            // remove for failure too many times
            else
            {
                kill_app(std::move(app));
            }
        }
        
        void daemon_s_service::kill_app(std::shared_ptr<app_internal> && app)
        {
            dinfo("kill app %s at working dir %s, port %d",
                app->info.app_type.c_str(),
                app->working_dir.c_str(),
                (int)app->working_port
                );

            {
                ::dsn::service::zauto_write_lock l(_lock);
                auto it = _apps.find(app->info.app_type);
                if (it != _apps.end())
                {
                    it->second->apps.erase(app->configuration.pid);
                    if (it->second->apps.empty() && !it->second->downloading)
                        _apps.erase(it);
                }
            }

            if (!app->exited)
            {
                app->close();
                app->exited = true;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1 ));
            utils::filesystem::remove_path(app->working_dir);
        }

        void daemon_s_service::check_apps()
        {
            std::vector< std::shared_ptr< app_internal> > apps, delete_list;

            _lock.lock_read();
            for (auto& pkg : _apps)
            {
                for (auto& app : pkg.second->apps)
                {
                    apps.push_back(app.second);
                }
            }
            _lock.unlock_read();
            
            for (auto& app : apps)
            {
                if (app->process_handle)
                {
                    if (app->is_exited())
                    {
                        app->exited = true;
                        int exit_code = app->close();

                        dinfo("app %s exits (code = %x), with working dir = %s, port = %d",
                            app->info.app_type.c_str(),
                            exit_code,
                            app->working_dir.c_str(),
                            (int)app->working_port
                        );
                    }
                }

                if (app->exited)
                {
                    delete_list.push_back(app);
                }
            }

            for (auto& app : delete_list)
            {
                auto cap_app = app;
                std::shared_ptr<package_internal> pkg = nullptr;

                if (app->info.is_stateful)
                {    
                    _lock.lock_read();
                    {
                        auto it = _apps.find(app->info.app_type);
                        if (it != _apps.end())
                        {
                            pkg = it->second;
                        }
                    }
                    _lock.unlock_read();
                }

                if (pkg != nullptr)
                {
                    // restart
                    app->exited = false;
                    start_app(std::move(cap_app), std::move(pkg));
                }
                else
                {                    
                    kill_app(std::move(cap_app));
                    update_configuration_on_meta_server(config_type::CT_REMOVE, std::move(app));
                }
            }
        }
        
        void daemon_s_service::update_configuration_on_meta_server(::dsn::replication::config_type::type type, std::shared_ptr<app_internal>&& app)
        {
            rpc_address node = primary_address();
            node.assign_ipv4(node.ip(), app->working_port);
            
            dsn_message_t msg = dsn_msg_create_request(RPC_CM_UPDATE_PARTITION_CONFIGURATION);

            std::shared_ptr<configuration_update_request> request(new configuration_update_request);
            request->info.is_stateful = false;
            request->config = app->configuration;
            request->type = type;
            request->node = node;
            request->host_node = primary_address();
            
            request->config.ballot++;
            if (type == config_type::CT_REMOVE)
            {
                auto it = std::remove(
                    request->config.secondaries.begin(),
                    request->config.secondaries.end(),
                    request->host_node
                    );
                request->config.secondaries.erase(it);

                it = std::remove(
                    request->config.last_drops.begin(),
                    request->config.last_drops.end(),
                    node
                    );
                request->config.last_drops.erase(it);
            }
            else
            {
                request->config.secondaries.emplace_back(request->host_node);
                request->config.last_drops.emplace_back(node);
            }

            ::dsn::marshall(msg, *request);

            rpc::call(
                _fd->get_servers(),
                msg,
                this,
                [=, cap_app = std::move(app)](error_code err, dsn_message_t reqmsg, dsn_message_t response) mutable
                {
                    on_update_configuration_on_meta_server_reply(type, std::move(cap_app), err, reqmsg, response);
                }
                );
        }

        void daemon_s_service::on_update_configuration_on_meta_server_reply(
            ::dsn::replication::config_type::type type, std::shared_ptr<app_internal> &&  app,
            error_code err, dsn_message_t request, dsn_message_t response
            )
        {
            if (false == _online)
            {
                err.end_tracking();
                return;
            }

            configuration_update_response resp;
            if (err == ERR_OK)
            {
                ::dsn::unmarshall(response, resp);
                if (resp.config.ballot <= app->configuration.ballot)
                    return;

                err = resp.err;
            }
            else if (err == ERR_TIMEOUT)
            {
                rpc::call(
                    _fd->get_servers(),
                    request,
                    this,
                    [=, cap_app = std::move(app)](error_code err, dsn_message_t reqmsg, dsn_message_t response) mutable
                    {
                        on_update_configuration_on_meta_server_reply(type, std::move(cap_app), err, reqmsg, response);
                    }
                    );
                return;
            }


            if (err != ERR_OK)
            {
                if (type == config_type::CT_ADD_SECONDARY)
                    kill_app(std::move(app));
            }
            else
            {
                app->configuration = resp.config;
            }
        }

        ::dsn::error_code daemon::start(int argc, char** argv)
        {
            _daemon_s_svc.reset(new daemon_s_service());
            _daemon_s_svc->open_service();
            return ::dsn::ERR_OK;
        }

        ::dsn::error_code daemon::stop(bool cleanup)
        {
            _daemon_s_svc->close_service();
            _daemon_s_svc = nullptr;
            return ERR_OK;
        }

        daemon::daemon(dsn_gpid gpid)
            : ::dsn::service_app(gpid)
        {

        }

        daemon::~daemon()
        {

        }
    }
}
