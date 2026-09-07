#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>

#include "image/frame.hpp"
#include "job/output.hpp"
#include "status.hpp"
#include "video/video_writer.hpp"

namespace dailyboy {

/*!
 * \brief Writes RGB frames to a QuickTime MOV as DNxHD or DNxHR.
 */
class DnxhdWriter : public VideoWriter {
 public:
  DnxhdWriter();
  ~DnxhdWriter() override;

  DnxhdWriter(const DnxhdWriter&) = delete;
  DnxhdWriter& operator=(const DnxhdWriter&) = delete;

  Status open(const std::filesystem::path& path, int width, int height, int fps,
              const JobOutputVideo& video,
              std::shared_ptr<const AudioPcmTimeline> audio = nullptr) override;
  Status write(const Frame& frame) override;
  Status close() override;

 private:
  struct AvState;
  static void reset_av(AvState& av);

  bool opened_ = false;
  int width_ = 0;
  int height_ = 0;
  int encode_width_ = 0;
  int encode_height_ = 0;
  int64_t pts_ = 0;
  bool interlaced_ = false;
  std::unique_ptr<AvState> av_;
  std::unique_ptr<class MovAudioTrack> audio_;
};

}  // namespace dailyboy
