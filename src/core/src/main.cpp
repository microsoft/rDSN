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

# include <dsn/service_api_c.h>
# include <dsn/tool_api.h>
# include <dsn/utility/enum_helper.h>
# include <dsn/cpp/auto_codes.h>
# include <dsn/cpp/serialization.h>
# include <dsn/tool-api/task_spec.h>
# include <dsn/tool-api/zlock_provider.h>
# include <dsn/tool-api/nfs.h>
# include <dsn/tool-api/env_provider.h>
# include <dsn/utility/factory_store.h>
# include <dsn/tool-api/task.h>
# include <dsn/utility/singleton_store.h>
# include <dsn/cpp/utils.h> 

# include <dsn/utility/configuration.h>
# include "command_manager.h"
# include "service_engine.h"
# include "rpc_engine.h"
# include "disk_engine.h"
# include "task_engine.h"
# include "coredump.h"
# include "transient_memory.h"
# include "library_utils.h"
# include <fstream>

# ifndef _WIN32
# include <signal.h>
# include <unistd.h>
# else
# include <TlHelp32.h>
# endif

# ifdef __TITLE__
# undef __TITLE__
# endif
# define __TITLE__ "service_api_main"


 //
 // global state
 //
static struct _all_info_
{
    unsigned int                                              magic;
    bool                                                      engine_ready;
    bool                                                      config_completed;
    ::dsn::tools::tool_app                                    *tool;
    ::dsn::configuration_ptr                                  config;
    ::dsn::service_engine                                     *engine;
    std::vector< ::dsn::task_spec*>                            task_specs;
    ::dsn::memory_provider                                    *memory;

    bool is_config_completed() const {
        return magic == 0xdeadbeef && config_completed;
    }

    bool is_engine_ready() const {
        return magic == 0xdeadbeef && engine_ready;
    }

} dsn_all;


DSN_API const char* dsn_config_get_value_string(const char* section, const char* key, const char* default_value, const char* dsptr)
{
    return dsn_all.config->get_string_value(section, key, default_value, dsptr);
}

DSN_API bool dsn_config_get_value_bool(const char* section, const char* key, bool default_value, const char* dsptr)
{
    return dsn_all.config->get_value<bool>(section, key, default_value, dsptr);
}

DSN_API uint64_t dsn_config_get_value_uint64(const char* section, const char* key, uint64_t default_value, const char* dsptr)
{
    return dsn_all.config->get_value<uint64_t>(section, key, default_value, dsptr);
}

DSN_API double dsn_config_get_value_double(const char* section, const char* key, double default_value, const char* dsptr)
{
    return dsn_all.config->get_value<double>(section, key, default_value, dsptr);
}

DSN_API int dsn_config_get_all_sections(const char** buffers, /*inout*/ int* buffer_count)
{
    std::vector<const char*> sections;
    dsn_all.config->get_all_section_ptrs(sections);
    int scount = (int)sections.size();

    if (*buffer_count > scount)
        *buffer_count = scount;

    for (int i = 0; i < *buffer_count; i++)
    {
        buffers[i] = sections[i];
    }

    return scount;
}

DSN_API int dsn_config_get_all_keys(const char* section, const char** buffers, /*inout*/ int* buffer_count) // return all key count (may greater than buffer_count)
{
    std::vector<const char*> keys;
    dsn_all.config->get_all_keys(section, keys);
    int kcount = (int)keys.size();

    if (*buffer_count > kcount)
        *buffer_count = kcount;

    for (int i = 0; i < *buffer_count; i++)
    {
        buffers[i] = keys[i];
    }

    return kcount;
}

DSN_API void dsn_config_dump(const char* file)
{
    std::ofstream os(file, std::ios::out);
    dsn_all.config->dump(os);
    os.close();
}

extern void dsn_log_init();

