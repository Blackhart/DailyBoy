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
#include <libavutil/pixdesc.h>
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

// AVFrame docs: swscale may read 16 bytes past planes; prefer 32-byte linesizes.
constexpr int kSwsRgbStrideAlign = 32;
constexpr int kSwsSimdOverread = 64;

int sws_rgb_stride(int encode_width, RgbEncodeDepth depth) {
  const int bytes = encode_width * rgb_bytes_per_pixel(depth);
  return (bytes + kSwsRgbStrideAlign - 1) & ~(kSwsRgbStrideAlign - 1);
}

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

Status extract_rgb_channels(const Frame& frame, OIIO::TypeDesc type,
                            int bytes_per_pixel, std::vector<uint8_t>& rgb) {
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
               static_cast<std::size_t>(height) *
               static_cast<std::size_t>(bytes_per_pixel));
    if (!rgb_buf.get_pixels(OIIO::ROI(0, width, 0, height), type, rgb.data())) {
      return Status::User(std::string(USER_ERROR_ENCODE_4) + " " +
                          rgb_buf.geterror());
    }
  } catch (const std::exception& ex) {
    return Status::User(std::string(USER_ERROR_ENCODE_4) + " " + ex.what());
  }
  return Status::Ok();
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

RgbEncodeDepth rgb_encode_depth_for_pix_fmt(AVPixelFormat pix_fmt) {
  const AVPixFmtDescriptor* desc = av_pix_fmt_desc_get(pix_fmt);
  if (desc != nullptr && desc->nb_components > 0 && desc->comp[0].depth > 8) {
    return RgbEncodeDepth::Bits16;
  }
  return RgbEncodeDepth::Bits8;
}

AVPixelFormat sws_rgb_pix_fmt(RgbEncodeDepth depth) {
  return depth == RgbEncodeDepth::Bits16 ? AV_PIX_FMT_RGB48 : AV_PIX_FMT_RGB24;
}

int rgb_bytes_per_pixel(RgbEncodeDepth depth) {
  return depth == RgbEncodeDepth::Bits16 ? 6 : 3;
}

Status extract_rgb(const Frame& frame, RgbEncodeDepth depth,
                   std::vector<uint8_t>& rgb) {
  const OIIO::TypeDesc type = depth == RgbEncodeDepth::Bits16
                                  ? OIIO::TypeDesc::UINT16
                                  : OIIO::TypeDesc::UINT8;
  return extract_rgb_channels(frame, type, rgb_bytes_per_pixel(depth), rgb);
}

const uint8_t* rgb_with_encode_pad(const std::vector<uint8_t>& rgb, int width,
                                   int height, int encode_width,
                                   int encode_height, RgbEncodeDepth depth,
                                   std::vector<uint8_t>& padded,
                                   int* dst_stride) {
  const int bpp = rgb_bytes_per_pixel(depth);
  *dst_stride = sws_rgb_stride(encode_width, depth);
  const int src_stride = width * bpp;
  padded.assign(static_cast<std::size_t>(*dst_stride) *
                        static_cast<std::size_t>(encode_height) +
                    kSwsSimdOverread,
                0);
  for (int y = 0; y < height; ++y) {
    std::copy(
        rgb.begin() + static_cast<std::ptrdiff_t>(y) * src_stride,
        rgb.begin() + static_cast<std::ptrdiff_t>(y) * src_stride + src_stride,
        padded.begin() + static_cast<std::ptrdiff_t>(y) * *dst_stride);
  }
  return padded.data();
}

void fill_rgb_sws_src(const uint8_t* src, int stride,
                      const uint8_t* (&planes)[AV_NUM_DATA_POINTERS],
                      int (&strides)[AV_NUM_DATA_POINTERS]) {
  for (int i = 0; i < AV_NUM_DATA_POINTERS; ++i) {
    planes[i] = nullptr;
    strides[i] = 0;
  }
  planes[0] = src;
  strides[0] = stride;
}

Status create_rgb_to_yuv_sws(int encode_width, int encode_height,
                             AVPixelFormat dst_pix_fmt,
                             const JobOutputVideoSignal& signal,
                             SwsContext** out_sws) {
  if (out_sws == nullptr) {
    return ffmpeg_error("sws output pointer is null.");
  }
  const RgbEncodeDepth depth = rgb_encode_depth_for_pix_fmt(dst_pix_fmt);
  const AVPixelFormat src_fmt = sws_rgb_pix_fmt(depth);
  *out_sws = sws_getContext(encode_width, encode_height, src_fmt, encode_width,
                            encode_height, dst_pix_fmt, SWS_BILINEAR, nullptr,
                            nullptr, nullptr);
  if (*out_sws == nullptr) {
    return ffmpeg_error("sws_getContext failed.");
  }
  const Status status = apply_sws_video_signal(*out_sws, signal);
  if (!status.ok()) {
    sws_freeContext(*out_sws);
    *out_sws = nullptr;
  }
  return status;
}

Status convert_frame_rgb_to_yuv(const Frame& frame, int width, int height,
                                int encode_width, int encode_height,
                                SwsContext* sws, AVFrame* yuv) {
  if (sws == nullptr || yuv == nullptr) {
    return ffmpeg_error("sws or yuv frame is null.");
  }
  const RgbEncodeDepth depth =
      rgb_encode_depth_for_pix_fmt(static_cast<AVPixelFormat>(yuv->format));
  std::vector<uint8_t> rgb;
  DAILYBOY_RETURN_IF_ERROR(extract_rgb(frame, depth, rgb));
  std::vector<uint8_t> padded;
  int dst_stride = 0;
  const uint8_t* src =
      rgb_with_encode_pad(rgb, width, height, encode_width, encode_height,
                          depth, padded, &dst_stride);
  const uint8_t* planes[AV_NUM_DATA_POINTERS];
  int strides[AV_NUM_DATA_POINTERS];
  fill_rgb_sws_src(src, dst_stride, planes, strides);
  const int err = av_frame_make_writable(yuv);
  if (err < 0) {
    return ffmpeg_error("av_frame_make_writable: " + av_error_string(err));
  }
  const int scaled = sws_scale(sws, planes, strides, 0, encode_height,
                               yuv->data, yuv->linesize);
  if (scaled != encode_height) {
    return ffmpeg_error("sws_scale failed.");
  }
  return Status::Ok();
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

Status set_mov_timecode(AVFormatContext* format,
                        const std::optional<std::string>& timecode) {
  if (!timecode.has_value() || timecode->empty() || format == nullptr) {
    return Status::Ok();
  }
  if (av_dict_set(&format->metadata, "timecode", timecode->c_str(), 0) < 0) {
    return ffmpeg_error("av_dict_set timecode failed.");
  }
  if (format->nb_streams > 0 && format->streams[0] != nullptr) {
    if (av_dict_set(&format->streams[0]->metadata, "timecode",
                    timecode->c_str(), 0) < 0) {
      return ffmpeg_error("av_dict_set stream timecode failed.");
    }
  }
  return Status::Ok();
}

}  // namespace dailyboy
