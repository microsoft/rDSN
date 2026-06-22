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
# include <gtest/gtest.h>
# include <dsn/cpp/serialization.h>

# include "idl_test.types.h"

# include <fstream>
# include <iostream>
# include <iterator>
# include <vector>
# include "stdlib.h"

//#define DSN_IDL_TESTS_DEBUG

enum Language {lang_cpp, lang_csharp};
enum IDL{idl_protobuf, idl_thrift};
enum Format{format_binary, format_json};
const char* kGeneratedProjectScratchBuildDir = "generated_build";
const char* kTestScratchDir = "dsn.idl.tests";

std::string file(const std::string &val)
{
    std::string nval;
    dsn::utils::filesystem::get_normalized_path(val, nval);
    return nval;
}

std::string file(const char* val)
{
    return file(std::string(val));
}

std::string combine(const std::string &dir, const char* sub)
{
    return dsn::utils::filesystem::path_combine(file(dir), file(sub));
}

std::string combine(const std::string &dir,const std::string &sub)
{
    return dsn::utils::filesystem::path_combine(file(dir), file(sub));
}

std::string get_test_scratch_dir();
std::string scratch_file(const std::string &path);
std::string scratch_file(const char* path);

void execute(std::string cmd, bool &result)
{
#ifdef DSN_IDL_TESTS_DEBUG
    std::cout << cmd << std::endl;
#else
    cmd = cmd + std::string(" >> ") + scratch_file("log") + " 2>&1";
#endif
    const int err = system(cmd.c_str());
    bool ret = (err == 0);
    if (!ret)
    {
        std::cerr << "Command failed with exit code " << err << ": " << cmd << std::endl;
    }
    result = result && ret;
}

void dump_log_on_failure(bool result)
{
#ifndef DSN_IDL_TESTS_DEBUG
    const std::string log_file = scratch_file("log");
    if (result || !::dsn::utils::filesystem::file_exists(log_file))
    {
        return;
    }

    std::ifstream input(log_file.c_str(), std::ios::in | std::ios::binary);
    if (input.is_open())
    {
        std::cerr << input.rdbuf();
    }
#endif
}

void copy_file(const std::string& src, const std::string& dst, bool &result)
{
#ifdef _WIN32
    std::string cmd = std::string("copy /Y ");
#else
    std::string cmd = std::string("cp -f ");
#endif
    cmd += src + " " + dst;
    execute(cmd, result);
}