// load all modules: local components, tools, frameworks, apps
static void load_all_modules(::dsn::configuration_ptr config)
{    
    std::vector< std::pair<std::string, std::string> > modules;
    std::map<std::string, std::size_t> module_map; // name -> index in modules

    // load local components, toollets, and tools
    // [modules]
    // dsn.tools.common
    // dsn.tools.hpc
    std::vector<const char*> lmodules;
    config->get_all_keys("modules", lmodules);
    for (auto& m : lmodules)
    {
        dassert(module_map.find(m) == module_map.end(), "duplicate [modules].%s", m);
        module_map[m] = modules.size();
        modules.push_back(std::make_pair(std::string(m), ""));
    }

    // load app and framework modules
    // TODO: move them to [modules] section as well
    std::vector<std::string> all_section_names;
    config->get_all_sections(all_section_names);
    
    for (auto it = all_section_names.begin(); it != all_section_names.end(); ++it)
    {
        if (it->substr(0, strlen("apps.")) == std::string("apps."))
        {
            std::string module = dsn_config_get_value_string(it->c_str(), "dmodule", "",
                "path of a dynamic library which implement this app role, and register itself upon loaded");
            if (module.length() > 0)
            {
                dassert(module_map.find(module) != module_map.end(), "[apps.%s].dmodule %s is not set in [modules]", it->c_str(), module.c_str());
                std::size_t idx = module_map[module];

                std::string bridge_args = dsn_config_get_value_string(it->c_str(), "dmodule_bridge_arguments", "",
                    "\n; when the service cannot automatically register its app types into rdsn \n"
                    "; through %dmoudule%'s dllmain or attribute(constructor), we require the %dmodule% \n"
                    "; implement an exporte function called \"dsn_error_t dsn_bridge(const char* args);\", \n"
                    "; which loads the real target (e.g., a python/Java/php module), that registers their \n"
                    "; app types and factories."
                );

                if (modules[idx].second.length() > 0)
                {
                    dassert(modules[idx].second == bridge_args, "inconsistent dmodule_bridge_arguments of module %s", module.c_str());
                }
                else
                {
                    modules[idx].second = bridge_args;
                }
            }
        }
    }

    // do the real job
    for (auto m : modules)
    {
        auto hmod = ::dsn::utils::load_dynamic_library(m.first.c_str());
        if (nullptr == hmod)
        {
            dassert(false, "cannot load shared library %s specified in config file",
                m.first.c_str());
            break;
        }
        else
        {
            dwarn("load shared library %s successfully", m.first.c_str());
        }

// attribute(contructor) is not reliable on *nix
#ifndef  _WIN32
        typedef void(*dsn_module_init_fn)();
        dsn_module_init_fn init_fn = (dsn_module_init_fn)::dsn::utils::load_symbol(hmod, "dsn_module_init");
        dassert(init_fn != nullptr,
            "dsn_module_init is not present (%s), use MODULE_INIT_BEGIN/END to define it",
            m.first.c_str()
        );
        init_fn();
#endif // ! _WIN32
        
        // have dmodule_bridge_arguments?
        if (m.second.length() > 0)
        {
            dsn_app_bridge_t bridge_ptr = (dsn_app_bridge_t)::dsn::utils::load_symbol(hmod, "dsn_app_bridge");
            dassert(bridge_ptr != nullptr,
                "when dmodule_bridge_arguments is present (%s), function dsn_app_bridge must be implemented in module %s",
                m.second.c_str(),
                m.first.c_str()
            );

            ddebug("call %s.dsn_app_bridge(...%s...)",
                m.first.c_str(),
                m.second.c_str()
            );

            std::vector<std::string> args;
            std::vector<const char*> args_ptr;
            ::dsn::utils::split_args(m.second.c_str(), args);

            for (auto& arg : args)
            {
                args_ptr.push_back(arg.c_str());
            }

            bridge_ptr((int)args_ptr.size(), &args_ptr[0]);
        }
    }
}

