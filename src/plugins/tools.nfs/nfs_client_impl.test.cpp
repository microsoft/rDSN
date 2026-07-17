#include "nfs_client_impl.h"

#include <fstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <dsn/cpp/test_utils.h>

namespace dsn {
namespace service {
namespace {

error_code copy_remote_files(const rpc_address &remote,
                             const std::string &source_dir,
                             const std::vector<std::string> &files,
                             const std::string &destination_dir,
                             bool overwrite)
{
    error_code result = ERR_INVALID_STATE;
    auto task = file::copy_remote_files(remote,
                                        source_dir,
                                        files,
                                        destination_dir,
                                        overwrite,
                                        LPC_AIO_TEST_NFS,
                                        nullptr,
                                        [&result](error_code err, size_t) { result = err; });
    if (task == nullptr) {
        return ERR_INVALID_STATE;
    }
    if (!task->wait(20000)) {
        task->cancel(true);
        return ERR_TIMEOUT;
    }
    return result;
}

error_code copy_remote_directory(const rpc_address &remote,
                                 const std::string &source_dir,
                                 const std::string &destination_dir)
{
    error_code result = ERR_INVALID_STATE;
    auto task = file::copy_remote_directory(remote,
                                            source_dir,
                                            destination_dir,
                                            false,
                                            LPC_AIO_TEST_NFS,
                                            nullptr,
                                            [&result](error_code err, size_t) { result = err; });
    if (task == nullptr) {
        return ERR_INVALID_STATE;
    }
    if (!task->wait(20000)) {
        task->cancel(true);
        return ERR_TIMEOUT;
    }
    return result;
}

void write_file(const std::string &path, const std::string &contents)
{
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    ASSERT_TRUE(stream.is_open());
    stream.write(contents.data(), contents.size());
    ASSERT_TRUE(stream.good());
}

std::string read_file(const std::string &path)
{
    std::ifstream stream(path, std::ios::binary);
    EXPECT_TRUE(stream.is_open());
    return std::string((std::istreambuf_iterator<char>(stream)),
                       std::istreambuf_iterator<char>());
}

TEST(tools_nfs, copy_helpers_validate_configuration_and_write_results)
{
    using namespace nfs_client_detail;

    EXPECT_EQ(default_copy_block_bytes, normalize_copy_block_bytes(0));
    EXPECT_EQ(default_copy_block_bytes,
              normalize_copy_block_bytes(
                  static_cast<uint64_t>((std::numeric_limits<int32_t>::max)()) + 1));
    EXPECT_EQ(static_cast<uint32_t>((std::numeric_limits<int32_t>::max)()),
              normalize_copy_block_bytes((std::numeric_limits<int32_t>::max)()));
    EXPECT_EQ(4096u, normalize_copy_block_bytes(4096));

    const int preserve_flags = destination_open_flags(false);
    EXPECT_NE(0, preserve_flags & O_EXCL);
    EXPECT_EQ(0, preserve_flags & O_TRUNC);

    const int overwrite_flags = destination_open_flags(true);
    EXPECT_NE(0, overwrite_flags & O_TRUNC);
    EXPECT_EQ(0, overwrite_flags & O_EXCL);

    EXPECT_EQ(ERR_OK, local_write_result(ERR_OK, 64, 64));
    EXPECT_EQ(ERR_FILE_OPERATION_FAILED, local_write_result(ERR_OK, 63, 64));
    EXPECT_EQ(ERR_TIMEOUT, local_write_result(ERR_TIMEOUT, 63, 64));
}

TEST(tools_nfs, empty_directory_copy_completes)
{
    ASSERT_TRUE(utils::test::prepare_test_tmp_dir("dsn.tools.nfs.empty_directory"));
    const std::string source =
        utils::test::test_tmp_path("dsn.tools.nfs.empty_directory", "source");
    const std::string destination =
        utils::test::test_tmp_path("dsn.tools.nfs.empty_directory", "destination");
    ASSERT_TRUE(utils::filesystem::create_directory(source));

    EXPECT_EQ(ERR_OK,
              copy_remote_directory(rpc_address("127.0.0.1", 20101), source, destination));
}

TEST(tools_nfs, overwrite_policy_preserves_or_truncates_existing_file)
{
    ASSERT_TRUE(utils::test::prepare_test_tmp_dir("dsn.tools.nfs.overwrite"));
    const std::string source =
        utils::test::test_tmp_path("dsn.tools.nfs.overwrite", "source");
    const std::string destination =
        utils::test::test_tmp_path("dsn.tools.nfs.overwrite", "destination");
    ASSERT_TRUE(utils::filesystem::create_directory(source));
    ASSERT_TRUE(utils::filesystem::create_directory(destination));

    const std::string file_name = "data";
    write_file(utils::filesystem::path_combine(source, file_name), "new");
    write_file(utils::filesystem::path_combine(destination, file_name), "old-content");

    const rpc_address remote("127.0.0.1", 20101);
    EXPECT_EQ(ERR_FILE_OPERATION_FAILED,
              copy_remote_files(remote, source, {file_name}, destination, false));
    EXPECT_EQ("old-content",
              read_file(utils::filesystem::path_combine(destination, file_name)));

    EXPECT_EQ(ERR_OK, copy_remote_files(remote, source, {file_name}, destination, true));
    EXPECT_EQ("new", read_file(utils::filesystem::path_combine(destination, file_name)));
}

} // anonymous namespace
} // namespace service
} // namespace dsn
