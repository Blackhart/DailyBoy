/*!
 * \file ffmpeg.cpp
 * \brief Shared FFmpeg helpers for MOV video writers.
 */

#include "video/ffmpeg.hpp"

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
#include <libavutil/error.h>
#include <libavutil/frame.h>
#include <libavutil/log.h>
#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>
}

#include <algorithm>
#include <cstdarg>
#include <cstdint>
#include <dailyboy/log.hpp>
#include <exception>
#include <mutex>
#include <string>
#include <vector>

#include "error/video.hpp"
#include "status.hpp"

namespace dailyboy {

namespace {

std::mutex g_ffmpeg_log_mu;
std::string* g_ffmpeg_log_sink = nullptr;

void capture_av_log(void* ptr, int level, const char* fmt, va_list vl) {
  if (level > AV_LOG_ERROR) {
    return;
  }
  char line[1024];
  int print_prefix = 1;
  av_log_format_line(ptr, level, fmt, vl, line, sizeof(line), &print_prefix);
  std::lock_guard<std::mutex> lock(g_ffmpeg_log_mu);
  if (g_ffmpeg_log_sink == nullptr) {
    return;
  }
  *g_ffmpeg_log_sink += line;
}

}  // namespace

void silence_ffmpeg_logs() {
  static std::once_flag once;
  std::call_once(once, [] { av_log_set_level(AV_LOG_QUIET); });
}

FfmpegLogCapture::FfmpegLogCapture() {
  std::lock_guard<std::mutex> lock(g_ffmpeg_log_mu);
  buffer_.clear();
  g_ffmpeg_log_sink = &buffer_;
  av_log_set_callback(capture_av_log);
  av_log_set_level(AV_LOG_ERROR);
}

FfmpegLogCapture::~FfmpegLogCapture() {
  std::lock_guard<std::mutex> lock(g_ffmpeg_log_mu);
  g_ffmpeg_log_sink = nullptr;
  av_log_set_callback(av_log_default_callback);
  av_log_set_level(AV_LOG_QUIET);
}

std::string av_error_string(int err) {
  char buf[AV_ERROR_MAX_STRING_SIZE];
  av_strerror(err, buf, sizeof(buf));
  return buf;
}

Status ffmpeg_error(const std::string& detail) {
  std::string msg = std::string(USER_ERROR_ENCODE_2) + " " + detail;
  std::lock_guard<std::mutex> lock(g_ffmpeg_log_mu);
  if (g_ffmpeg_log_sink != nullptr && !g_ffmpeg_log_sink->empty()) {
    msg += " ";
    msg += *g_ffmpeg_log_sink;
  }
  return Status::User(msg);
}

Status send_packet_loop(AVFormatContext* format, AVCodecContext* codec,
                        AVStream* stream, AVFrame* frame) {
  FfmpegLogCapture capture;
  int err = avcodec_send_frame(codec, frame);
  if (err < 0) {
    return ffmpeg_error("avcodec_send_frame: " + av_error_string(err));
  }
  AVPacket* pkt = av_packet_alloc();
  if (pkt == nullptr) {
    return ffmpeg_error("av_packet_alloc failed.");
  }
  while (true) {
    err = avcodec_receive_packet(codec, pkt);
    if (err == AVERROR(EAGAIN) || err == AVERROR_EOF) {
      break;
    }
    if (err < 0) {
      av_packet_free(&pkt);
      return ffmpeg_error("avcodec_receive_packet: " + av_error_string(err));
    }
    pkt->stream_index = stream->index;
    av_packet_rescale_ts(pkt, codec->time_base, stream->time_base);
    err = av_interleaved_write_frame(format, pkt);
    av_packet_unref(pkt);
    if (err < 0) {
      av_packet_free(&pkt);
      return ffmpeg_error("av_interleaved_write_frame: " +
                          av_error_string(err));
    }
  }
  av_packet_free(&pkt);
  return Status::Ok();
}

Status extract_rgb8(const Frame& frame, std::vector<uint8_t>& rgb) {
  const OIIO::ImageBuf& src = frame.buf();
  const int width = frame.width();
  const int height = frame.height();
  if (width <= 0 || height <= 0) {
    return Status::User(std::string(USER_ERROR_ENCODE_4));
  }

  int order[3] = {0, 1, 2};
  float fill[3] = {0.0f, 0.0f, 0.0f};
  const int n = src.spec().nchannels;
  if (n < 2) {
    order[1] = 0;
    order[2] = 0;
  } else if (n < 3) {
    order[2] = -1;
  }

  OIIO::ImageBuf rgb_buf;
  try {
    if (!OIIO::ImageBufAlgo::channels(rgb_buf, src, 3, order, fill)) {
      return Status::User(std::string(USER_ERROR_ENCODE_4) + " " +
                          rgb_buf.geterror());
    }
    rgb.resize(static_cast<std::size_t>(width) *
               static_cast<std::size_t>(height) * 3);
    if (!rgb_buf.get_pixels(OIIO::ROI(0, width, 0, height),
                            OIIO::TypeDesc::UINT8, rgb.data())) {
      return Status::User(std::string(USER_ERROR_ENCODE_4) + " " +
                          rgb_buf.geterror());
    }
  } catch (const std::exception& ex) {
    return Status::User(std::string(USER_ERROR_ENCODE_4) + " " + ex.what());
  }
  return Status::Ok();
}

const uint8_t* rgb8_with_encode_pad(const std::vector<uint8_t>& rgb, int width,
                                    int height, int encode_width,
                                    int encode_height,
                                    std::vector<uint8_t>& padded,
                                    int* dst_stride) {
  *dst_stride = encode_width * 3;
  if (encode_width == width && encode_height == height) {
    return rgb.data();
  }
  const int src_stride = width * 3;
  padded.assign(static_cast<std::size_t>(*dst_stride) *
                    static_cast<std::size_t>(encode_height),
                0);
  for (int y = 0; y < height; ++y) {
    std::copy(
        rgb.begin() + static_cast<std::ptrdiff_t>(y) * src_stride,
        rgb.begin() + static_cast<std::ptrdiff_t>(y) * src_stride + src_stride,
        padded.begin() + static_cast<std::ptrdiff_t>(y) * *dst_stride);
  }
  return padded.data();
}

void apply_video_signal(AVCodecContext* codec,
                        const JobOutputVideoSignal& signal) {
  codec->color_primaries = AVCOL_PRI_BT709;
  codec->color_trc = AVCOL_TRC_BT709;
  codec->colorspace = AVCOL_SPC_BT709;
  codec->color_range = signal.range() == JobOutputVideoSignal::RangeValue::Pc
                           ? AVCOL_RANGE_JPEG
                           : AVCOL_RANGE_MPEG;
  log_debug(
      std::string("encode: signal range ") +
      (signal.range() == JobOutputVideoSignal::RangeValue::Pc ? "pc" : "tv") +
      " matrix/primaries/transfer bt709");
}

void apply_video_signal(AVFrame* frame, const JobOutputVideoSignal& signal) {
  frame->color_primaries = AVCOL_PRI_BT709;
  frame->color_trc = AVCOL_TRC_BT709;
  frame->colorspace = AVCOL_SPC_BT709;
  frame->color_range = signal.range() == JobOutputVideoSignal::RangeValue::Pc
                           ? AVCOL_RANGE_JPEG
                           : AVCOL_RANGE_MPEG;
}

Status apply_sws_video_signal(SwsContext* sws,
                              const JobOutputVideoSignal& signal) {
  if (sws == nullptr) {
    return ffmpeg_error("sws context is null.");
  }
  const int* coeffs = sws_getCoefficients(SWS_CS_ITU709);
  const int src_range =
      signal.range() == JobOutputVideoSignal::RangeValue::Pc ? 1 : 0;
  const int dst_range = src_range;
  if (sws_setColorspaceDetails(sws, coeffs, src_range, coeffs, dst_range, 0,
                               1 << 16, 1 << 16) < 0) {
    return ffmpeg_error("sws_setColorspaceDetails failed.");
  }
  return Status::Ok();
}

void apply_global_header(AVCodecContext* codec, const AVFormatContext* format) {
  if (format->oformat->flags & AVFMT_GLOBALHEADER) {
    codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
  }
}

void apply_faststart(AVDictionary** mux_opts, bool faststart) {
  log_debug(std::string("encode: faststart ") + (faststart ? "true" : "false"));
  if (faststart) {
    av_dict_set(mux_opts, "movflags", "faststart", 0);
  }
}

}  // namespace dailyboy