void run_all_unit_tests_prepare_when_necessary();
bool run(const char* config_file, const char* config_arguments, bool sleep_after_init, std::string& app_list)
{
    ::dsn::task::set_tls_dsn_context(nullptr, nullptr, nullptr);

    dsn_all.engine_ready = false;
    dsn_all.config_completed = false;
    dsn_all.tool = nullptr;
    dsn_all.engine = &::dsn::service_engine::instance();
    dsn_all.config.reset(new ::dsn::configuration());
    dsn_all.memory = nullptr;
    dsn_all.magic = 0xdeadbeef;

    if (!dsn_all.config->load(config_file, config_arguments))
    {
        printf("Fail to load config file %s\n", config_file);
        return false;
    }

    // pause when necessary
    if (dsn_all.config->get_value<bool>("core", "pause_on_start", false,
        "whether to pause at startup time for easier debugging"))
    {
#if defined(_WIN32)
        printf("\nPause for debugging (pid = %d)...\n", static_cast<int>(::GetCurrentProcessId()));
#else
        printf("\nPause for debugging (pid = %d)...\n", static_cast<int>(getpid()));
#endif
        getchar();
    }

    // load plugged modules
    load_all_modules(dsn_all.config);

    // prepare unit test run if necessary
    run_all_unit_tests_prepare_when_necessary();
    
    for (int i = 0; i <= dsn_task_code_max(); i++)
    {
        dsn_all.task_specs.push_back(::dsn::task_spec::get(i));
    }

    // initialize global specification from config file
    ::dsn::service_spec spec;
    if (!spec.init())
    {
        printf("error in config file %s, exit ...\n", config_file);
        return false;
    }

    dsn_all.config_completed = true;

    // setup data dir
    auto& data_dir = spec.data_dir;
    dassert(!dsn::utils::filesystem::file_exists(data_dir), "%s should not be a file.", data_dir.c_str());
    if (!dsn::utils::filesystem::directory_exists(data_dir.c_str()))
    {
        if (!dsn::utils::filesystem::create_directory(data_dir))
        {
            dassert(false, "Fail to create %s.", data_dir.c_str());
        }
    }
    std::string cdir;
    if (!dsn::utils::filesystem::get_absolute_path(data_dir.c_str(), cdir))
    {
        dassert(false, "Fail to get absolute path from %s.", data_dir.c_str());
    }
    spec.data_dir = cdir;

    // setup coredump dir
    spec.dir_coredump = ::dsn::utils::filesystem::path_combine(cdir, "coredumps");
    dsn::utils::filesystem::create_directory(spec.dir_coredump);
    ::dsn::utils::coredump::init(spec.dir_coredump.c_str());

    // setup log dir
    spec.dir_log = ::dsn::utils::filesystem::path_combine(cdir, "logs");
    dsn::utils::filesystem::create_directory(spec.dir_log);
    
    // init tools
    dsn_all.tool = ::dsn::utils::factory_store< ::dsn::tools::tool_app>::create(spec.tool.c_str(), ::dsn::PROVIDER_TYPE_MAIN, spec.tool.c_str());
    dsn_all.tool->install(spec);

    // init app specs
    if (!spec.init_app_specs())
    {
        printf("error in config file %s, exit ...\n", config_file);
        return false;
    }

    // init tool memory
    auto tls_trans_memory_KB = (size_t)dsn_all.config->get_value<int>(
        "core", "tls_trans_memory_KB",
        1024, // 1 MB
        "thread local transient memory buffer size (KB), default is 1024"
        );
    ::dsn::tls_trans_mem_init(tls_trans_memory_KB * 1024);
    dsn_all.memory = ::dsn::utils::factory_store< ::dsn::memory_provider>::create(
        spec.tools_memory_factory_name.c_str(), ::dsn::PROVIDER_TYPE_MAIN);

    // prepare minimum necessary
    ::dsn::service_engine::fast_instance().init_before_toollets(spec);

    // init logging    
    dsn_log_init();

    // init toollets
    for (auto it = spec.toollets.begin(); it != spec.toollets.end(); ++it)
    {
        auto tlet = dsn::tools::internal_use_only::get_toollet(it->c_str(), ::dsn::PROVIDER_TYPE_MAIN);
        dassert(tlet, "toolet not found");
        tlet->install(spec);
    }

    // init provider specific system inits
    dsn::tools::sys_init_before_app_created.execute();

    // TODO: register sys_exit execution

    // init runtime
    ::dsn::service_engine::fast_instance().init_after_toollets();

    dsn_all.engine_ready = true;

    // split app_name and app_index
    std::list<std::string> applistkvs;
    ::dsn::utils::split_args(app_list.c_str(), applistkvs, ';');
    
    // init apps
    for (auto& sp : spec.app_specs)
    {
        if (!sp.run)
            continue;

        bool create_it = false;

        if (app_list == "") // create all apps
        {
            create_it = true;
        }
        else
        {
            for (auto &kv : applistkvs)
            {
                std::list<std::string> argskvs;
                ::dsn::utils::split_args(kv.c_str(), argskvs, '@');
                if (std::string("apps.") + argskvs.front() == sp.config_section)
                {
                    if (argskvs.size() < 2)
                        create_it = true;
                    else
                        create_it = (std::stoi(argskvs.back()) == sp.index);
                    break;
                }
            }
        }

        if (create_it)
        {
            ::dsn::service_engine::fast_instance().start_node(sp);
        }
    }
        
    // start cli if necessary
    if (dsn_all.config->get_value<bool>("core", "cli_local", true,
        "whether to enable local command line interface (cli)"))
    {
        ::dsn::command_manager::instance().start_local_cli();
    }

    if (dsn_all.config->get_value<bool>("core", "cli_remote", true,
        "whether to enable remote command line interface (using dsn.cli)"))
    {
        ::dsn::command_manager::instance().start_remote_cli();
    }

    // register local cli commands
    ::dsn::register_command("config-dump",
        "config-dump - dump configuration",
        "config-dump [to-this-config-file]",
        [](const std::vector<std::string>& args)
    {
        std::ostringstream oss;
        std::ofstream off;
        std::ostream* os = &oss;
        if (args.size() > 0)
        {
            off.open(args[0]);
            os = &off;

            oss << "config dump to file " << args[0] << std::endl;
        }

        dsn_all.config->dump(*os);
        return oss.str();
    });
    
    // invoke customized init after apps are created
    dsn::tools::sys_init_after_app_created.execute();

    // start the tool
    dsn_all.tool->run();

    // add this to allow mimic app call from this thread.
    memset((void*)&dsn::tls_dsn, 0, sizeof(dsn::tls_dsn));

    //
    if (sleep_after_init)
    {
        while (true)
        {
            std::this_thread::sleep_for(std::chrono::hours(1));
        }
    }

    return true;
}


