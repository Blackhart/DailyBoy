#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace dailyview {

/*!
 * \brief RGB8 frame load outcome delivered from the TBB worker to the GUI.
 */
struct LoadFrameResult {
  uint64_t generation = 0;
  int frame = 0;
  int width = 0;
  int height = 0;
  std::vector<uint8_t> rgb8;
  bool ok = false;
  std::string error_message;
};

/*!
 * \brief Sequence open outcome delivered from the TBB worker to the GUI.
 */
struct OpenSequenceResult {
  int frame_start = 0;
  int frame_end = 0;
  bool ok = false;
  std::string error_message;
};

}  // namespace dailyview
