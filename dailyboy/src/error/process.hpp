#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace dailyboy {

/*! \defgroup process_errors Layout, overlay, and daily orchestration errors
 *  @{
 */

inline constexpr std::string_view USER_ERROR_OVERLAY_1 =
    "layout: min_margin_px leaves no usable image area.";

inline constexpr std::string_view USER_ERROR_OVERLAY_2 =
    "layout: failed to compose frame onto canvas.";

inline constexpr std::string_view USER_ERROR_OVERLAY_3 =
    "layout: burn-in font file is missing.";

inline constexpr std::string_view USER_ERROR_OVERLAY_4 =
    "layout: failed to render burn-in text.";

inline constexpr std::string_view USER_ERROR_RENDER_1 =
    "render: job has no plans.";

inline constexpr std::string_view USER_ERROR_RENDER_2 =
    "render: source frame size changed within the sequence.";

inline constexpr std::string_view USER_ERROR_RENDER_3 =
    "render: job has no enabled video or image sequence output.";

inline std::string video_path_frame_map_error(std::size_t index) {
  return "output.videos[" + std::to_string(index) +
         "].path: frame-map substitutions are not allowed in file paths.";
}

inline std::string sequence_path_frame_map_error(std::size_t index) {
  return "output.image_sequences[" + std::to_string(index) +
         "].path_pattern: frame-map substitutions are not allowed in file "
         "paths.";
}

inline constexpr std::string_view USER_ERROR_RENDER_5 =
    "render: cannot create output directory.";

inline constexpr std::string_view USER_ERROR_RENDER_6 =
    "render: duplicate video output path.";

inline constexpr std::string_view USER_ERROR_RENDER_7 =
    "render: duplicate image sequence output pattern.";

/*! @} */

}  // namespace dailyboy
