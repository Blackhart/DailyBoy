/*!
 * \file mov_audio_track.cpp
 * \brief AAC-LC mux helper for MOV guide tracks.
 */

#include "video/mov_audio_track.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/frame.h>
}

#include <cstring>

#include "error/video.hpp"
#include "status.hpp"
#include "video/ffmpeg.hpp"

namespace dailyboy {

namespace {

int aac_frame_size(const AVCodecContext* codec) {
  return codec->frame_size > 0 ? codec->frame_size : 1024;
}

Status copy_encoder_params_to_stream(AVCodecContext* codec, AVStream* stream) {
  const int err = avcodec_parameters_from_context(stream->codecpar, codec);
  if (err < 0) {
    return Status::User(std::string(USER_ERROR_ENCODE_6) + " " +
                        av_error_string(err));
  }
  return Status::Ok();
}

Status configure_aac_encoder(AVCodecContext* codec, AVFormatContext* format) {
  codec->codec_id = AV_CODEC_ID_AAC;
  codec->codec_type = AVMEDIA_TYPE_AUDIO;
  codec->sample_fmt = AV_SAMPLE_FMT_FLTP;
  codec->sample_rate = AudioPcmTimeline::kSampleRate;
  codec->bit_rate = MovAudioTrack::kAacBitrate;
  av_channel_layout_default(&codec->ch_layout, AudioPcmTimeline::kChannels);
  codec->time_base = AVRational{1, codec->sample_rate};
  if (format->oformat->flags & AVFMT_GLOBALHEADER) {
    codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
  }
  return Status::Ok();
}

StatusOr<AVFrame*> allocate_aac_frame(AVCodecContext* codec, int frame_size) {
  AVFrame* frame = av_frame_alloc();
  if (frame == nullptr) {
    return Status::User(std::string(USER_ERROR_ENCODE_6));
  }
  frame->nb_samples = frame_size;
  frame->format = codec->sample_fmt;
  av_channel_layout_copy(&frame->ch_layout, &codec->ch_layout);
  frame->sample_rate = codec->sample_rate;
  if (av_frame_get_buffer(frame, 0) < 0) {
    av_frame_free(&frame);
    return Status::User(std::string(USER_ERROR_ENCODE_6));
  }
  return frame;
}

}  // namespace

MovAudioTrack::~MovAudioTrack() { free_encoder(); }

void MovAudioTrack::free_encoder() {
  if (codec_ != nullptr) {
    avcodec_free_context(&codec_);
  }
  format_ = nullptr;
  stream_ = nullptr;
  timeline_.reset();
  next_sample_ = 0;
  next_pts_ = 0;
}

Status MovAudioTrack::open_aac_stream_on_mov(
    AVFormatContext* format, int fps,
    std::shared_ptr<const AudioPcmTimeline> timeline) {
  if (format == nullptr || fps <= 0 || !timeline || timeline->empty()) {
    return Status::Ok();
  }
  silence_ffmpeg_logs();
  const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
  if (codec == nullptr) {
    return Status::User(std::string(USER_ERROR_ENCODE_6) + " aac missing.");
  }
  return finish_open_aac_stream(format, fps, codec, std::move(timeline));
}

Status MovAudioTrack::finish_open_aac_stream(
    AVFormatContext* format, int fps, const AVCodec* codec,
    std::shared_ptr<const AudioPcmTimeline> timeline) {
  stream_ = avformat_new_stream(format, nullptr);
  codec_ = avcodec_alloc_context3(codec);
  if (stream_ == nullptr || codec_ == nullptr) {
    free_encoder();
    return Status::User(std::string(USER_ERROR_ENCODE_6));
  }
  DAILYBOY_RETURN_IF_ERROR(configure_aac_encoder(codec_, format));
  const int err = avcodec_open2(codec_, codec, nullptr);
  if (err < 0) {
    free_encoder();
    return Status::User(std::string(USER_ERROR_ENCODE_6) + " " +
                        av_error_string(err));
  }
  stream_->time_base = codec_->time_base;
  DAILYBOY_RETURN_IF_ERROR(copy_encoder_params_to_stream(codec_, stream_));
  format_ = format;
  fps_ = fps;
  timeline_ = std::move(timeline);
  next_sample_ = 0;
  next_pts_ = 0;
  return Status::Ok();
}

Status MovAudioTrack::send_audio_frame(AVFrame* frame) {
  return send_packet_loop(format_, codec_, stream_, frame);
}

void MovAudioTrack::fill_planar_float_from_pcm(AVFrame* frame,
                                               int sample_count) {
  const int16_t* src =
      timeline_->samples().data() +
      static_cast<std::size_t>(next_sample_) * AudioPcmTimeline::kChannels;
  for (int i = 0; i < frame->nb_samples; ++i) {
    for (int ch = 0; ch < AudioPcmTimeline::kChannels; ++ch) {
      float sample = 0.0f;
      if (i < sample_count) {
        sample = static_cast<float>(src[i * AudioPcmTimeline::kChannels + ch]) /
                 32768.0f;
      }
      reinterpret_cast<float*>(frame->data[ch])[i] = sample;
    }
  }
}

Status MovAudioTrack::encode_pcm_chunk(int sample_count) {
  if (sample_count <= 0 || !timeline_ || codec_ == nullptr) {
    return Status::Ok();
  }
  const int frame_size = aac_frame_size(codec_);
  DAILYBOY_ASSIGN_OR_RETURN(AVFrame * frame,
                            allocate_aac_frame(codec_, frame_size));
  fill_planar_float_from_pcm(frame, sample_count);
  frame->pts = next_pts_;
  next_pts_ += frame_size;
  next_sample_ += sample_count;
  const Status status = send_audio_frame(frame);
  av_frame_free(&frame);
  return status;
}

Status MovAudioTrack::write_audio_up_to_video_pts(int64_t video_pts) {
  if (!active() || !timeline_) {
    return Status::Ok();
  }
  const int frame_size = aac_frame_size(codec_);
  const int64_t target = (video_pts + 1) * AudioPcmTimeline::kSampleRate / fps_;
  const int64_t available = timeline_->sample_count();
  while (next_sample_ + frame_size <= target &&
         next_sample_ + frame_size <= available) {
    DAILYBOY_RETURN_IF_ERROR(encode_pcm_chunk(frame_size));
  }
  return Status::Ok();
}

Status MovAudioTrack::flush_remaining_audio() {
  if (!active() || !timeline_) {
    return Status::Ok();
  }
  const int frame_size = aac_frame_size(codec_);
  const int64_t available = timeline_->sample_count();
  while (next_sample_ + frame_size <= available) {
    DAILYBOY_RETURN_IF_ERROR(encode_pcm_chunk(frame_size));
  }
  const int leftover = static_cast<int>(available - next_sample_);
  if (leftover > 0) {
    DAILYBOY_RETURN_IF_ERROR(encode_pcm_chunk(leftover));
  }
  return send_audio_frame(nullptr);
}

}  // namespace dailyboy