void replace_in_file(const std::string& path, const std::string& from, const std::string& to, bool &result)
{
    std::ifstream input(file(path).c_str(), std::ios::in | std::ios::binary);
    if (!input.is_open())
    {
        result = false;
        return;
    }

    std::string content((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    input.close();

    const size_t pos = content.find(from);
    if (pos == std::string::npos)
    {
        result = false;
        return;
    }

    content.replace(pos, from.length(), to);

    std::ofstream output(file(path).c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
    if (!output.is_open())
    {
        result = false;
        return;
    }

    output << content;
    result = result && output.good();
}

void create_dir(const char* dir, bool &result)
{
    bool ret = dsn::utils::filesystem::create_directory(file(dir));
    result = result && ret;
}

void require_file(const std::string& path, const char* description, bool &result)
{
    if (!::dsn::utils::filesystem::file_exists(file(path)))
    {
        std::cerr << "Failed to generate " << description << ": " << file(path) << std::endl;
        result = false;
    }
}

void rm_dir(const char* dir, bool &result)
{
    const bool ret = dsn::utils::filesystem::remove_path(file(dir));
    result = result && ret;
}

void cleanup_generated_project(bool &result)
{
    rm_dir(get_test_scratch_dir().c_str(), result);
}

std::string get_generated_project_dsn_root()
{
    if (::dsn::utils::filesystem::file_exists(combine(DSN_INSTALL_ROOT_DIR, "bin/dsn.cmake")))
    {
        return file(DSN_INSTALL_ROOT_DIR);
    }

    return file(DSN_ROOT_DIR);
}

std::string get_dsn_build_dir()
{
#ifdef DSN_BUILD_DIR
    return file(DSN_BUILD_DIR);
#else
    return file(combine(DSN_ROOT_DIR, "builder"));
#endif
}

std::string get_test_scratch_dir()
{
    const char* test_tmp_root = getenv("DSN_TEST_TMP_DIR");
    const std::string root =
        (test_tmp_root != nullptr && test_tmp_root[0] != '\0')
            ? file(test_tmp_root)
            : combine(get_dsn_build_dir(), "test_tmp");
    return combine(root, kTestScratchDir);
}

std::string scratch_file(const std::string &path)
{
    return combine(get_test_scratch_dir(), path);
}

std::string scratch_file(const char* path)
{
    return scratch_file(std::string(path));
}

void cleanup_test_scratch_dir(bool &result)
{
    rm_dir(get_test_scratch_dir().c_str(), result);
}

void prepare_test_scratch_dir(bool &result)
{
    cleanup_test_scratch_dir(result);
    create_dir(get_test_scratch_dir().c_str(), result);
}

void copy_idl_files_to_scratch_dir(bool &result)
{
    std::vector<std::string> idl_files;
    idl_files.push_back("repo/counter.proto");
    idl_files.push_back("repo/counter.proto.annotations");
    idl_files.push_back("repo/counter.thrift");
    idl_files.push_back("repo/counter.thrift.annotations");
    for (auto i : idl_files)
    {
        copy_file(combine(DSN_ROOT_DIR "/src/tests/idl/resources", i), get_test_scratch_dir(), result);
    }
}

std::string get_codegen_script()
{
#ifdef WIN32
    const char* script = "bin/dsn.cg.bat";
#else
    const char* script = "bin/dsn.cg.sh";
#endif
    std::string installed_script = combine(DSN_INSTALL_ROOT_DIR, script);
    if (::dsn::utils::filesystem::file_exists(installed_script))
    {
        return file(installed_script);
    }

    return file(combine(DSN_ROOT_DIR, script));
}

std::string get_codegen_command()
{
#ifdef _WIN32
    return "set \"DSN_BUILD_DIR=" + get_dsn_build_dir() + "\" && cd /d " + get_test_scratch_dir() + " && " + get_codegen_script();
#else
    return "cd " + get_test_scratch_dir() + " && DSN_BUILD_DIR=\"" + get_dsn_build_dir() + "\" " + get_codegen_script();
#endif
}

std::string get_boost_include_dir()
{
#ifdef DSN_BOOST_INCLUDEDIR
    return file(DSN_BOOST_INCLUDEDIR);
#else
    return std::string();
#endif
}

std::string run_generated_project_command(const std::string &cmd)
{
#if defined(__linux__)
    std::string library_path = combine(get_generated_project_dsn_root(), "lib");
#ifdef DSN_GTEST_LIB_DIR
    library_path += ":" + file(DSN_GTEST_LIB_DIR);
#endif
    return "cd " + get_test_scratch_dir() + " && LD_LIBRARY_PATH=\"" + library_path + ":$LD_LIBRARY_PATH\" " + cmd;
#elif defined(_WIN32)
    return "cd /d " + get_test_scratch_dir() + " && " + cmd;
#else
    return "cd " + get_test_scratch_dir() + " && " + cmd;
#endif
}

void cmake(Language lang, bool &result)
{
    const std::string generated_build_dir = scratch_file(kGeneratedProjectScratchBuildDir);
    create_dir(generated_build_dir.c_str(), result);
        
#ifdef _WIN32
    std::string cmake_cmd = std::string("cd /d ") + generated_build_dir + " && cmake " + scratch_file("src");
    cmake_cmd += std::string(" -DCMAKE_GENERATOR_PLATFORM=x64");
#else
    std::string cmake_cmd = std::string("cd ") + generated_build_dir + " && cmake " + scratch_file("src");
#endif
    cmake_cmd += std::string(" -DDSN_ROOT=") + get_generated_project_dsn_root();
    const std::string boost_include_dir = get_boost_include_dir();
    if (!boost_include_dir.empty())
    {
        cmake_cmd += std::string(" -DBOOST_INCLUDEDIR=") + boost_include_dir;
    }
    
    execute(cmake_cmd, result);
    if (!result)
    {
        std::cerr << "Failed to configure generated counter project with CMake." << std::endl;
        dump_log_on_failure(result);
        return;
    }

    if (lang == lang_cpp)
    {
#ifdef _WIN32
        execute(std::string("msbuild ") + file(combine(generated_build_dir, "counter.sln")), result);
        if (!result)
        {
            std::cerr << "Failed to build generated counter project with MSBuild." << std::endl;
            return;
        }
        execute(file(combine(generated_build_dir, "bin/counter/Debug/counter.exe")) + " " +
                    file(combine(generated_build_dir, "bin/counter/config.ini")),
                result);
#else
        execute(std::string("cd ") + generated_build_dir + " && make ", result);
        if (!result)
        {
            std::cerr << "Failed to build generated counter project with make." << std::endl;
            return;
        }
        execute(run_generated_project_command(
                    file(combine(generated_build_dir, "bin/counter/counter")) + " " +
                        file(combine(generated_build_dir, "bin/counter/config.ini"))),
                result);
#endif
    }
    else
    {
        execute(file(combine(generated_build_dir, "bin/counter/counter.exe")) + " " +
                    file(combine(generated_build_dir, "bin/counter/config.ini")),
                result);
    }
    if (!result)
    {
        std::cerr << "Failed to run generated counter project." << std::endl;
    }
}

bool test_code_generation(Language lang, IDL idl, Format format)
{
    bool result = true;
    std::string codegen_cmd = get_codegen_command()
        + std::string(" counter.")
        + (idl == idl_protobuf ? "proto" : "thrift")
        + (lang == lang_cpp ? " cpp" : " csharp")
        + " src "
        + (format == format_binary ? "binary" : "json")
        + " single";
    cleanup_generated_project(result);
    if (!result)
    {
        std::cerr << "Failed to clean IDL test scratch directory: " << get_test_scratch_dir()
                  << std::endl;
#ifdef _WIN32
        std::cerr << "Close previous tests or terminals that are using this directory, then retry."
                  << std::endl;
#endif
        return false;
    }
    create_dir(get_test_scratch_dir().c_str(), result);
    if (!result)
    {
        std::cerr << "Failed to create IDL test scratch directory: " << get_test_scratch_dir()
                  << std::endl;
        return false;
    }
    copy_idl_files_to_scratch_dir(result);
    if (!result)
    {
        std::cerr << "Failed to copy IDL test inputs to scratch directory: "
                  << get_test_scratch_dir() << std::endl;
        dump_log_on_failure(result);
        return false;
    }
    create_dir(scratch_file("src").c_str(), result);
    if (!result)
    {
        std::cerr << "Failed to create generated counter project directory: "
                  << scratch_file("src") << std::endl;
        dump_log_on_failure(result);
        return false;
    }
    execute(codegen_cmd, result);
    if (!result)
    {
        std::cerr << "Failed to run IDL codegen command: " << codegen_cmd << std::endl;
        dump_log_on_failure(result);
        return false;
    }
    require_file(scratch_file("src/CMakeLists.txt"), "counter project CMakeLists.txt", result);
    dump_log_on_failure(result);
    if (!result)
    {
        return false;
    }
    std::vector<std::string> src_files;
    std::string src_root(lang == lang_cpp ? "repo/cpp" : "repo/csharp");
    if (lang == lang_cpp)
    {
        src_files.push_back("counter.main.cpp");
        replace_in_file(
            scratch_file("src/CMakeLists.txt"),
            "dsn_add_shared_library()",
            "dsn_add_executable()",
            result);
        replace_in_file(
            scratch_file("src/config.ini"),
            "dsn.tools.nfs\ncounter\n\n[apps.server]\n",
            "dsn.tools.nfs\n\n[apps.server]\n",
            result);
        replace_in_file(
            scratch_file("src/config.ini"),
            "pause_on_start = false\n",
            "pause_on_start = false\ncli_local = false\ncli_remote = false\n",
            result);
    } else
    {
        src_files.push_back("counter.main.cs");
    }
    for (auto i : src_files)
    {
        copy_file(combine(combine(DSN_ROOT_DIR "/src/tests/idl/resources", src_root), i), scratch_file("src"), result);
    }
    cmake(lang, result);
    bool tmp = true;
    cleanup_generated_project(tmp);
    return result;
}

template<typename T>
void thrift_basic_type_serialization_checker(const std::vector<T> &data, Format fmt)
{
    const int bufsize = 2000;
    char buf[bufsize];
    for (const T& i : data)
    {
        T input = i;
        dsn::blob b(buf, 0, bufsize);
        dsn::binary_writer writer(b);
        if (fmt == format_binary)
        {
            dsn::marshall_thrift_binary(writer, input);
        }
        else
        {
            dsn::marshall_thrift_json(writer, input);
        }
        dsn::binary_reader reader(b);
        T output;
        if (fmt == format_binary)
        {
            dsn::unmarshall_thrift_binary(reader, output);
        }
        else
        {
            dsn::unmarshall_thrift_json(reader, output);
        }
        EXPECT_TRUE(input == output);
    }
}
void test_thrift_basic_type_serialization(Format fmt)
{
    std::vector<bool> data_bool_t{true, false};
    thrift_basic_type_serialization_checker(data_bool_t, fmt);

    std::vector<int8_t> data_int8_t{(std::numeric_limits<int8_t>::min)(), (std::numeric_limits<int8_t>::max)(), 0, 1 , -1, 13, -13};
    thrift_basic_type_serialization_checker(data_int8_t, fmt);

    std::vector<int16_t> data_int16_t{(std::numeric_limits<int16_t>::min)(), (std::numeric_limits<int16_t>::max)(), 0, 1 , -1, 13, -13};
    thrift_basic_type_serialization_checker(data_int16_t, fmt);

    std::vector<int32_t> data_int32_t{(std::numeric_limits<int32_t>::min)(), (std::numeric_limits<int32_t>::max)(), 0, 1 , -1, 13, -13};
    thrift_basic_type_serialization_checker(data_int32_t, fmt);

    std::vector<int64_t> data_int64_t{(std::numeric_limits<int64_t>::min)(), (std::numeric_limits<int64_t>::max)(), 0, 1 , -1, 13, -13};
    thrift_basic_type_serialization_checker(data_int32_t, fmt);

    std::vector<std::string> data_string_t{std::string("hello"), std::string("world"), std::string("")};
    thrift_basic_type_serialization_checker(data_string_t, fmt);

    //TODO:: test double type

    std::vector<std::vector<int32_t> > data_vec_int32_t{data_int32_t, std::vector<int32_t>()};
    thrift_basic_type_serialization_checker(data_vec_int32_t, fmt);

    std::map<int, std::string> m1;
    m1[1] = "hello";
    m1[233] = "world";
    m1[-22] = "";
    std::map<int, std::string> m2;
    std::vector<std::map<int, std::string> > data_map_int32_str_t{m1, m2};
    thrift_basic_type_serialization_checker(data_map_int32_str_t, fmt);
}

void check_thrift_generated_type_serialization(const dsn::idl::test::test_thrift_item &input, Format fmt)
{
    const int bufsize = 2000;
    char buf[bufsize];
    dsn::blob b(buf, 0, bufsize);
    dsn::binary_writer writer(b);
    if (fmt == format_binary)
    {
        dsn::marshall_thrift_binary(writer, input);
    }
    else
    {
        dsn::marshall_thrift_json(writer, input);
    }
    dsn::binary_reader reader(b);
    dsn::idl::test::test_thrift_item output;
    if (fmt == format_binary)
    {
        dsn::unmarshall_thrift_binary(reader, output);
    }
    else
    {
        dsn::unmarshall_thrift_json(reader, output);
    }
    EXPECT_EQ(input.bool_item, output.bool_item);
    EXPECT_EQ(input.byte_item, output.byte_item);
    EXPECT_EQ(input.i16_item, output.i16_item);
    EXPECT_EQ(input.i32_item, output.i32_item);
    EXPECT_EQ(input.i64_item, output.i64_item);
    EXPECT_EQ(input.list_i32_item, output.list_i32_item);
    EXPECT_EQ(input.set_i32_item, output.set_i32_item);
    EXPECT_EQ(input.map_i32_item, output.map_i32_item);
    EXPECT_DOUBLE_EQ(input.double_item, output.double_item);
}

void test_thrift_generated_type_serialization(Format fmt)
{
    const int container_n = 10;
    dsn::idl::test::test_thrift_item item;

    item.bool_item = false;
    item.byte_item = 0;
    item.i16_item = 0;
    item.i32_item = 0;
    item.i64_item = 0;
    item.double_item = 0.0;
    check_thrift_generated_type_serialization(item, fmt);

    item.bool_item = true;
    item.byte_item = (std::numeric_limits<int8_t>::max)();
    item.i16_item = (std::numeric_limits<int16_t>::max)();
    item.i32_item = (std::numeric_limits<int32_t>::max)();
    item.i64_item = (std::numeric_limits<int64_t>::max)();
    item.double_item = 123.321;
    item.string_item = "hello world";
    for (int i = 0; i < container_n; i++)
    {
        item.list_i32_item.push_back(i);
    }
    for (int i = 0; i < container_n; i++)
    {
        item.set_i32_item.insert(item.set_i32_item.begin(), i + 1);
    }
    for (int i = 0; i < container_n; i++)
    {
        item.map_i32_item[i] = i * 2;
    }
    check_thrift_generated_type_serialization(item, fmt);
}

void check_protobuf_generated_type_serialization(const dsn::idl::test::test_protobuf_item &input, Format fmt)
{
    const int bufsize = 2000;
    char buf[bufsize];
    dsn::blob b(buf, 0, bufsize);
    dsn::binary_writer writer(b);
    if (fmt == format_binary)
    {
        dsn::marshall_protobuf_binary(writer, input);
    }
    else
    {
        dsn::marshall_protobuf_json(writer, input);
    }
    dsn::binary_reader reader(b);
    dsn::idl::test::test_protobuf_item output;
    if (fmt == format_binary)
    {
        dsn::unmarshall_protobuf_binary(reader, output);
    }
    else
    {
        dsn::unmarshall_protobuf_json(reader, output);
    }
    EXPECT_EQ(input.bool_item(), output.bool_item());
    EXPECT_EQ(input.int32_item(), output.int32_item());
    EXPECT_EQ(input.int64_item(), output.int64_item());
    EXPECT_EQ(input.uint32_item(), output.uint32_item());
    EXPECT_EQ(input.uint64_item(), output.uint64_item());
    EXPECT_EQ(input.string_item(), output.string_item());
    EXPECT_FLOAT_EQ(input.float_item(), output.float_item());
    EXPECT_DOUBLE_EQ(input.double_item(), output.double_item());
    for (int i = 0; i < input.repeated_int32_item_size(); i++)
    {
        EXPECT_EQ(input.repeated_int32_item().Get(i) , output.repeated_int32_item().Get(i));
    }
    EXPECT_EQ(input.map_int32_item_size(), output.map_int32_item_size());
    for (auto i = input.map_int32_item().begin(); i != input.map_int32_item().end(); i++)
    {
        EXPECT_TRUE(output.map_int32_item().find(i->first) != output.map_int32_item().end());
        if (output.map_int32_item().find(i->first) != output.map_int32_item().end())
        {
            EXPECT_EQ(i->second, output.map_int32_item().at(i->first));
        }
    }
}

void test_protobuf_generated_type_serialization(Format fmt)
{
    const int container_n = 10;
    dsn::idl::test::test_protobuf_item item;

    check_protobuf_generated_type_serialization(item, fmt);

    item.set_bool_item(true);
    item.set_int32_item((std::numeric_limits<int32_t>::max)());
    item.set_int64_item((std::numeric_limits<int64_t>::max)());
    item.set_uint32_item((std::numeric_limits<uint32_t>::max)());
    item.set_uint64_item((std::numeric_limits<uint64_t>::max)());
    item.set_float_item(123.321);
    item.set_double_item(1234.4321);
    item.set_string_item("hello world");
    for (int i = 0; i < container_n; i++)
    {
        item.add_repeated_int32_item(i);
    }
    for (int i = 0; i < container_n; i++)
    {
        auto mp = item.mutable_map_int32_item();
        (*mp)[i] = 2 * i;
    }
    check_protobuf_generated_type_serialization(item, fmt);
}

bool prepare()
{
    bool ret = true;
    prepare_test_scratch_dir(ret);
    return ret;
}

/*
#ifdef WIN32

TEST(TEST_PROTOBUF_HELPER, CSHARP_BINARY)
{
    EXPECT_TRUE(test_code_generation(lang_csharp, idl_protobuf, format_binary));
}

TEST(TEST_PROTOBUF_HELPER, CSHARP_JSON)
{
    EXPECT_TRUE(test_code_generation(lang_csharp, idl_protobuf, format_json));
}

#endif
*/

TEST(thrift_helper, cpp_binary_basic_type_serialization)
{
    test_thrift_basic_type_serialization(format_binary);
}

TEST(thrift_helper, cpp_binary_generated_type_serialization)
{
    test_thrift_generated_type_serialization(format_binary);
}

TEST(thrift_helper, cpp_json_basic_type_serialization)
{
    test_thrift_basic_type_serialization(format_json);
}

TEST(thrift_helper, cpp_json_generated_type_serialization)
{
    test_thrift_generated_type_serialization(format_json);
}

TEST(thrift_helper, cpp_binary_code_generation)
{
    EXPECT_TRUE(test_code_generation(lang_cpp, idl_thrift, format_binary));
}

TEST(thrift_helper, cpp_json_code_generation)
{
    EXPECT_TRUE(test_code_generation(lang_cpp, idl_thrift, format_json));
}

TEST(protobuf_helper, cpp_binary_generated_type_serialization)
{
    test_protobuf_generated_type_serialization(format_binary);
}

TEST(protobuf_helper, cpp_json_generated_type_serialization)
{
    test_protobuf_generated_type_serialization(format_json);
}

TEST(protobuf_helper, cpp_binary_code_generation)
{
    EXPECT_TRUE(test_code_generation(lang_cpp, idl_protobuf, format_binary));
}

TEST(protobuf_helper, cpp_json_code_generation)
{
    EXPECT_TRUE(test_code_generation(lang_cpp, idl_protobuf, format_json));
}

/*
#ifdef WIN32

TEST(TEST_THRIFT_HELPER, CSHARP_BINARY)
{
    EXPECT_TRUE(test_code_generation(lang_csharp, idl_thrift, format_binary));
}

TEST(TEST_THRIFT_HELPER, CSHARP_JSON)
{
    EXPECT_TRUE(test_code_generation(lang_csharp, idl_thrift, format_json));
}

#endif
*/

GTEST_API_ int main(int argc, char **argv)
{
    if (!prepare())
    {
        return 1;
    }

    const char* args[] = { "dsn.idl.test", "--gtest_filter=thrift_helper.*" };
    int args_count = static_cast<int>(sizeof(args) / sizeof(const char*));
    ::testing::InitGoogleTest(&args_count, (char**)&args[0]);

    return RUN_ALL_TESTS();
}
