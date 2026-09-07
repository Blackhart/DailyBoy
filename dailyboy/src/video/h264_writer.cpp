/*!
 * \file h264_writer.cpp
 * \brief FFmpeg libx264/MOV encode: RGB ImageBuf → YUV, one frame at a time.
 */

#include "video/h264_writer.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
#include <libavutil/frame.h>
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

const char* preset_name(JobOutputVideoH264::JobOutputVideoH264PresetValue v) {
  switch (v) {
    case JobOutputVideoH264::JobOutputVideoH264PresetValue::Ultrafast:
      return "ultrafast";
    case JobOutputVideoH264::JobOutputVideoH264PresetValue::Superfast:
      return "superfast";
    case JobOutputVideoH264::JobOutputVideoH264PresetValue::Veryfast:
      return "veryfast";
    case JobOutputVideoH264::JobOutputVideoH264PresetValue::Faster:
      return "faster";
    case JobOutputVideoH264::JobOutputVideoH264PresetValue::Fast:
      return "fast";
    case JobOutputVideoH264::JobOutputVideoH264PresetValue::Medium:
      return "medium";
    case JobOutputVideoH264::JobOutputVideoH264PresetValue::Slow:
      return "slow";
  }
  return "medium";
}

const char* tune_name(JobOutputVideoH264::JobOutputVideoH264TuneValue v) {
  switch (v) {
    case JobOutputVideoH264::JobOutputVideoH264TuneValue::Film:
      return "film";
    case JobOutputVideoH264::JobOutputVideoH264TuneValue::Animation:
      return "animation";
    case JobOutputVideoH264::JobOutputVideoH264TuneValue::Grain:
      return "grain";
    case JobOutputVideoH264::JobOutputVideoH264TuneValue::Stillimage:
      return "stillimage";
    case JobOutputVideoH264::JobOutputVideoH264TuneValue::Fastdecode:
      return "fastdecode";
    case JobOutputVideoH264::JobOutputVideoH264TuneValue::Zerolatency:
      return "zerolatency";
    case JobOutputVideoH264::JobOutputVideoH264TuneValue::Unspecified:
      return nullptr;
  }
  return nullptr;
}

const char* profile_name(const JobOutputVideoH264& options) {
  using Profile = JobOutputVideoH264::JobOutputVideoH264ProfileValue;
  using Pix = JobOutputVideoH264::JobOutputVideoH264PixFmtValue;
  switch (options.profile()) {
    case Profile::Baseline:
      return "baseline";
    case Profile::Main:
      return "main";
    case Profile::High:
      return "high";
    case Profile::High422:
      return "high422";
    case Profile::Unspecified:
      return options.pix_fmt() == Pix::Yuv422p ? "high422" : "high";
  }
  return "high";
}

AVPixelFormat h264_av_pix_fmt(
    JobOutputVideoH264::JobOutputVideoH264PixFmtValue v) {
  return v == JobOutputVideoH264::JobOutputVideoH264PixFmtValue::Yuv422p
             ? AV_PIX_FMT_YUV422P
             : AV_PIX_FMT_YUV420P;
}

Status apply_pix_fmt(AVCodecContext* codec, const JobOutputVideoH264& options) {
  codec->pix_fmt = h264_av_pix_fmt(options.pix_fmt());
  log_debug(std::string("encode: pix_fmt ") +
            (codec->pix_fmt == AV_PIX_FMT_YUV422P ? "yuv422p" : "yuv420p"));
  return Status::Ok();
}

Status apply_gop(AVCodecContext* codec, const JobOutputVideoH264& options) {
  if (options.gop() > 0) {
    codec->gop_size = options.gop();
    log_debug("encode: gop " + std::to_string(options.gop()));
  } else {
    log_debug("encode: gop default");
  }
  return Status::Ok();
}

Status apply_bitrate_kbps(AVCodecContext* codec,
                          const JobOutputVideoH264& options) {
  if (options.bitrate_kbps() > 0) {
    codec->bit_rate = static_cast<int64_t>(options.bitrate_kbps()) * 1000;
    log_debug("encode: bitrate_kbps " + std::to_string(options.bitrate_kbps()));
  } else {
    log_debug("encode: bitrate_kbps unused (crf)");
  }
  return Status::Ok();
}

Status apply_preset(AVCodecContext* codec, const JobOutputVideoH264& options) {
  const char* name = preset_name(options.preset());
  log_debug(std::string("encode: preset ") + name);
  if (av_opt_set(codec->priv_data, "preset", name, 0) < 0) {
    return ffmpeg_error("failed to set x264 preset.");
  }
  return Status::Ok();
}

