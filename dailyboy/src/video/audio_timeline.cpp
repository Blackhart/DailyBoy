/*!
 * \file audio_timeline.cpp
 * \brief Build slate-silence + per-plan PCM guide tracks at 48 kHz stereo.
 */

#include "video/audio_timeline.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/frame.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <dailyboy/log.hpp>
#include <map>
#include <string>

#include "error/video.hpp"
#include "job/metadata.hpp"
#include "process/path_tokens.hpp"
#include "status.hpp"
#include "video/ffmpeg.hpp"

namespace dailyboy {

namespace {

int samples_for_frame_count(int frames, int fps);
void append_silence_samples(std::vector<int16_t>& pcm, int sample_count);
void append_slate_silence(AudioPcmTimeline& timeline, int slate_frames,
                          int fps);
void pad_or_trim_to_sample_count(std::vector<int16_t>& pcm, int sample_count);
Status find_best_audio_stream(AVFormatContext* format, const std::string& path,
                              int* stream_index);
StatusOr<AVCodecContext*> allocate_and_open_decoder(AVStream* stream,
                                                    const AVCodec* codec);
Status receive_and_append_frames(AVCodecContext* decoder, SwrContext* swr,
                                 AVFrame* frame, std::vector<int16_t>& pcm);
Status read_all_audio_packets(AVFormatContext* format, int stream_index,
                              AVCodecContext* decoder, SwrContext* swr,
                              AVPacket* packet, AVFrame* frame,
                              std::vector<int16_t>& pcm);
StatusOr<std::vector<int16_t>> decode_opened_audio(AVFormatContext* format,
                                                   int stream_index);
StatusOr<std::vector<int16_t>> load_and_fit_plan_audio(
    const JobPlan& plan, int want, int fps, int start_sample,
    const std::map<std::string, JobMetadataSubstitutionValue>& subs);
StatusOr<std::shared_ptr<const AudioPcmTimeline>> assemble_job_audio_timeline(
    const Job& job, int fps);

double seconds_from_samples(int samples) {
  return static_cast<double>(samples) / AudioPcmTimeline::kSampleRate;
}

int frames_from_samples(int samples, int fps) {
  return (samples * fps + AudioPcmTimeline::kSampleRate / 2) /
         AudioPcmTimeline::kSampleRate;
}

std::string format_seconds(double seconds) {
  char buffer[32];
  std::snprintf(buffer, sizeof(buffer), "%.3f", seconds);
  return buffer;
}

void log_plan_audio_start(const JobPlan& plan, int fps, int start_sample) {
  const int start_frame = frames_from_samples(start_sample, fps);
  log_debug("audio: plan " + plan.id() + " starts at " +
            format_seconds(seconds_from_samples(start_sample)) +
            "s (movie frame " + std::to_string(start_frame) + ")");
}

void log_plan_audio_plate_range(const JobPlan& plan, int plate_frames,
                                int fps) {
  log_debug("audio: plan " + plan.id() + " plates " +
            std::to_string(plan.sequence().frame_start()) + "-" +
            std::to_string(plan.sequence().frame_end()) + " (" +
            std::to_string(plate_frames) + " frames, " +
            format_seconds(static_cast<double>(plate_frames) / fps) + "s)");
}

void log_plan_audio_fit_delta(const JobPlan& plan, int source_samples,
                              int want_samples, int fps) {
  const int delta = source_samples - want_samples;
  if (delta == 0) {
    log_debug("audio: plan " + plan.id() + " fit exact (no omit/pad)");
    return;
  }
  const int delta_frames = frames_from_samples(std::abs(delta), fps);
  const std::string amount =
      std::to_string(delta_frames) + " frames / " +
      format_seconds(seconds_from_samples(std::abs(delta))) + "s";
  if (delta > 0) {
    log_debug("audio: plan " + plan.id() + " omit " + amount +
              " (beyond frame_end)");
  } else {
    log_debug("audio: plan " + plan.id() + " pad silence " + amount +
              " (short of frame_end)");
  }
}

void log_slate_audio_silence(int slate_frames, int fps) {
  if (slate_frames <= 0) {
    log_debug("audio: slate silence none");
    return;
  }
  log_debug("audio: slate silence starts at 0.000s for " +
            std::to_string(slate_frames) + " frames / " +
            format_seconds(static_cast<double>(slate_frames) / fps) + "s");
}

int samples_for_frame_count(int frames, int fps) {
  return (frames * AudioPcmTimeline::kSampleRate + fps / 2) / fps;
}

void append_silence_samples(std::vector<int16_t>& pcm, int sample_count) {
  const std::size_t n =
      static_cast<std::size_t>(sample_count) * AudioPcmTimeline::kChannels;
  pcm.resize(pcm.size() + n, 0);
}

void append_slate_silence(AudioPcmTimeline& timeline, int slate_frames,
                          int fps) {
  if (slate_frames <= 0) {
    return;
  }
  append_silence_samples(timeline.samples(),
                         samples_for_frame_count(slate_frames, fps));
}

void pad_or_trim_to_sample_count(std::vector<int16_t>& pcm, int sample_count) {
  const std::size_t want =
      static_cast<std::size_t>(sample_count) * AudioPcmTimeline::kChannels;
  if (pcm.size() > want) {
    pcm.resize(want);
    return;
  }
  pcm.resize(want, 0);
}

Status find_best_audio_stream(AVFormatContext* format, const std::string& path,
                              int* stream_index) {
  *stream_index =
      av_find_best_stream(format, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
  if (*stream_index < 0) {
    return Status::User(std::string(USER_ERROR_ENCODE_5) + " " + path +
                        ": no audio stream.");
  }
  return Status::Ok();
}

Status open_audio_demux(const std::string& path, AVFormatContext** format,
                        int* stream_index) {
  *format = nullptr;
  int err = avformat_open_input(format, path.c_str(), nullptr, nullptr);
  if (err < 0) {
    return Status::User(std::string(USER_ERROR_ENCODE_5) + " " + path + ": " +
                        av_error_string(err));
  }
  err = avformat_find_stream_info(*format, nullptr);
  if (err < 0) {
    avformat_close_input(format);
    return Status::User(std::string(USER_ERROR_ENCODE_5) + " " + path);
  }
  Status status = find_best_audio_stream(*format, path, stream_index);
  if (!status.ok()) {
    avformat_close_input(format);
  }
  return status;
}

StatusOr<AVCodecContext*> allocate_and_open_decoder(AVStream* stream,
                                                    const AVCodec* codec) {
  AVCodecContext* decoder = avcodec_alloc_context3(codec);
  if (decoder == nullptr) {
    return Status::User(std::string(USER_ERROR_ENCODE_5));
  }
  int err = avcodec_parameters_to_context(decoder, stream->codecpar);
  if (err < 0 || avcodec_open2(decoder, codec, nullptr) < 0) {
    avcodec_free_context(&decoder);
    return Status::User(std::string(USER_ERROR_ENCODE_5));
  }
  return decoder;
}

StatusOr<AVCodecContext*> open_audio_decoder(AVFormatContext* format,
                                             int stream_index) {
  AVStream* stream = format->streams[stream_index];
  const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
  if (codec == nullptr) {
    return Status::User(std::string(USER_ERROR_ENCODE_5) +
                        " unsupported audio codec.");
  }
  return allocate_and_open_decoder(stream, codec);
}

SwrContext* make_stereo_48k_resampler(AVCodecContext* decoder) {
  SwrContext* swr = nullptr;
  AVChannelLayout out_ch = AV_CHANNEL_LAYOUT_STEREO;
  int err = swr_alloc_set_opts2(&swr, &out_ch, AV_SAMPLE_FMT_S16,
                                AudioPcmTimeline::kSampleRate,
                                &decoder->ch_layout, decoder->sample_fmt,
                                decoder->sample_rate, 0, nullptr);
  if (err < 0 || swr_init(swr) < 0) {
    swr_free(&swr);
    return nullptr;
  }
  return swr;
}

Status append_decoded_frame(std::vector<int16_t>& pcm, SwrContext* swr,
                            AVFrame* frame) {
  const int out_count = swr_get_out_samples(swr, frame->nb_samples);
  std::vector<int16_t> chunk(static_cast<std::size_t>(out_count) *
                             AudioPcmTimeline::kChannels);
  uint8_t* out_planes[1] = {reinterpret_cast<uint8_t*>(chunk.data())};
  const int converted = swr_convert(
      swr, out_planes, out_count,
      const_cast<const uint8_t**>(frame->extended_data), frame->nb_samples);
  if (converted < 0) {
    return Status::User(std::string(USER_ERROR_ENCODE_5));
  }
  chunk.resize(static_cast<std::size_t>(converted) *
               AudioPcmTimeline::kChannels);
  pcm.insert(pcm.end(), chunk.begin(), chunk.end());
  return Status::Ok();
}

Status receive_and_append_frames(AVCodecContext* decoder, SwrContext* swr,
                                 AVFrame* frame, std::vector<int16_t>& pcm) {
  while (avcodec_receive_frame(decoder, frame) >= 0) {
    DAILYBOY_RETURN_IF_ERROR(append_decoded_frame(pcm, swr, frame));
  }
  return Status::Ok();
}

Status decode_one_packet(AVCodecContext* decoder, AVPacket* packet,
                         SwrContext* swr, AVFrame* frame,
                         std::vector<int16_t>& pcm) {
  if (avcodec_send_packet(decoder, packet) < 0) {
    return Status::Ok();
  }
  return receive_and_append_frames(decoder, swr, frame, pcm);
}

Status read_all_audio_packets(AVFormatContext* format, int stream_index,
                              AVCodecContext* decoder, SwrContext* swr,
                              AVPacket* packet, AVFrame* frame,
                              std::vector<int16_t>& pcm) {
  while (av_read_frame(format, packet) >= 0) {
    if (packet->stream_index != stream_index) {
      av_packet_unref(packet);
      continue;
    }
    const Status status = decode_one_packet(decoder, packet, swr, frame, pcm);
    av_packet_unref(packet);
    if (!status.ok()) {
      return status;
    }
  }
  return Status::Ok();
}

Status drain_decoder_packets(AVFormatContext* format, int stream_index,
                             AVCodecContext* decoder, SwrContext* swr,
                             std::vector<int16_t>& pcm) {
  AVPacket* packet = av_packet_alloc();
  AVFrame* frame = av_frame_alloc();
  if (packet == nullptr || frame == nullptr) {
    av_packet_free(&packet);
    av_frame_free(&frame);
    return Status::User(std::string(USER_ERROR_ENCODE_5));
  }
  Status status = read_all_audio_packets(format, stream_index, decoder, swr,
                                         packet, frame, pcm);
  if (status.ok()) {
    avcodec_send_packet(decoder, nullptr);
    status = receive_and_append_frames(decoder, swr, frame, pcm);
  }
  av_packet_free(&packet);
  av_frame_free(&frame);
  return status;
}

StatusOr<std::vector<int16_t>> decode_opened_audio(AVFormatContext* format,
                                                   int stream_index) {
  StatusOr<AVCodecContext*> decoder = open_audio_decoder(format, stream_index);
  if (!decoder.ok()) {
    avformat_close_input(&format);
    return decoder.status();
  }
  SwrContext* swr = make_stereo_48k_resampler(decoder.value());
  if (swr == nullptr) {
    avcodec_free_context(&decoder.value());
    avformat_close_input(&format);
    return Status::User(std::string(USER_ERROR_ENCODE_5) + " resample.");
  }
  std::vector<int16_t> pcm;
  Status status =
      drain_decoder_packets(format, stream_index, decoder.value(), swr, pcm);
  swr_free(&swr);
  avcodec_free_context(&decoder.value());
  avformat_close_input(&format);
  if (!status.ok()) {
    return status;
  }
  return pcm;
}

StatusOr<std::vector<int16_t>> load_audio_file_as_pcm(const std::string& path) {
  silence_ffmpeg_logs();
  FfmpegLogCapture logs;
  AVFormatContext* format = nullptr;
  int stream_index = -1;
  DAILYBOY_RETURN_IF_ERROR(open_audio_demux(path, &format, &stream_index));
  return decode_opened_audio(format, stream_index);
}

StatusOr<std::vector<int16_t>> load_and_fit_plan_audio(
    const JobPlan& plan, int want, int fps, int start_sample,
    const std::map<std::string, JobMetadataSubstitutionValue>& subs) {
  DAILYBOY_ASSIGN_OR_RETURN(
      std::string path,
      expand_path_tokens(plan.audio()->path(), subs, USER_ERROR_ENCODE_5));
  DAILYBOY_ASSIGN_OR_RETURN(std::vector<int16_t> pcm,
                            load_audio_file_as_pcm(path));
  const int plate_frames =
      plan.sequence().frame_end() - plan.sequence().frame_start() + 1;
  const int source_samples =
      static_cast<int>(pcm.size() / AudioPcmTimeline::kChannels);
  log_plan_audio_start(plan, fps, start_sample);
  log_plan_audio_plate_range(plan, plate_frames, fps);
  log_debug("audio: plan " + plan.id() + " source " + path + " -> " +
            std::to_string(source_samples) + " samples @48k (" +
            format_seconds(seconds_from_samples(source_samples)) + "s)");
  log_plan_audio_fit_delta(plan, source_samples, want, fps);
  pad_or_trim_to_sample_count(pcm, want);
  return pcm;
}

StatusOr<std::vector<int16_t>> load_plan_audio_or_silence(
    const JobPlan& plan, int fps, int start_sample,
    const std::map<std::string, JobMetadataSubstitutionValue>& subs) {
  const int frames =
      plan.sequence().frame_end() - plan.sequence().frame_start() + 1;
  const int want = samples_for_frame_count(frames, fps);
  if (!plan.audio().has_value()) {
    log_plan_audio_start(plan, fps, start_sample);
    log_plan_audio_plate_range(plan, frames, fps);
    log_debug("audio: plan " + plan.id() + " no audio.path -> plate silence");
    std::vector<int16_t> pcm;
    append_silence_samples(pcm, want);
    return pcm;
  }
  return load_and_fit_plan_audio(plan, want, fps, start_sample, subs);
}

bool any_plan_has_audio(const Job& job) {
  for (const JobPlan& plan : job.plans().plans()) {
    if (plan.audio().has_value()) {
      return true;
    }
  }
  return false;
}

StatusOr<std::shared_ptr<const AudioPcmTimeline>> assemble_job_audio_timeline(
    const Job& job, int fps) {
  auto timeline = std::make_shared<AudioPcmTimeline>();
  const int slate_frames = job.layout().slate().duration_frames();
  log_slate_audio_silence(slate_frames, fps);
  append_slate_silence(*timeline, slate_frames, fps);
  for (const JobPlan& plan : job.plans().plans()) {
    const int start_sample = timeline->sample_count();
    DAILYBOY_ASSIGN_OR_RETURN(
        std::vector<int16_t> chunk,
        load_plan_audio_or_silence(plan, fps, start_sample,
                                   job.metadata().substitutions()));
    timeline->samples().insert(timeline->samples().end(), chunk.begin(),
                               chunk.end());
  }
  log_debug("audio: timeline " + std::to_string(timeline->sample_count()) +
            " samples / " +
            format_seconds(seconds_from_samples(timeline->sample_count())) +
            "s (slate silence + plans)");
  return std::shared_ptr<const AudioPcmTimeline>{std::move(timeline)};
}

}  // namespace

StatusOr<std::shared_ptr<const AudioPcmTimeline>> build_job_audio_timeline(
    const Job& job, int fps) {
  if (fps <= 0) {
    return Status::User(std::string(USER_ERROR_ENCODE_5) + " invalid fps.");
  }
  if (!any_plan_has_audio(job)) {
    return std::shared_ptr<const AudioPcmTimeline>{};
  }
  return assemble_job_audio_timeline(job, fps);
}

}  // namespace dailyboy
