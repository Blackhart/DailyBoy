#pragma once

#include <cstdint>
#include <memory>

#include "status.hpp"
#include "video/audio_timeline.hpp"

extern "C" {
struct AVCodec;
struct AVCodecContext;
struct AVFormatContext;
struct AVFrame;
struct AVStream;
}

namespace dailyboy {

/*!
 * \brief AAC-LC guide track muxed into an open MOV \c AVFormatContext.
 *
 * Call \c open_aac_stream_on_mov before \c avformat_write_header, then
 * \c write_audio_up_to_video_pts after each video frame, and
 * \c flush_remaining_audio before the trailer.
 */
class MovAudioTrack {
 public:
  static constexpr int kAacBitrate = 192000;

  MovAudioTrack() = default;
  ~MovAudioTrack();

  MovAudioTrack(const MovAudioTrack&) = delete;
  MovAudioTrack& operator=(const MovAudioTrack&) = delete;

  /*!
   * \brief Adds an AAC stream on \a format using \a timeline PCM.
   */
  Status open_aac_stream_on_mov(
      AVFormatContext* format, int fps,
      std::shared_ptr<const AudioPcmTimeline> timeline);

  /*!
   * \brief Encodes and muxes audio covering video pts up to \a video_pts.
   */
  Status write_audio_up_to_video_pts(int64_t video_pts);

  /*!
   * \brief Encodes any remaining PCM and flushes the AAC encoder.
   */
  Status flush_remaining_audio();

  bool active() const { return codec_ != nullptr; }

 private:
  Status finish_open_aac_stream(
      AVFormatContext* format, int fps, const AVCodec* codec,
      std::shared_ptr<const AudioPcmTimeline> timeline);
  Status encode_pcm_chunk(int sample_count);
  void fill_planar_float_from_pcm(AVFrame* frame, int sample_count);
  Status send_audio_frame(AVFrame* frame);
  void free_encoder();

  AVFormatContext* format_ = nullptr;
  AVStream* stream_ = nullptr;
  AVCodecContext* codec_ = nullptr;
  std::shared_ptr<const AudioPcmTimeline> timeline_;
  int fps_ = 24;
  int64_t next_sample_ = 0;
  int64_t next_pts_ = 0;
};

}  // namespace dailyboy