Status apply_tune(AVCodecContext* codec, const JobOutputVideoH264& options) {
  const char* name = tune_name(options.tune());
  if (name == nullptr) {
    log_debug("encode: tune unset");
    return Status::Ok();
  }
  log_debug(std::string("encode: tune ") + name);
  if (av_opt_set(codec->priv_data, "tune", name, 0) < 0) {
    return ffmpeg_error("failed to set x264 tune.");
  }
  return Status::Ok();
}

Status apply_profile(AVCodecContext* codec, const JobOutputVideoH264& options) {
  const char* name = profile_name(options);
  log_debug(std::string("encode: profile ") + name);
  if (av_opt_set(codec->priv_data, "profile", name, 0) < 0) {
    return ffmpeg_error("failed to set x264 profile.");
  }
  return Status::Ok();
}

Status apply_level(AVCodecContext* codec, const JobOutputVideoH264& options) {
  if (options.level().empty()) {
    log_debug("encode: level auto");
    return Status::Ok();
  }
  log_debug("encode: level " + options.level());
  if (av_opt_set(codec->priv_data, "level", options.level().c_str(), 0) < 0) {
    return ffmpeg_error("failed to set x264 level.");
  }
  return Status::Ok();
}

Status apply_crf(AVCodecContext* codec, const JobOutputVideoH264& options) {
  if (options.bitrate_kbps() > 0) {
    log_debug("encode: crf unused (bitrate_kbps)");
    return Status::Ok();
  }
  const std::string crf = std::to_string(options.crf());
  log_debug("encode: crf " + crf);
  if (av_opt_set(codec->priv_data, "crf", crf.c_str(), 0) < 0) {
    return ffmpeg_error("failed to set x264 crf.");
  }
  return Status::Ok();
}

Status apply_h264_codec_options(AVCodecContext* codec,
                                const JobOutputVideoH264& options) {
  DAILYBOY_RETURN_IF_ERROR(apply_pix_fmt(codec, options));
  DAILYBOY_RETURN_IF_ERROR(apply_gop(codec, options));
  DAILYBOY_RETURN_IF_ERROR(apply_bitrate_kbps(codec, options));
  DAILYBOY_RETURN_IF_ERROR(apply_preset(codec, options));
  DAILYBOY_RETURN_IF_ERROR(apply_tune(codec, options));
  DAILYBOY_RETURN_IF_ERROR(apply_profile(codec, options));
  DAILYBOY_RETURN_IF_ERROR(apply_level(codec, options));
  DAILYBOY_RETURN_IF_ERROR(apply_crf(codec, options));
  return Status::Ok();
}

}  // namespace

struct H264Writer::AvState {
  AVFormatContext* format = nullptr;
  AVCodecContext* codec = nullptr;
  AVStream* stream = nullptr;
  SwsContext* sws = nullptr;
  AVFrame* yuv = nullptr;
};

void H264Writer::reset_av(AvState& av) {
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

H264Writer::H264Writer() = default;

H264Writer::~H264Writer() { close(); }

Status H264Writer::open(const std::filesystem::path& path, int width,
                        int height, int fps, const JobOutputVideo& video,
                        std::shared_ptr<const AudioPcmTimeline> audio) {
  silence_ffmpeg_logs();
  const JobOutputVideoH264* options =
      std::get_if<JobOutputVideoH264>(&video.codec_options());
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
  encode_width_ = width + (width % 2);
  encode_height_ = height + (height % 2);
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

  const AVCodec* codec = avcodec_find_encoder_by_name("libx264");
  if (codec == nullptr) {
    return fail(ffmpeg_error("libx264 encoder not found."));
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
  av_->codec->codec_id = AV_CODEC_ID_H264;
  av_->codec->codec_type = AVMEDIA_TYPE_VIDEO;
  av_->codec->width = encode_width_;
  av_->codec->height = encode_height_;
  av_->codec->time_base = AVRational{1, fps};
  av_->codec->framerate = AVRational{fps, 1};
  apply_video_signal(av_->codec, video.signal());
  apply_global_header(av_->codec, av_->format);
  DAILYBOY_RETURN_IF_ERROR(
      fail(apply_h264_codec_options(av_->codec, *options)));

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

Status H264Writer::write(const Frame& frame) {
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
  DAILYBOY_RETURN_IF_ERROR(
      send_packet_loop(av_->format, av_->codec, av_->stream, av_->yuv));
  if (audio_) {
    DAILYBOY_RETURN_IF_ERROR(audio_->write_audio_up_to_video_pts(pts_));
  }
  ++pts_;
  return Status::Ok();
}

Status H264Writer::close() {
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
