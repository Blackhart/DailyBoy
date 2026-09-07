#pragma once

#include <string_view>

namespace dailyboy {

/*! \defgroup io_errors Sequence and frame I/O errors
 *  @{
 */

/*!
 * \var USER_ERROR_IO_1
 * \brief Message when \c frame_start is greater than \c frame_end.
 */
inline constexpr std::string_view USER_ERROR_IO_1 =
    "sequence: frame_start must be less than or equal to frame_end.";

/*!
 * \var USER_ERROR_IO_2
 * \brief Message when the sequence path is empty after token expansion.
 */
inline constexpr std::string_view USER_ERROR_IO_2 =
    "sequence: empty file sequence pattern.";

/*!
 * \var USER_ERROR_IO_3
 * \brief Message when libfileseq cannot parse the sequence pattern.
 */
inline constexpr std::string_view USER_ERROR_IO_3 =
    "sequence: invalid file sequence pattern.";

/*!
 * \var USER_ERROR_IO_4
 * \brief Message when a requested frame number is outside the sequence range.
 */
inline constexpr std::string_view USER_ERROR_IO_4 =
    "sequence: frame number is outside frame_start..frame_end.";

/*!
 * \var USER_ERROR_IO_5
 * \brief Message when the file for a frame is missing or not a regular file.
 */
inline constexpr std::string_view USER_ERROR_IO_5 =
    "sequence: source frame file is missing.";

/*!
 * \var USER_ERROR_IO_6
 * \brief Message when a frame-map substitution is used in the sequence path.
 */
inline constexpr std::string_view USER_ERROR_IO_6 =
    "sequence.path: frame-map substitutions are not allowed in file paths.";

/*!
 * \var USER_ERROR_IO_7
 * \brief Message when OpenImageIO cannot read a source frame (details appended).
 */
inline constexpr std::string_view USER_ERROR_IO_7 =
    "sequence: failed to read source frame.";

/*!
 * \var USER_ERROR_IO_8
 * \brief Message when OpenImageIO cannot write an output frame (details
 *        appended).
 */
inline constexpr std::string_view USER_ERROR_IO_8 =
    "sequence: failed to write output frame.";

/*!
 * \var USER_ERROR_IO_9
 * \brief Message when display-window reformat fails (details appended).
 */
inline constexpr std::string_view USER_ERROR_IO_9 =
    "sequence: failed to reformat frame.";

/*! @} */

}  // namespace dailyboy
