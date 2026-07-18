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

# include "simple_logger.h"
# include <sstream>
# include <cerrno>
# include <cstring>
# include <cstdint>
# include <cstdarg>
# include <cinttypes>
# include <string>
# include <vector>

namespace dsn {
    namespace tools {

        screen_logger::screen_logger(const char* log_dir, logging_provider* inner)
            : logging_provider(log_dir, inner)
        {
            _short_header = dsn_config_get_value_bool("tools.screen_logger", "short_header",
                true, "whether to use short header (excluding file/function etc.)");
        }

        screen_logger::~screen_logger(void)
        {
        }

        void screen_logger::dsn_logv(const char *file,
            const char *function,
            const int line,
            dsn_log_level_t log_level,
            const char* title,
            const char *fmt,
            va_list args
            )
        {
            utils::auto_lock< ::dsn::utils::ex_lock_nr> l(_lock);

            // write logs to stderr (not stdout) so that stdout stays clean for
            // machine-readable program output (metrics, cli results, etc.)
            logging_provider::print_header(stderr, log_level);
            if (!_short_header)
            {
                fprintf(stderr, "%s:%d:%s(): ", title, line, function);
            }
            vfprintf(stderr, fmt, args);
            fprintf(stderr, "\n");
        }

        void screen_logger::flush()
        {
            ::fflush(stderr);
        }

        simple_logger::simple_logger(const char* log_dir, logging_provider* inner)
            : logging_provider(log_dir, inner)
        {
            _log_dir = std::string(log_dir);
            //we assume all valid entries are positive
            _start_index = 0;
            _index = 1;
            _lines = 0;
            _log = nullptr;
            _short_header = dsn_config_get_value_bool("tools.simple_logger", "short_header", 
                true, "whether to use short header (excluding file/function etc.)");
            _fast_flush = dsn_config_get_value_bool("tools.simple_logger", "fast_flush",
                false, "whether to flush immediately");
            _stderr_start_level = enum_from_string(
                        dsn_config_get_value_string("tools.simple_logger", "stderr_start_level",
                            enum_to_string(LOG_LEVEL_WARNING),
                            "copy log messages at or above this level to stderr in addition to logfiles"),
                        LOG_LEVEL_INVALID
                        );
            if (_stderr_start_level == LOG_LEVEL_INVALID)
            {
                // Do not abort logger initialization on an invalid config value: fall back to the
                // default level. Use stderr directly because the logging system is not fully
                // initialized at this point.
                fprintf(stderr,
                        "invalid [tools.simple_logger] stderr_start_level specified, "
                        "falling back to WARNING\n");
                _stderr_start_level = LOG_LEVEL_WARNING;
            }

            _max_number_of_log_files_on_disk = dsn_config_get_value_uint64(
                "tools.simple_logger",
                "max_number_of_log_files_on_disk",
                20,
                "max number of log files reserved on disk, older logs are auto deleted"
                );

            // check existing log files
            std::vector<std::string> sub_list;
            if (!dsn::utils::filesystem::get_subfiles(_log_dir, sub_list, false))
            {
                // Do not abort during logger initialization if the log directory cannot be listed:
                // skip scanning existing files and continue with a fresh index. Use stderr directly
                // because the logging system is not fully initialized at this point. Existing logs are
                // preserved because create_log_file() opens in append mode rather than truncating.
                fprintf(stderr, "Fail to get subfiles in %s, skip scanning existing log files.\n",
                    _log_dir.c_str());
                sub_list.clear();
            }
            for (auto& fpath : sub_list)
            {
                auto&& name = dsn::utils::filesystem::get_file_name(fpath);
                if (name.length() <= 8 ||
                    name.substr(0, 4) != "log.")
                    continue;

                int index;
                if (1 != sscanf(name.c_str(), "log.%d.txt", &index) || index <= 0)
                    continue;

                if (index > _index)
                    _index = index;

                if (_start_index == 0 || index < _start_index)
                    _start_index = index;
            }
            sub_list.clear();

            if (_start_index == 0)
            {
                _start_index = _index;
            }
            else
                ++_index;

            create_log_file();
        }

