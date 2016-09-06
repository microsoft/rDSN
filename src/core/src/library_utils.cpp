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

# include <dsn/cpp/utils.h>
# include <dsn/utility/singleton.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <random>

# if defined(__linux__)
# include <sys/syscall.h>
# include <dlfcn.h> 
# elif defined(__FreeBSD__)
# include <sys/thr.h>
# include <dlfcn.h> 
# elif defined(__APPLE__)
# include <pthread.h>
# include <dlfcn.h> 
# endif

# ifdef __TITLE__
# undef __TITLE__
# endif
# define __TITLE__ "dsn.utils"

namespace dsn {
    namespace utils {


        dsn_handle_t load_dynamic_library(const char* module, const std::vector<std::string>& search_dirs)
        {
            std::string module_name(module);
# if defined(_WIN32)
            module_name += ".dll";
# elif defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__)
            module_name = "lib" + module_name + ".so";
# else
# error not implemented yet
# endif

            // search given dirs with priority
            std::vector<std::string> passes;
            for (auto & dr : search_dirs)
            {
                passes.push_back(utils::filesystem::path_combine(dr, module_name));
            }

            // search OS given passes
            passes.push_back(module_name);

            // try 
            dsn_handle_t hmod = nullptr;
            for (auto & m : passes)
            {
# if defined(_WIN32)
                hmod = (dsn_handle_t)::LoadLibraryA(m.c_str());
                if (hmod == nullptr)
                {
                    ddebug("load dynamic library '%s' failed, err = %d", m.c_str(), ::GetLastError());
                }
# else
                hmod = (dsn_handle_t)dlopen(m.c_str(), RTLD_LAZY | RTLD_GLOBAL);
                if (nullptr == hmod)
                {
                    ddebug("load dynamic library '%s' failed, err = %s", m.c_str(), dlerror());
                }
# endif 
                else
                    break;
            }

            if (hmod == nullptr)
            {
                derror("load dynamic library '%s' failed, check logs for details", module_name.c_str());
            }
            return hmod;
        }

        dsn_handle_t load_symbol(dsn_handle_t hmodule, const char* symbol)
        {
# if defined(_WIN32)
            return (dsn_handle_t)::GetProcAddress((HMODULE)hmodule, (LPCSTR)symbol);
# else
            return (dsn_handle_t)dlsym((void*)hmodule, symbol);
# endif 
        }

        void unload_dynamic_library(dsn_handle_t hmodule)
        {
# if defined(_WIN32)
            ::CloseHandle((HMODULE)hmodule);
# else
            dlclose((void*)hmodule);
# endif
        }
    }
}

