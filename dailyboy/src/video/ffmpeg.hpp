#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "image/frame.hpp"
#include "job/output.hpp"
#include "status.hpp"

extern "C" {
struct AVCodecContext;
struct AVDictionary;
struct AVFormatContext;
struct AVFrame;
struct AVStream;
struct SwsContext;
}

namespace dailyboy {

/*!
 * \brief Drops FFmpeg stderr chatter (encoder stats, swscaler, faststart).
 *
 * Encode failures still return \c Status. Capture \c av_log with
 * \c FfmpegLogCapture so \c ffmpeg_error can append the encoder text.
 */
void silence_ffmpeg_logs();

/*!
 * \brief Collects FFmpeg \c av_log ERROR lines until destroyed.
 *
 * Restores quiet logging in the destructor. Do not nest captures.
 */
class FfmpegLogCapture {
 public:
  FfmpegLogCapture();
  ~FfmpegLogCapture();

  FfmpegLogCapture(const FfmpegLogCapture&) = delete;
  FfmpegLogCapture& operator=(const FfmpegLogCapture&) = delete;

 private:
  std::string buffer_;
};

/*!
 * \brief FFmpeg \c av_strerror text for \a err.
 */
std::string av_error_string(int err);

/*!
 * \brief User status wrapping \c USER_ERROR_ENCODE_2 and \a detail.
 *
 * Appends captured \c av_log text when a \c FfmpegLogCapture is active.
 */
Status ffmpeg_error(const std::string& detail);

/*!
 * \brief Sends \a frame (or flush if null) and muxes received packets.
 */
Status send_packet_loop(AVFormatContext* format, AVCodecContext* codec,
                        AVStream* stream, AVFrame* frame);

/*!
 * \brief Extracts packed RGB8 from \a frame into \a rgb.
 */
Status extract_rgb8(const Frame& frame, std::vector<uint8_t>& rgb);

/*!
 * \brief Copies RGB8 into \a padded when the encoder size is larger.
 *
 * \return Pointer to \a rgb or \a padded. \a dst_stride is encode width × 3.
 */
const uint8_t* rgb8_with_encode_pad(const std::vector<uint8_t>& rgb, int width,
                                    int height, int encode_width,
                                    int encode_height,
                                    std::vector<uint8_t>& padded,
                                    int* dst_stride);

/*!
 * \brief Sets color primaries, transfer, matrix, and range from \a signal.
 */
void apply_video_signal(AVCodecContext* codec,
                        const JobOutputVideoSignal& signal);

/*!
 * \brief Sets color primaries, transfer, matrix, and range on a YUV frame.
 */
void apply_video_signal(AVFrame* frame, const JobOutputVideoSignal& signal);

/*!
 * \brief Configures swscale RGB→YUV colorspace details from \a signal.
 */
Status apply_sws_video_signal(SwsContext* sws,
                              const JobOutputVideoSignal& signal);

/*!
 * \brief Sets \c AV_CODEC_FLAG_GLOBAL_HEADER when the MOV muxer needs it.
 */
void apply_global_header(AVCodecContext* codec, const AVFormatContext* format);

/*!
 * \brief Adds \c movflags=faststart to \a mux_opts when \a faststart is true.
 */
void apply_faststart(AVDictionary** mux_opts, bool faststart);

}  // namespace dailyboy
