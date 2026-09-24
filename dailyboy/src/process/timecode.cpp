/*!
 * \file timecode.cpp
 * \brief SMPTE parse/format and job-level timecode resolution.
 */

#include "process/timecode.hpp"

#include <cstdio>
#include <cstring>
#include <optional>
#include <string>
#include <variant>

#include "error/job.hpp"
#include "job/output.hpp"
#include "video/video_writer.hpp"

extern "C" {
#include <libavutil/timecode.h>
}

namespace dailyboy {

namespace {

constexpr int kHoursPerDay = 24;

Status init_av_timecode(AVTimecode* tc, int fps, bool drop_frame) {
  unsigned flags = 0;
  if (drop_frame) {
    if (fps != 30 && fps != 60) {
      return Status::User(std::string(USER_ERROR_JOB_103));
    }
    flags |= AV_TIMECODE_FLAG_DROPFRAME;
  }
  const AVRational rate{fps, 1};
  const int err =
      av_timecode_init(tc, rate, static_cast<int>(flags), 0, nullptr);
  if (err < 0) {
    return Status::User(std::string(USER_ERROR_JOB_102));
  }
  return Status::Ok();
}

std::int64_t wrap_frames(std::int64_t frames, int fps) {
  const std::int64_t day =
      static_cast<std::int64_t>(kHoursPerDay) * 60 * 60 * fps;
  frames %= day;
  if (frames < 0) {
    frames += day;
  }
  return frames;
}

StatusOr<std::int64_t> parse_smpte_with_ffmpeg(const std::string& text, int fps,
                                               bool drop_frame) {
  AVTimecode tc{};
  DAILYBOY_RETURN_IF_ERROR(init_av_timecode(&tc, fps, drop_frame));
  const int err = av_timecode_init_from_string(&tc, AVRational{fps, 1},
                                               text.c_str(), nullptr);
  if (err < 0) {
    return Status::User(std::string(USER_ERROR_JOB_102));
  }
  if (drop_frame != ((tc.flags & AV_TIMECODE_FLAG_DROPFRAME) != 0)) {
    return Status::User(std::string(USER_ERROR_JOB_102));
  }
  return static_cast<std::int64_t>(tc.start);
}

const JobPlan* first_plan(const Job& job) {
  if (job.plans().plans().empty()) {
    return nullptr;
  }
  return &job.plans().plans().front();
}

}  // namespace

StatusOr<std::string> frames_to_smpte(std::int64_t frames, int fps,
                                      bool drop_frame) {
  if (fps < 1) {
    return Status::User(std::string(USER_ERROR_JOB_102));
  }
  AVTimecode tc{};
  DAILYBOY_RETURN_IF_ERROR(init_av_timecode(&tc, fps, drop_frame));
  frames = wrap_frames(frames, fps);
  char buffer[AV_TIMECODE_STR_SIZE];
  av_timecode_make_string(&tc, buffer, static_cast<int>(frames));
  return std::string(buffer);
}

StatusOr<std::int64_t> smpte_to_frames(const std::string& text, int fps,
                                       bool drop_frame) {
  if (fps < 1) {
    return Status::User(std::string(USER_ERROR_JOB_102));
  }
  const bool has_semicolon = text.find(';') != std::string::npos;
  if (has_semicolon != drop_frame) {
    return Status::User(std::string(USER_ERROR_JOB_102));
  }
  return parse_smpte_with_ffmpeg(text, fps, drop_frame);
}

StatusOr<int> resolve_timecode_fps(const Job& job) {
  const JobPlan* plan = first_plan(job);
  const bool has_timecode = plan != nullptr && plan->timecode().has_value();
  std::optional<int> fps;
  for (const JobOutputVideo& video : job.output().videos().videos()) {
    if (!video.enabled()) {
      continue;
    }
    if (!fps.has_value()) {
      fps = video.fps();
      continue;
    }
    if (*fps != video.fps() && has_timecode) {
      return Status::User(std::string(USER_ERROR_JOB_104));
    }
  }
  const int resolved = fps.value_or(VideoWriter::kDefaultFps);
  if (has_timecode && plan->timecode()->drop_frame() && resolved != 30 &&
      resolved != 60) {
    return Status::User(std::string(USER_ERROR_JOB_103));
  }
  return resolved;
}

StatusOr<std::int64_t> resolve_timecode_start_frames(const Job& job, int fps) {
  const JobPlan* plan = first_plan(job);
  if (plan == nullptr || !plan->timecode().has_value()) {
    return Status::User(std::string(USER_ERROR_JOB_101));
  }
  const JobPlanTimecode& timecode = *plan->timecode();
  if (std::holds_alternative<int>(timecode.start())) {
    return static_cast<std::int64_t>(std::get<int>(timecode.start()));
  }
  return smpte_to_frames(std::get<std::string>(timecode.start()), fps,
                         timecode.drop_frame());
}

StatusOr<std::string> format_overlay_timecode(const Job& job, int frame) {
  const JobPlan* plan = first_plan(job);
  if (plan == nullptr || !plan->timecode().has_value()) {
    return std::string{};
  }
  DAILYBOY_ASSIGN_OR_RETURN(const int fps, resolve_timecode_fps(job));
  DAILYBOY_ASSIGN_OR_RETURN(const std::int64_t start_frames,
                            resolve_timecode_start_frames(job, fps));
  const int offset = frame - plan->sequence().frame_start();
  return frames_to_smpte(start_frames + offset, fps,
                         plan->timecode()->drop_frame());
}

StatusOr<std::string> format_mov_timecode(const Job& job) {
  const JobPlan* plan = first_plan(job);
  if (plan == nullptr || !plan->timecode().has_value()) {
    return std::string{};
  }
  const int slate_duration = job.layout().slate().duration_frames();
  const int first_media_frame =
      plan->sequence().effective_frame_start() - slate_duration;
  return format_overlay_timecode(job, first_media_frame);
}

}  // namespace dailyboy
