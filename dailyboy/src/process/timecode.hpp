#pragma once

#include <cstdint>
#include <string>

#include "job/job.hpp"
#include "status.hpp"

namespace dailyboy {

/*!
 * \brief Shared fps for SMPTE when \c plans[].timecode is set.
 *
 * Requires all enabled \c output.videos[] to share one fps. With no enabled
 * video, returns 24. With no plan timecode, returns 24 without checking fps.
 */
StatusOr<int> resolve_timecode_fps(const Job& job);

/*!
 * \brief Absolute frame count for \c plans[0].timecode.start at \a fps.
 */
StatusOr<std::int64_t> resolve_timecode_start_frames(const Job& job, int fps);

/*!
 * \brief Formats SMPTE for source \a frame (hero-anchored).
 *
 * Empty string when the plan has no \c timecode block.
 */
StatusOr<std::string> format_overlay_timecode(const Job& job, int frame);

/*!
 * \brief MOV metadata timecode for the first media sample (slate + handles).
 *
 * Empty string when the plan has no \c timecode block.
 */
StatusOr<std::string> format_mov_timecode(const Job& job);

/*!
 * \brief Converts an absolute frame count to an SMPTE string.
 */
StatusOr<std::string> frames_to_smpte(std::int64_t frames, int fps,
                                      bool drop_frame);

/*!
 * \brief Parses an SMPTE string to an absolute frame count.
 */
StatusOr<std::int64_t> smpte_to_frames(const std::string& text, int fps,
                                       bool drop_frame);

}  // namespace dailyboy
