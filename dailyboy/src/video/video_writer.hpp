#pragma once

#include <filesystem>
#include <memory>

#include "image/frame.hpp"
#include "job/output.hpp"
#include "status.hpp"
#include "video/audio_timeline.hpp"

namespace dailyboy {

/*!
 * \brief Abstract MOV encoder (one codec implementation per subclass).
 *
 * Construct via \c make_video_writer. \c open allocates the FFmpeg pipeline
 * from \a video codec_options; optional \a audio adds an AAC guide track.
 * \c write encodes one frame; \c close flushes and writes the trailer.
 */
class VideoWriter {
 public:
  static constexpr int kDefaultFps = 24;

  VideoWriter() = default;
  virtual ~VideoWriter() = default;

  VideoWriter(const VideoWriter&) = delete;
  VideoWriter& operator=(const VideoWriter&) = delete;

  /*!
   * \brief Creates \a path and opens video (and optional AAC) streams.
   */
  virtual Status open(
      const std::filesystem::path& path, int width, int height, int fps,
      const JobOutputVideo& video,
      std::shared_ptr<const AudioPcmTimeline> audio = nullptr) = 0;

  /*!
   * \brief Encodes one frame (RGB extracted from the ImageBuf) and muxes it.
   */
  virtual Status write(const Frame& frame) = 0;

  /*!
   * \brief Flushes the encoder and writes the MOV trailer.
   */
  virtual Status close() = 0;
};

/*!
 * \brief Returns an empty writer for \a codec (H.264, MJPEG, or DNxHD).
 */
std::unique_ptr<VideoWriter> make_video_writer(
    JobOutputVideo::JobOutputVideoCodecValue codec);

}  // namespace dailyboy
