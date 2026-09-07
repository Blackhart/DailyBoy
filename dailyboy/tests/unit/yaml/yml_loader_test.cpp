#include "yaml/yml_loader.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>

#include "error/yaml.hpp"
#include "status.hpp"

namespace {

std::filesystem::path test_data(const std::string& name) {
  return std::filesystem::path(DAILYBOY_TEST_DATA_DIR) / name;
}

void expect_user_error(std::string_view code, const dailyboy::Status& status) {
  ASSERT_FALSE(status.ok()) << "expected user error " << code;
  EXPECT_EQ(status.code(), dailyboy::Status::Code::kUser);
  const std::string_view msg = status.message();
  if (code == dailyboy::USER_ERROR_3) {
    EXPECT_EQ(
        msg.compare(0, dailyboy::USER_ERROR_3.size(), dailyboy::USER_ERROR_3),
        0)
        << "got: " << msg;
  } else {
    EXPECT_EQ(msg, code) << "got: " << msg;
  }
}

void expect_internal_error(std::string_view code,
                           const dailyboy::Status& status) {
  ASSERT_FALSE(status.ok()) << "expected internal error " << code;
  EXPECT_EQ(status.code(), dailyboy::Status::Code::kInternal);
  EXPECT_EQ(std::string_view(status.message()), code)
      << "got: " << status.message();
}

class ScopedReadPermission {
 public:
  explicit ScopedReadPermission(std::filesystem::path path)
      : path_(std::move(path)) {
    std::error_code ec;
    old_perms_ = std::filesystem::status(path_, ec).permissions();
    if (ec) {
      ADD_FAILURE() << "ScopedReadPermission: status: " << ec.message();
      path_.clear();
      return;
    }
    std::filesystem::permissions(path_, std::filesystem::perms::none,
                                 std::filesystem::perm_options::replace, ec);
    if (ec) {
      ADD_FAILURE() << "ScopedReadPermission: chmod: " << ec.message();
      path_.clear();
    }
  }

  ~ScopedReadPermission() {
    if (path_.empty()) {
      return;
    }
    std::error_code ec;
    std::filesystem::permissions(path_, old_perms_,
                                 std::filesystem::perm_options::replace, ec);
  }

  ScopedReadPermission(const ScopedReadPermission&) = delete;
  ScopedReadPermission& operator=(const ScopedReadPermission&) = delete;

 private:
  std::filesystem::path path_;
  std::filesystem::perms old_perms_{};
};

}  // namespace

/*!
 * \brief Loads a path that does not exist and returns user error 1.
 */
TEST(YmlLoader, Load_MissingFile_ReturnsUserError1) {
  // Prepare
  dailyboy::YmlLoader loader;
  const auto path = test_data("does_not_exist.yaml");

  // Test
  const dailyboy::Status status = loader.load(path);

  // Assert
  expect_user_error(dailyboy::USER_ERROR_1, status);
}

/*!
 * \brief Loads an empty path and returns internal error 1.
 */
TEST(YmlLoader, Load_EmptyPath_ReturnsInternalError1) {
  // Prepare
  dailyboy::YmlLoader loader;

  // Test
  const dailyboy::Status status = loader.load(std::filesystem::path{});

  // Assert
  expect_internal_error(dailyboy::INTERNAL_ERROR_1, status);
}

/*!
 * \brief Loads a directory path and returns user error 1.
 */
TEST(YmlLoader, Load_PathIsDirectory_ReturnsUserError1) {
  // Prepare
  dailyboy::YmlLoader loader;

  // Test
  const dailyboy::Status status =
      loader.load(std::filesystem::path(DAILYBOY_TEST_DATA_DIR));

  // Assert
  expect_user_error(dailyboy::USER_ERROR_1, status);
}

/*!
 * \brief Loads an unreadable file and returns user error 2, then succeeds after
 *        permissions are restored.
 */
TEST(YmlLoader, Load_NoReadPermission_ReturnsUserError2) {
  // Prepare
  const auto path =
      std::filesystem::path(DAILYBOY_TEST_BINARY_DIR) / "unreadable.yaml";
  {
    std::ofstream out(path);
    ASSERT_TRUE(out.is_open());
    out << "key: value\n";
  }
  dailyboy::YmlLoader loader;

  // Test
  {
    ScopedReadPermission deny(path);
    expect_user_error(dailyboy::USER_ERROR_2, loader.load(path));
  }
  const dailyboy::Status status = loader.load(path);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  EXPECT_TRUE(loader.loaded());
  EXPECT_TRUE(loader.root()["key"].IsDefined());
}

/*!
 * \brief Loads malformed YAML and returns user error 3.
 */
TEST(YmlLoader, Load_MalformedYaml_ReturnsUserError3) {
  // Prepare
  dailyboy::YmlLoader loader;

  // Test
  const dailyboy::Status status = loader.load(
      test_data("test__load__malformed_yaml__throws_user_error_3.yaml"));

  // Assert
  expect_user_error(dailyboy::USER_ERROR_3, status);
}

/*!
 * \brief Loads an empty document and returns user error 4.
 */
TEST(YmlLoader, Load_EmptyDocument_ReturnsUserError4) {
  // Prepare
  dailyboy::YmlLoader loader;

  // Test
  const dailyboy::Status status = loader.load(
      test_data("test__load__empty_document__throws_user_error_4.yaml"));

  // Assert
  expect_user_error(dailyboy::USER_ERROR_4, status);
}

/*!
 * \brief Loads valid YAML and populates the root node and source paths.
 */
TEST(YmlLoader, Load_ValidYaml_PopulatesRootNode) {
  // Prepare
  dailyboy::YmlLoader loader;

  // Test
  const dailyboy::Status status =
      loader.load(test_data("test__load__valid_yaml__loads_root_node.yaml"));

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  EXPECT_TRUE(loader.loaded());
  EXPECT_TRUE(loader.root().IsMap());
  EXPECT_EQ(loader.root()["dailyboy_version"].as<int>(), 1);
  EXPECT_EQ(loader.source_path().filename(),
            "test__load__valid_yaml__loads_root_node.yaml");
  EXPECT_EQ(loader.source_directory().filename(), "data");
}
