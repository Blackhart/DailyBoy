#pragma once

#include <string_view>

namespace dailyboy {

/*! \defgroup encode_errors Video writer errors
 *  @{
 */

/*!
 * \var USER_ERROR_ENCODE_1
 * \brief Message when the output path is empty.
 */
inline constexpr std::string_view USER_ERROR_ENCODE_1 =
    "encode: output path is empty.";

/*!
 * \var USER_ERROR_ENCODE_2
 * \brief Message when FFmpeg cannot create or write the MOV (details appended).
 */
inline constexpr std::string_view USER_ERROR_ENCODE_2 =
    "encode: failed to write video MOV.";

/*!
 * \var USER_ERROR_ENCODE_3
 * \brief Message when a frame's size does not match the writer dimensions.
 */
inline constexpr std::string_view USER_ERROR_ENCODE_3 =
    "encode: frame size does not match the opened video.";

/*!
 * \var USER_ERROR_ENCODE_4
 * \brief Message when pixels cannot be extracted from the ImageBuf.
 */
inline constexpr std::string_view USER_ERROR_ENCODE_4 =
    "encode: failed to read RGB pixels from the frame.";

/*!
 * \var USER_ERROR_ENCODE_5
 * \brief Message when guide-track audio cannot be demuxed or decoded.
 */
inline constexpr std::string_view USER_ERROR_ENCODE_5 =
    "encode: failed to read guide-track audio.";

/*!
 * \var USER_ERROR_ENCODE_6
 * \brief Message when AAC mux into the MOV fails.
 */
inline constexpr std::string_view USER_ERROR_ENCODE_6 =
    "encode: failed to write guide-track audio into the MOV.";

/*!
 * \var INTERNAL_ERROR_ENCODE_1
 * \brief Message when write/close is called before a successful open.
 */
inline constexpr std::string_view INTERNAL_ERROR_ENCODE_1 =
    "Internal error: video writer is not open.";

/*!
 * \var INTERNAL_ERROR_ENCODE_2
 * \brief Message when a writer is opened with codec_options for another codec.
 */
inline constexpr std::string_view INTERNAL_ERROR_ENCODE_2 =
    "Internal error: codec_options do not match the video writer.";

/*! @} */

}  // namespace dailyboy
