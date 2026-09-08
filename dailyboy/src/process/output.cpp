/*!
 * \file output.cpp
 * \brief Open, write, and close daily movie and image-sequence writers.
 */

#include "process/output.hpp"

#include <dailyboy/log.hpp>
#include <filesystem>
#include <memory>
#include <set>
#include <string>
#include <system_error>
#include <vector>

#include "error/image.hpp"
#include "error/process.hpp"
#include "error/video.hpp"
#include "image/sequence_writer.hpp"
#include "process/colorimetry.hpp"
#include "process/path_tokens.hpp"
#include "status.hpp"
#include "video/audio_timeline.hpp"
#include "video/video_writer.hpp"

namespace dailyboy {

namespace {

struct ActiveVideo {
  const JobOutputVideo* video = nullptr;
  std::filesystem::path path;
  std::unique_ptr<VideoWriter> writer;
};

struct ActiveSequence {
  const JobOutputImageSequence* item = nullptr;
  std::string pattern;
  SequenceWriter writer;
};

bool same_display_view(const JobOutputDisplayView& a,
                       const JobOutputDisplayView& b) {
  return display_view_key(a) == display_view_key(b);
}

std::string size_text(const Frame& frame) {
  return std::to_string(frame.width()) + "x" + std::to_string(frame.height());
}

Status ensure_parent_directory(const std::filesystem::path& path) {
  const std::filesystem::path parent = path.parent_path();
  if (parent.empty()) {
    return Status::Ok();
  }
  std::error_code ec;
  std::filesystem::create_directories(parent, ec);
  if (ec) {
    return Status::User(std::string(USER_ERROR_RENDER_5) + " " +
                        parent.string() + ": " + ec.message());
  }
  return Status::Ok();
}

StatusOr<ActiveVideo> collect_one_video(const Job& job, std::size_t index,
                                        const JobOutputVideo& video) {
  DAILYBOY_ASSIGN_OR_RETURN(
      std::string expanded,
      expand_path_tokens(video.path(), job.metadata().substitutions(),
                         video_path_frame_map_error(index)));
  if (expanded.empty()) {
    return Status::User(std::string(USER_ERROR_ENCODE_1));
  }
  const std::filesystem::path output_path(std::move(expanded));
  DAILYBOY_RETURN_IF_ERROR(ensure_parent_directory(output_path));
  ActiveVideo active;
  active.video = &video;
  active.path = output_path;
  active.writer = make_video_writer(video.codec());
  return active;
}

StatusOr<std::vector<ActiveVideo>> collect_active_videos(const Job& job) {
  std::vector<ActiveVideo> movies;
  std::set<std::string> used_paths;
  const auto& videos = job.output().videos().videos();
  for (std::size_t i = 0; i < videos.size(); ++i) {
    if (!videos[i].enabled()) {
      log_info("render: skip video " + videos[i].id() + " (disabled)");
      continue;
    }
    DAILYBOY_ASSIGN_OR_RETURN(ActiveVideo active,
                              collect_one_video(job, i, videos[i]));
    if (!used_paths.insert(active.path.string()).second) {
      return Status::User(std::string(USER_ERROR_RENDER_6) + " " +
                          active.path.string());
    }
    movies.push_back(std::move(active));
  }
  return movies;
}

JobSequence output_pattern(std::string path) {
  JobSequence sequence;
  sequence.set_path(std::move(path));
  return sequence;
}

StatusOr<ActiveSequence> collect_one_sequence(
    const Job& job, std::size_t index, const JobOutputImageSequence& item) {
  DAILYBOY_ASSIGN_OR_RETURN(
      std::string expanded,
      expand_path_tokens(item.path_pattern(), job.metadata().substitutions(),
                         sequence_path_frame_map_error(index)));
  if (expanded.empty()) {
    return Status::User(std::string(USER_ERROR_IO_2));
  }
  ActiveSequence active;
  active.item = &item;
  active.pattern = expanded;
  DAILYBOY_ASSIGN_OR_RETURN(active.writer,
                            SequenceWriter::open(output_pattern(expanded), {},
                                                 item.format_options()));
  return active;
}

StatusOr<std::vector<ActiveSequence>> collect_active_sequences(const Job& job) {
  std::vector<ActiveSequence> sequences;
  std::set<std::string> used_patterns;
  const auto& items = job.output().image_sequences().image_sequences();
  for (std::size_t i = 0; i < items.size(); ++i) {
    if (!items[i].enabled()) {
      log_info("render: skip image sequence " + items[i].id() + " (disabled)");
      continue;
    }
    DAILYBOY_ASSIGN_OR_RETURN(ActiveSequence active,
                              collect_one_sequence(job, i, items[i]));
    if (!used_patterns.insert(active.pattern).second) {
      return Status::User(std::string(USER_ERROR_RENDER_7) + " " +
                          active.pattern);
    }
    sequences.push_back(std::move(active));
  }
  return sequences;
}

void log_outputs(const std::vector<ActiveVideo>& movies,
                 const std::vector<ActiveSequence>& sequences) {
  log_debug("render: outputs " + std::to_string(movies.size()) + " movie(s) " +
            std::to_string(sequences.size()) + " sequence(s)");
  for (const ActiveVideo& active : movies) {
    log_info("render: video " + active.video->id() + " -> " +
             active.path.string());
  }
  for (const ActiveSequence& active : sequences) {
    log_info("render: image sequence " + active.item->id() + " -> " +
             active.pattern);
  }
}

Status ensure_movie_audio_timeline(
    const Job& job, int fps, int& cached_fps,
    std::shared_ptr<const AudioPcmTimeline>& cached) {
  if (cached && cached_fps == fps) {
    return Status::Ok();
  }
  DAILYBOY_ASSIGN_OR_RETURN(cached, build_job_audio_timeline(job, fps));
  cached_fps = fps;
  return Status::Ok();
}

Status open_movies(std::vector<ActiveVideo>& movies, bool& movies_open,
                   const Frame& frame, const Job& job, int& audio_fps,
                   std::shared_ptr<const AudioPcmTimeline>& audio) {
  log_debug("render: open movies " + size_text(frame));
  for (ActiveVideo& active : movies) {
    log_debug("render: open movie " + active.video->id() + " " +
              active.path.string());
    DAILYBOY_RETURN_IF_ERROR(ensure_movie_audio_timeline(
        job, active.video->fps(), audio_fps, audio));
    DAILYBOY_RETURN_IF_ERROR(
        active.writer->open(active.path, frame.width(), frame.height(),
                            active.video->fps(), *active.video, audio));
  }
  movies_open = true;
  return Status::Ok();
}

Status write_one_movie(ActiveVideo& active, const Frame& frame) {
  log_debug("render: write_movie " + active.video->id());
  Status write_status = active.writer->write(frame);
  if (!write_status.ok() &&
      write_status.message() == std::string(USER_ERROR_ENCODE_3)) {
    return Status::User(std::string(USER_ERROR_RENDER_2));
  }
  return write_status;
}

Status write_one_sequence(ActiveSequence& active, const Frame& frame,
                          int frame_number) {
  log_debug("render: write_sequence " + active.item->id() + " frame " +
            std::to_string(frame_number));
  return active.writer.write(frame, frame_number);
}

Status write_movies(std::vector<ActiveVideo>& movies, bool& movies_open,
                    const Frame& frame,
                    const JobOutputDisplayView* display_view, const Job& job,
                    int& audio_fps,
                    std::shared_ptr<const AudioPcmTimeline>& audio) {
  if (movies.empty()) {
    log_debug("render: write_movie skip (none enabled)");
    return Status::Ok();
  }
  if (!movies_open) {
    DAILYBOY_RETURN_IF_ERROR(
        open_movies(movies, movies_open, frame, job, audio_fps, audio));
  }
  for (ActiveVideo& active : movies) {
    if (display_view != nullptr &&
        !same_display_view(active.video->display_view(), *display_view)) {
      continue;
    }
    DAILYBOY_RETURN_IF_ERROR(write_one_movie(active, frame));
  }
  return Status::Ok();
}

Status write_sequences(std::vector<ActiveSequence>& sequences,
                       const Frame& frame, int frame_number,
                       const JobOutputDisplayView* display_view) {
  if (sequences.empty()) {
    log_debug("render: write_sequence skip (none enabled)");
    return Status::Ok();
  }
  for (ActiveSequence& active : sequences) {
    if (display_view != nullptr &&
        !same_display_view(active.item->display_view(), *display_view)) {
      continue;
    }
    DAILYBOY_RETURN_IF_ERROR(write_one_sequence(active, frame, frame_number));
  }
  return Status::Ok();
}

}  // namespace

struct Outputs::Impl {
  const Job* job = nullptr;
  std::vector<ActiveVideo> movies;
  std::vector<ActiveSequence> sequences;
  std::shared_ptr<const AudioPcmTimeline> audio;
  int audio_fps = 0;
  bool movies_open = false;
  bool closed = false;
};

Outputs::Outputs() = default;
Outputs::Outputs(Outputs&&) noexcept = default;
Outputs& Outputs::operator=(Outputs&&) noexcept = default;
Outputs::~Outputs() = default;

StatusOr<Outputs> Outputs::open(const Job& job) {
  log_debug("Outputs::open");
  Outputs out;
  out.impl_ = std::make_unique<Impl>();
  out.impl_->job = &job;
  DAILYBOY_ASSIGN_OR_RETURN(out.impl_->movies, collect_active_videos(job));
  DAILYBOY_ASSIGN_OR_RETURN(out.impl_->sequences,
                            collect_active_sequences(job));
  if (out.impl_->movies.empty() && out.impl_->sequences.empty()) {
    return Status::User(std::string(USER_ERROR_RENDER_3));
  }
  log_outputs(out.impl_->movies, out.impl_->sequences);
  return out;
}

Status Outputs::close() {
  if (!impl_ || impl_->closed) {
    return Status::Ok();
  }
  log_debug("Outputs::close");
  impl_->closed = true;
  if (impl_->movies.empty()) {
    log_debug("render: close_movies skip (none enabled)");
    return Status::Ok();
  }
  log_debug("render: close_movies");
  Status first_error = Status::Ok();
  for (ActiveVideo& active : impl_->movies) {
    const Status close_status = active.writer->close();
    if (!close_status.ok() && first_error.ok()) {
      first_error = close_status;
    }
  }
  return first_error;
}

Status write_movie(Outputs& out, const Frame& frame,
                   const JobOutputDisplayView& display_view) {
  return write_movies(out.impl_->movies, out.impl_->movies_open, frame,
                      &display_view, *out.impl_->job, out.impl_->audio_fps,
                      out.impl_->audio);
}

Status write_sequence(Outputs& out, const Frame& frame, int frame_number,
                      const JobOutputDisplayView& display_view) {
  return write_sequences(out.impl_->sequences, frame, frame_number,
                         &display_view);
}

Status write_outputs(Outputs& out, const Frame& frame, int frame_number,
                     const JobOutputDisplayView& display_view) {
  log_debug("render: write_outputs frame " + std::to_string(frame_number));
  DAILYBOY_RETURN_IF_ERROR(write_sequences(out.impl_->sequences, frame,
                                           frame_number, &display_view));
  return write_movies(out.impl_->movies, out.impl_->movies_open, frame,
                      &display_view, *out.impl_->job, out.impl_->audio_fps,
                      out.impl_->audio);
}

Status write_outputs(Outputs& out, const Frame& frame, int frame_number) {
  log_debug("render: write_outputs_all frame " + std::to_string(frame_number));
  DAILYBOY_RETURN_IF_ERROR(
      write_sequences(out.impl_->sequences, frame, frame_number, nullptr));
  return write_movies(out.impl_->movies, out.impl_->movies_open, frame, nullptr,
                      *out.impl_->job, out.impl_->audio_fps, out.impl_->audio);
}

}  // namespace dailyboy
