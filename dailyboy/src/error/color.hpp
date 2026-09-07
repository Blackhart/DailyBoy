#pragma once

#include <string_view>

namespace dailyboy {

/*! \defgroup color_errors OpenColorIO transform errors
 *  @{
 */

/*!
 * \var USER_ERROR_COLOR_1
 * \brief Message when the OCIO config path cannot be loaded.
 */
inline constexpr std::string_view USER_ERROR_COLOR_1 =
    "color: failed to load OCIO config.";

/*!
 * \var USER_ERROR_COLOR_2
 * \brief Message when a colorspace transform cannot be built.
 */
inline constexpr std::string_view USER_ERROR_COLOR_2 =
    "color: failed to create OCIO processor.";

/*!
 * \var USER_ERROR_COLOR_3
 * \brief Message when applying an OCIO transform fails.
 */
inline constexpr std::string_view USER_ERROR_COLOR_3 =
    "color: failed to apply OCIO transform.";

/*!
 * \var USER_ERROR_COLOR_4
 * \brief Message when a frame-map substitution is used in a color string.
 */
inline constexpr std::string_view USER_ERROR_COLOR_4 =
    "color: frame-map substitutions are not allowed in colorspace or context "
    "strings.";

/*! @} */

}  // namespace dailyboy