        void simple_logger::create_log_file()
        {
            if (_log != nullptr)
            {
                if (::fclose(_log) != 0)
                {
                    fprintf(stderr, "failed to close log file, err = %s\n", strerror(errno));
                }
            }

            _lines = 0;

            std::stringstream str;
            // so now the start index is from 0
            str << _log_dir << "/log." << _index++ << ".txt";
            // Open in append mode ("a+") rather than truncating ("w+"): during normal rotation the
            // computed index refers to a fresh file so this is equivalent, but if scanning existing
            // files was skipped (e.g. get_subfiles() failed and we reset to index 1) this avoids
            // clobbering logs that already exist on disk.
            _log = ::fopen(str.str().c_str(), "a+");
            if (_log == nullptr)
            {
                fprintf(stderr, "failed to open log file %s: %s\n", str.str().c_str(), strerror(errno));
                return;
            }

            // TODO: move gc out of criticial path
            while (_index - _start_index > _max_number_of_log_files_on_disk)
            {
                std::stringstream str2;
                str2 << "log." << _start_index++ << ".txt";
                auto dp = utils::filesystem::path_combine(_log_dir, str2.str());
                if (::remove(dp.c_str()) != 0)
                {
                    fprintf(stderr, "Failed to remove garbage log file %s\n", dp.c_str());
                    _start_index--;
                    break;
                }
            }
        }

        simple_logger::~simple_logger(void) 
        { 
            utils::auto_lock< ::dsn::utils::ex_lock_nr> l(_lock);
            if (_log != nullptr)
            {
                if (::fclose(_log) != 0)
                {
                    fprintf(stderr, "failed to close log file, err = %s\n", strerror(errno));
                }
            }
        }

        void simple_logger::flush()
        {
            utils::auto_lock< ::dsn::utils::ex_lock_nr> l(_lock);
            if (_log != nullptr)
            {
                if (::fflush(_log) != 0)
                {
                    fprintf(stderr, "failed to flush log file, err = %s\n", strerror(errno));
                }
            }
            ::fflush(stdout);
        }

        void simple_logger::dsn_logv(const char *file,
            const char *function,
            const int line,
            dsn_log_level_t log_level,
            const char* title,
            const char *fmt,
            va_list args
            )
        {
            va_list args2;
            if (log_level >= _stderr_start_level)
            {
                va_copy(args2, args);
            }

            utils::auto_lock< ::dsn::utils::ex_lock_nr> l(_lock);
            if (_log == nullptr)
            {
                create_log_file();
                if (_log == nullptr)
                {
                    if (log_level >= _stderr_start_level)
                    {
                        logging_provider::print_header(stderr, log_level);
                        if (!_short_header)
                        {
                            fprintf(stderr, "%s:%d:%s(): ", title, line, function);
                        }
                        vfprintf(stderr, fmt, args);
                        fprintf(stderr, "\n");
                    }
                    if (log_level >= _stderr_start_level)
                    {
                        va_end(args2);
                    }
                    return;
                }
            }
         
            logging_provider::print_header(_log, log_level);
            if (!_short_header)
            {
                fprintf(_log, "%s:%d:%s(): ", title, line, function);
            }
            vfprintf(_log, fmt, args);
            fprintf(_log, "\n");
            if (_fast_flush || log_level >= LOG_LEVEL_ERROR)
            {
                if (::fflush(_log) != 0)
                {
                    fprintf(stderr, "failed to flush log file, err = %s\n", strerror(errno));
                }
            }

            if (log_level >= _stderr_start_level)
            {
                logging_provider::print_header(stderr, log_level);
                if (!_short_header)
                {
                    fprintf(stderr, "%s:%d:%s(): ", title, line, function);
                }
                vfprintf(stderr, fmt, args2);
                fprintf(stderr, "\n");
                va_end(args2);
            }

            if (++_lines >= 200000)
            {
                create_log_file();
            }
        }
    }
}
