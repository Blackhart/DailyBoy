#pragma once

#include <string_view>

namespace dailyboy {

/*! \defgroup yml_loader_errors YmlLoader I/O and parse errors
 *  Shared by YAML loaders.
 *  @{
 */

/*!
 * \var INTERNAL_ERROR_1
 * \brief Message when load() is called with an empty path.
 */
inline constexpr std::string_view INTERNAL_ERROR_1 =
    "Internal error: YAML path is empty.";

/*!
 * \var USER_ERROR_1
 * \brief Message when the path is missing, a directory, or not a regular file.
 */
inline constexpr std::string_view USER_ERROR_1 =
    "YAML path must point to an existing regular file (.yaml / .yml).";

/*!
 * \var USER_ERROR_2
 * \brief Message when the file exists but cannot be opened for reading.
 */
inline constexpr std::string_view USER_ERROR_2 =
    "YAML file cannot be read: permission denied.";

/*!
 * \var USER_ERROR_3
 * \brief Message when the file is not valid YAML (details may be appended).
 */
inline constexpr std::string_view USER_ERROR_3 = "YAML file is not valid.";

/*!
 * \var USER_ERROR_4
 * \brief Message when the file parses to an empty document.
 */
inline constexpr std::string_view USER_ERROR_4 =
    "YAML file contains no content.";

/*! @} */

// Job views: add USER_ERROR_<n> / INTERNAL_ERROR_<n>
// for contract validation when those types are introduced.

}  // namespace dailyboy
