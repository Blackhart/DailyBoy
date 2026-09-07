/*!
 * \file yml_loader.cpp
 * \brief YAML path checks and parse (yaml-cpp exceptions become Status).
 */

#include "yaml/yml_loader.hpp"

#include <filesystem>
#include <fstream>
#include <string>

#include "error/yaml.hpp"
#include "status.hpp"

namespace dailyboy {

namespace {

/*!
 * \brief Returns a weakly canonical path, or \a path unchanged if that fails.
 */
[[nodiscard]] std::filesystem::path resolve_path(
    const std::filesystem::path& path) {
  std::error_code ec;
  const auto canonical = std::filesystem::weakly_canonical(path, ec);
  return ec ? path : canonical;
}

/*!
 * \brief Returns USER_ERROR_1 unless \a resolved is an existing regular file.
 */
Status ensure_regular_file(const std::filesystem::path& resolved) {
  std::error_code ec;
  if (!std::filesystem::exists(resolved, ec) || ec) {
    return Status::User(std::string(USER_ERROR_1));
  }
  if (std::filesystem::is_directory(resolved, ec) || ec) {
    return Status::User(std::string(USER_ERROR_1));
  }
  if (!std::filesystem::is_regular_file(resolved, ec) || ec) {
    return Status::User(std::string(USER_ERROR_1));
  }
  return Status::Ok();
}

/*!
 * \brief Returns USER_ERROR_2 if the process cannot open \a resolved for reading.
 */
Status ensure_readable(const std::filesystem::path& resolved) {
  std::ifstream in(resolved, std::ios::in | std::ios::binary);
  if (!in.is_open()) {
    return Status::User(std::string(USER_ERROR_2));
  }
  return Status::Ok();
}

}  // namespace

Status YmlLoader::load(const std::filesystem::path& path) {
  loaded_ = false;
  root_ = YAML::Node();
  source_path_.clear();
  source_directory_.clear();

  if (path.empty()) {
    return Status::Internal(std::string(INTERNAL_ERROR_1));
  }

  const std::filesystem::path resolved = resolve_path(path);
  DAILYBOY_RETURN_IF_ERROR(ensure_regular_file(resolved));
  DAILYBOY_RETURN_IF_ERROR(ensure_readable(resolved));

  try {
    root_ = YAML::LoadFile(resolved.string());
  } catch (const YAML::Exception& ex) {
    return Status::User(std::string(USER_ERROR_3) + " (" + ex.what() + ")");
  }

  if (!root_.IsMap() && !root_.IsSequence() && !root_.IsScalar()) {
    return Status::User(std::string(USER_ERROR_4));
  }

  source_path_ = resolved;
  source_directory_ = resolved.parent_path();
  loaded_ = true;
  return Status::Ok();
}

}  // namespace dailyboy
