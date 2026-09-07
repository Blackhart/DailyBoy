#pragma once

#include <yaml-cpp/yaml.h>

#include <filesystem>

#include "status.hpp"

namespace dailyboy {

/*!
 * \brief Reads a YAML file into a document tree after path and access checks.
 *
 * Call \c load() before \c root(), \c source_path(), or \c source_directory().
 */
class YmlLoader {
 public:
  YmlLoader() = default;

  /*!
   * \brief Loads and parses a YAML file from disk.
   * \param path Path to a `.yaml` or `.yml` file (absolute or relative).
   * \return \c Status::Internal \c INTERNAL_ERROR_1 if \a path is empty;
   *         \c Status::User \c USER_ERROR_1 if \a path is not an existing
   *         regular file; \c USER_ERROR_2 if unreadable; \c USER_ERROR_3 if
   *         not valid YAML; \c USER_ERROR_4 if the file contains no YAML.
   */
  Status load(const std::filesystem::path& path);

  [[nodiscard]] bool loaded() const { return loaded_; }

  /*!
   * \brief Returns the parsed YAML document root.
   * \note Only valid after a successful load().
   */
  [[nodiscard]] const YAML::Node& root() const { return root_; }

  /*!
   * \brief Returns the directory containing the loaded file.
   * \note Only meaningful after a successful load(). Used to resolve relative
   *       paths in the job.
   */
  [[nodiscard]] const std::filesystem::path& source_directory() const {
    return source_directory_;
  }

  /*!
   * \brief Returns the resolved absolute path of the loaded file.
   * \note Only meaningful after a successful load().
   */
  [[nodiscard]] const std::filesystem::path& source_path() const {
    return source_path_;
  }

 protected:
  YAML::Node root_;
  bool loaded_ = false;
  std::filesystem::path source_path_;
  std::filesystem::path source_directory_;
};

}  // namespace dailyboy
