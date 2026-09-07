/*!
 * \file mjpeg_writer.cpp
 * \brief FFmpeg MJPEG/MOV encode: RGB ImageBuf → YUV, one frame at a time.
 */

#include "video/mjpeg_writer.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
#include <libavutil/frame.h>
#include <libavutil/mathematics.h>
#include <libavutil/opt.h>
#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>
}

#include <cstdint>
#include <dailyboy/log.hpp>
#include <string>
#include <variant>
#include <vector>

#include "error/video.hpp"
#include "status.hpp"
#include "video/ffmpeg.hpp"
#include "video/mov_audio_track.hpp"

namespace dailyboy {

namespace {

const char* huffman_name(
    JobOutputVideoMjpeg::JobOutputVideoMjpegHuffmanValue v) {
  return v == JobOutputVideoMjpeg::JobOutputVideoMjpegHuffmanValue::Default
             ? "default"
             : "optimal";
}

AVPixelFormat mjpeg_av_pix_fmt(const JobOutputVideoMjpeg& options,
                               const JobOutputVideoSignal& signal) {
  using Pix = JobOutputVideoMjpeg::JobOutputVideoMjpegPixFmtValue;
  const bool full_range =
      signal.range() == JobOutputVideoSignal::RangeValue::Pc;
  switch (options.pix_fmt()) {
    case Pix::Yuv420p:
      return full_range ? AV_PIX_FMT_YUVJ420P : AV_PIX_FMT_YUV420P;
    case Pix::Yuv422p:
      return full_range ? AV_PIX_FMT_YUVJ422P : AV_PIX_FMT_YUV422P;
    case Pix::Yuv444p:
      return full_range ? AV_PIX_FMT_YUVJ444P : AV_PIX_FMT_YUV444P;
  }
  return AV_PIX_FMT_YUVJ422P;
}

void encode_pad(const JobOutputVideoMjpeg& options, int width, int height,
                int* encode_width, int* encode_height) {
  using Pix = JobOutputVideoMjpeg::JobOutputVideoMjpegPixFmtValue;
  *encode_width = width;
  *encode_height = height;
  if (options.pix_fmt() != Pix::Yuv444p && (width % 2) != 0) {
    *encode_width = width + 1;
  }
  if (options.pix_fmt() == Pix::Yuv420p && (height % 2) != 0) {
    *encode_height = height + 1;
  }
}

Status apply_pix_fmt(AVCodecContext* codec, const JobOutputVideoMjpeg& options,
                     const JobOutputVideoSignal& signal) {
  codec->pix_fmt = mjpeg_av_pix_fmt(options, signal);
  using Pix = JobOutputVideoMjpeg::JobOutputVideoMjpegPixFmtValue;
  const char* name = "yuv422p";
  if (options.pix_fmt() == Pix::Yuv420p) {
    name = "yuv420p";
  } else if (options.pix_fmt() == Pix::Yuv444p) {
    name = "yuv444p";
  }
  log_debug(std::string("encode: pix_fmt ") + name);
  return Status::Ok();
}

Status apply_qscale(AVCodecContext* codec, const JobOutputVideoMjpeg& options,
                    int* quality) {
  codec->flags |= AV_CODEC_FLAG_QSCALE;
  *quality = options.qscale() * FF_QP2LAMBDA;
  codec->global_quality = *quality;
  log_debug("encode: qscale " + std::to_string(options.qscale()));
  return Status::Ok();
}

Status apply_huffman(AVCodecContext* codec,
                     const JobOutputVideoMjpeg& options) {
  const char* name = huffman_name(options.huffman());
  log_debug(std::string("encode: huffman ") + name);
  if (av_opt_set(codec->priv_data, "huffman", name, 0) < 0) {
    return ffmpeg_error("failed to set mjpeg huffman.");
  }
  return Status::Ok();
}

Status apply_mjpeg_codec_options(AVCodecContext* codec,
                                 const JobOutputVideoMjpeg& options,
                                 const JobOutputVideoSignal& signal,
                                 int* quality) {
  DAILYBOY_RETURN_IF_ERROR(apply_pix_fmt(codec, options, signal));
  DAILYBOY_RETURN_IF_ERROR(apply_qscale(codec, options, quality));
  DAILYBOY_RETURN_IF_ERROR(apply_huffman(codec, options));
  return Status::Ok();
}

}  // namespace

struct MjpegWriter::AvState {
  AVFormatContext* format = nullptr;
  AVCodecContext* codec = nullptr;
  AVStream* stream = nullptr;
  SwsContext* sws = nullptr;
  AVFrame* yuv = nullptr;
};

void MjpegWriter::reset_av(AvState& av) {
  if (av.yuv != nullptr) {
    av_frame_free(&av.yuv);
  }
  if (av.sws != nullptr) {
    sws_freeContext(av.sws);
    av.sws = nullptr;
  }
  if (av.codec != nullptr) {
    avcodec_free_context(&av.codec);
  }
  if (av.format != nullptr) {
    if (av.format->pb != nullptr &&
        (av.format->oformat->flags & AVFMT_NOFILE) == 0) {
      avio_closep(&av.format->pb);
    }
    avformat_free_context(av.format);
    av.format = nullptr;
  }
}

MjpegWriter::MjpegWriter() = default;

MjpegWriter::~MjpegWriter() { close(); }

Status MjpegWriter::open(const std::filesystem::path& path, int width,
                         int height, int fps, const JobOutputVideo& video,
                         std::shared_ptr<const AudioPcmTimeline> audio) {
  silence_ffmpeg_logs();
  const JobOutputVideoMjpeg* options =
      std::get_if<JobOutputVideoMjpeg>(&video.codec_options());
  if (options == nullptr) {
    return Status::Internal(std::string(INTERNAL_ERROR_ENCODE_2));
  }
  if (opened_) {
    DAILYBOY_RETURN_IF_ERROR(close());
  }
  FfmpegLogCapture ffmpeg_logs;
  if (path.empty()) {
    return Status::User(std::string(USER_ERROR_ENCODE_1));
  }
  if (width <= 0 || height <= 0 || fps <= 0) {
    return ffmpeg_error("invalid width, height, or fps.");
  }

  av_ = std::make_unique<AvState>();
  width_ = width;
  height_ = height;
  encode_pad(*options, width, height, &encode_width_, &encode_height_);
  pts_ = 0;

  auto fail = [this](Status status) -> Status {
    if (!status.ok() && av_) {
      audio_.reset();
      reset_av(*av_);
      av_.reset();
    }
    return status;
  };

  const std::string path_str = path.string();
  int err = avformat_alloc_output_context2(&av_->format, nullptr, "mov",
                                           path_str.c_str());
  if (err < 0 || av_->format == nullptr) {
    return fail(ffmpeg_error("avformat_alloc_output_context2: " +
                             av_error_string(err)));
  }

  const AVCodec* codec = avcodec_find_encoder_by_name("mjpeg");
  if (codec == nullptr) {
    return fail(ffmpeg_error("mjpeg encoder not found."));
  }

  av_->stream = avformat_new_stream(av_->format, nullptr);
  if (av_->stream == nullptr) {
    return fail(ffmpeg_error("avformat_new_stream failed."));
  }
  av_->stream->id = 0;
  av_->stream->time_base = AVRational{1, fps};

  av_->codec = avcodec_alloc_context3(codec);
  if (av_->codec == nullptr) {
    return fail(ffmpeg_error("avcodec_alloc_context3 failed."));
  }
  av_->codec->codec_id = AV_CODEC_ID_MJPEG;
  av_->codec->codec_type = AVMEDIA_TYPE_VIDEO;
  av_->codec->width = encode_width_;
  av_->codec->height = encode_height_;
  av_->codec->time_base = AVRational{1, fps};
  av_->codec->framerate = AVRational{fps, 1};
  av_->codec->gop_size = 1;
  apply_video_signal(av_->codec, video.signal());
  if (video.signal().range() == JobOutputVideoSignal::RangeValue::Tv) {
    av_->codec->strict_std_compliance = FF_COMPLIANCE_UNOFFICIAL;
  }
  apply_global_header(av_->codec, av_->format);
  DAILYBOY_RETURN_IF_ERROR(fail(apply_mjpeg_codec_options(
      av_->codec, *options, video.signal(), &quality_)));

  err = avcodec_open2(av_->codec, codec, nullptr);
  if (err < 0) {
    return fail(ffmpeg_error("avcodec_open2: " + av_error_string(err)));
  }
  err = avcodec_parameters_from_context(av_->stream->codecpar, av_->codec);
  if (err < 0) {
    return fail(ffmpeg_error("avcodec_parameters_from_context: " +
                             av_error_string(err)));
  }

  av_->sws = sws_getContext(encode_width_, encode_height_, AV_PIX_FMT_RGB24,
                            encode_width_, encode_height_, av_->codec->pix_fmt,
                            SWS_BILINEAR, nullptr, nullptr, nullptr);
  if (av_->sws == nullptr) {
    return fail(ffmpeg_error("sws_getContext failed."));
  }
  DAILYBOY_RETURN_IF_ERROR(
      fail(apply_sws_video_signal(av_->sws, video.signal())));

  av_->yuv = av_frame_alloc();
  if (av_->yuv == nullptr) {
    return fail(ffmpeg_error("av_frame_alloc failed."));
  }
  av_->yuv->format = av_->codec->pix_fmt;
  av_->yuv->width = encode_width_;
  av_->yuv->height = encode_height_;
  av_->yuv->quality = quality_;
  av_->yuv->color_range = av_->codec->color_range;
  apply_video_signal(av_->yuv, video.signal());
  err = av_frame_get_buffer(av_->yuv, 32);
  if (err < 0) {
    return fail(ffmpeg_error("av_frame_get_buffer: " + av_error_string(err)));
  }

  if ((av_->format->oformat->flags & AVFMT_NOFILE) == 0) {
    err = avio_open(&av_->format->pb, path_str.c_str(), AVIO_FLAG_WRITE);
    if (err < 0) {
      return fail(ffmpeg_error("avio_open: " + av_error_string(err) + " (" +
                               path_str + ")"));
    }
  }

  AVDictionary* mux_opts = nullptr;
  apply_faststart(&mux_opts, options->faststart());
  audio_ = std::make_unique<MovAudioTrack>();
  DAILYBOY_RETURN_IF_ERROR(
      fail(audio_->open_aac_stream_on_mov(av_->format, fps, std::move(audio))));
  err = avformat_write_header(av_->format, &mux_opts);
  av_dict_free(&mux_opts);
  if (err < 0) {
    return fail(ffmpeg_error("avformat_write_header: " + av_error_string(err)));
  }

  opened_ = true;
  return Status::Ok();
}

Status MjpegWriter::write(const Frame& frame) {
  if (!opened_ || av_ == nullptr) {
    return Status::Internal(std::string(INTERNAL_ERROR_ENCODE_1));
  }
  if (frame.width() != width_ || frame.height() != height_) {
    return Status::User(std::string(USER_ERROR_ENCODE_3));
  }

  std::vector<uint8_t> rgb;
  DAILYBOY_RETURN_IF_ERROR(extract_rgb8(frame, rgb));
  std::vector<uint8_t> padded;
  int dst_stride = 0;
  const uint8_t* src = rgb8_with_encode_pad(
      rgb, width_, height_, encode_width_, encode_height_, padded, &dst_stride);

  const uint8_t* planes[1] = {src};
  const int strides[1] = {dst_stride};
  int err = av_frame_make_writable(av_->yuv);
  if (err < 0) {
    return ffmpeg_error("av_frame_make_writable: " + av_error_string(err));
  }
  sws_scale(av_->sws, planes, strides, 0, encode_height_, av_->yuv->data,
            av_->yuv->linesize);
  av_->yuv->pts = pts_;
  av_->yuv->quality = quality_;
  DAILYBOY_RETURN_IF_ERROR(
      send_packet_loop(av_->format, av_->codec, av_->stream, av_->yuv));
  if (audio_) {
    DAILYBOY_RETURN_IF_ERROR(audio_->write_audio_up_to_video_pts(pts_));
  }
  ++pts_;
  return Status::Ok();
}

Status MjpegWriter::close() {
  if (!opened_) {
    av_.reset();
    audio_.reset();
    return Status::Ok();
  }
  Status status =
      send_packet_loop(av_->format, av_->codec, av_->stream, nullptr);
  if (status.ok() && audio_) {
    status = audio_->flush_remaining_audio();
  }
  if (status.ok()) {
    FfmpegLogCapture ffmpeg_logs;
    const int err = av_write_trailer(av_->format);
    if (err < 0) {
      status = ffmpeg_error("av_write_trailer: " + av_error_string(err));
    }
  }

  audio_.reset();
  reset_av(*av_);
  av_.reset();
  opened_ = false;
  return status;
}

}  // namespace dailyboy
