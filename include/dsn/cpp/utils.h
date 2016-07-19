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

# include <dsn/cpp/auto_codes.h>
# include <dsn/cpp/callocator.h>
# include <functional>

# ifdef __TITLE__
# undef __TITLE__
# endif
# define __TITLE__ "utils"

# ifdef __cplusplus
extern "C" {
# endif

# ifdef _WIN32

enum
{
    FTW_F,        /* Regular file.  */
#define FTW_F    FTW_F
    FTW_D,        /* Directory.  */
#define FTW_D    FTW_D
    FTW_DNR,      /* Unreadable directory.  */
#define FTW_DNR  FTW_DNR
    FTW_NS,       /* Unstatable file.  */
#define FTW_NS   FTW_NS

    FTW_SL,       /* Symbolic link.  */
# define FTW_SL  FTW_SL
                    /* These flags are only passed from the `nftw' function.  */
    FTW_DP,       /* Directory, all subdirs have been visited. */
# define FTW_DP  FTW_DP
    FTW_SLN       /* Symbolic link naming non-existing file.  */
# define FTW_SLN FTW_SLN
};

struct FTW
{
    int base;
    int level;
};

# else

#ifndef _XOPEN_SOURCE
# define _XOPEN_SOURCE 500
#endif

# include <ftw.h>

# endif

#ifndef FTW_CONTINUE
# define FTW_CONTINUE 0
#endif

#ifndef FTW_STOP
# define FTW_STOP 1
#endif

#ifndef FTW_SKIP_SUBTREE
# define FTW_SKIP_SUBTREE 2
#endif

#ifndef FTW_SKIP_SIBLINGS
# define FTW_SKIP_SIBLINGS 3
#endif

# ifdef __cplusplus
}
# endif

namespace dsn {
    namespace utils {

        extern void split_args(const char* args, /*out*/ std::vector<std::string>& sargs, char splitter = ' ');
        extern void split_args(const char* args, /*out*/ std::list<std::string>& sargs, char splitter = ' ');
        extern std::string replace_string(std::string subject, const std::string& search, const std::string& replace);
        extern std::string get_last_component(const std::string& input, const char splitters[]);

        extern char* trim_string(char* s);

        extern uint64_t get_current_physical_time_ns();

        inline uint64_t get_current_rdtsc()
        {
# ifdef _WIN32
            return __rdtsc();
# else
            unsigned hi, lo;
            __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
            return ((unsigned long long)lo) | (((unsigned long long)hi) << 32);
# endif
        }

        extern void time_ms_to_string(uint64_t ts_ms, char* str);

        extern int get_current_tid_internal();

        typedef struct _tls_tid
        {
            unsigned int magic;
            int local_tid;
        } tls_tid;
        extern __thread tls_tid s_tid;

        inline int get_current_tid()
        {
            if (s_tid.magic == 0xdeadbeef)
                return s_tid.local_tid;
            else
            {
                s_tid.magic = 0xdeadbeef;
                s_tid.local_tid = get_current_tid_internal();                
                return s_tid.local_tid;
            }
        }

        inline int get_invalid_tid() { return -1; }

        namespace filesystem {

            extern bool get_absolute_path(const std::string& path1, std::string& path2);

            extern std::string remove_file_name(const std::string& path);

            extern std::string get_file_name(const std::string& path);

            extern std::string path_combine(const std::string& path1, const std::string& path2);

            extern int get_normalized_path(const std::string& path, std::string& npath);

            //int (const char* fpath, int typeflags, struct FTW *ftwbuf)
            typedef std::function<int(const char*, int, struct FTW*)> ftw_handler;

            extern bool file_tree_walk(
                const std::string& dirpath,
                ftw_handler handler,
                bool recursive = true
                );

            extern bool path_exists(const std::string& path);

            extern bool directory_exists(const std::string& path);

            extern bool file_exists(const std::string& path);

            extern bool get_subfiles(const std::string& path, std::vector<std::string>& sub_list, bool recursive);

            extern bool get_subdirectories(const std::string& path, std::vector<std::string>& sub_list, bool recursive);

            extern bool get_subpaths(const std::string& path, std::vector<std::string>& sub_list, bool recursive);

            extern bool remove_path(const std::string& path);
            
            // this will always remove target path if exist
            extern bool rename_path(const std::string& path1, const std::string& path2);

            extern bool file_size(const std::string& path, int64_t& sz);

            extern bool create_directory(const std::string& path);

            extern bool create_file(const std::string& path);

            extern bool get_current_directory(std::string& path);

            extern bool last_write_time(std::string& path, time_t& tm);

            extern error_code get_process_image_path(int pid, std::string& path);

            inline error_code get_current_process_image_path(std::string& path)
            {
                auto err = dsn::utils::filesystem::get_process_image_path(-1, path);
                dassert(err == ERR_OK, "get_current_process_image_path failed.");
                return err;
            }
        }
    }
} // end namespace dsn::utils