//
// run the system with arguments
//   config [-cargs k1=v1;k2=v2] [-app_list app_name1@index1;app_name2@index]
// e.g., config.ini -app_list replica@1 to start the first replica as a new process
//       config.ini -app_list replica to start ALL replicas (count specified in config) as a new process
//       config.ini -app_list replica -cargs replica-port=34556 to start ALL replicas with given port variable specified in config.ini
//       config.ini to start ALL apps as a new process
//
DSN_API void dsn_run(int argc, char** argv, bool sleep_after_init)
{
    if (argc < 2)
    {
        printf("invalid options for dsn_run\n"
            "// run the system with arguments\n"
            "//   config [-cargs k1=v1;k2=v2] [-app_list app_name1@index1;app_name2@index]\n"
            "// e.g., config.ini -app_list replica@1 to start the first replica as a new process\n"
            "//       config.ini -app_list replica to start ALL replicas (count specified in config) as a new process\n"
            "//       config.ini -app_list replica -cargs replica-port=34556 to start with %%replica-port%% var in config.ini\n"
            "//       config.ini to start ALL apps as a new process\n"
        );
        exit(1);
        return;
    }

    char* config = argv[1];
    std::string config_args = "";
    std::string app_list = "";

    for (int i = 2; i < argc;)
    {
        if (0 == strcmp(argv[i], "-cargs"))
        {
            if (++i < argc)
            {
                config_args = std::string(argv[i++]);
            }
        }

        else if (0 == strcmp(argv[i], "-app_list"))
        {
            if (++i < argc)
            {
                app_list = std::string(argv[i++]);
            }
        }
        else
        {
            printf("unknown arguments %s\n", argv[i]);
            exit(1);
            return;
        }
    }

    if (!run(config, config_args.size() > 0 ? config_args.c_str() : nullptr, sleep_after_init, app_list))
    {
        printf("run the system failed\n");
        dsn_exit(-1);
        return;
    }
}

DSN_API bool dsn_run_config(const char* config, bool sleep_after_init)
{
    std::string name;
    return run(config, nullptr, sleep_after_init, name);
}

DSN_API int dsn_get_all_apps(dsn_app_info* info_buffer, int count)
{
    auto& as = ::dsn::service_engine::fast_instance().get_all_nodes();
    int i = 0;
    for (auto& kv : as)
    {
        if (i >= count)
            return (int)as.size();

        dsn::service_node* node = kv.second;
        dsn_app_info& info = info_buffer[i++];
        info.app.app_context_ptr = node->get_app_context_ptr();
        info.app_id = node->id();
        info.index = node->spec().index;
        info.primary_address = node->rpc(nullptr)->primary_address().c_addr();
        strncpy(info.role, node->spec().role_name.c_str(), sizeof(info.role));
        strncpy(info.type, node->spec().type.c_str(), sizeof(info.type));
        strncpy(info.name, node->spec().name.c_str(), sizeof(info.name));
    }
    return i;
}


namespace dsn
{
    configuration_ptr get_main_config()
    {
        return dsn_all.config;
    }

    namespace tools
    {
        bool is_engine_ready()
        {
            return dsn_all.is_engine_ready();
        }

        tool_app* get_current_tool()
        {
            return dsn_all.tool;
        }
    }
}

