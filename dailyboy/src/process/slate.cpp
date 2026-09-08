/*!
 * \file slate.cpp
 * \brief Draw layout.slate.lines on a background canvas (text only).
 */

#include "process/slate.hpp"

#include <dailyboy/log.hpp>
#include <exception>
#include <string>
#include <vector>

#include "error/process.hpp"
#include "image/text.hpp"
#include "log_internal.hpp"
#include "process/compositing.hpp"
#include "status.hpp"

namespace dailyboy {

namespace {

Status draw_one_line(Frame& canvas, const JobLayoutSlateLine& line,
                     const Job& job, const OverlayTokenContext& tokens) {
  const std::string text = expand_overlay_tokens(
      line.text(), tokens, job.metadata().substitutions());
  if (text.empty()) {
    log_debug("slate: skip, empty text");
    return Status::Ok();
  }
  DAILYBOY_ASSIGN_OR_RETURN(const Size text_size,
                            compute_text_size(text, line.font()));
  const Point origin = compute_box_position(line.position(), text_size,
                                            canvas.width(), canvas.height());
  return draw_text(canvas.buf(), origin, text, line.font());
}

OverlayTokenContext slate_tokens(const Job& job) {
  OverlayTokenContext tokens;
  const JobSequence& sequence = job.plans().plans().front().sequence();
  tokens.frame = sequence.frame_start();
  tokens.frame_start = sequence.frame_start();
  tokens.frame_end = sequence.frame_end();
  tokens.plan_id = job.plans().plans().front().id();
  return tokens;
}

Status write_slate_copies(Outputs& out, const Frame& canvas, const Job& job,
                          int duration) {
  const int start =
      job.plans().plans().front().sequence().frame_start() - duration;
  for (int i = 0; i < duration; ++i) {
    const int frame = start + i;
    log_debug_banner("slate frame " + std::to_string(frame));
    DAILYBOY_RETURN_IF_ERROR(write_outputs(out, canvas, frame));
  }
  return Status::Ok();
}

StatusOr<Frame> make_slate_canvas(const Job& job) {
  DAILYBOY_ASSIGN_OR_RETURN(Frame canvas, make_filled_canvas(job));
  DAILYBOY_RETURN_IF_ERROR(draw_slate_texts(canvas, job, slate_tokens(job)));
  return canvas;
}

}  // namespace

Status draw_slate_texts(Frame& canvas, const Job& job,
                        const OverlayTokenContext& tokens) {
  try {
    const std::vector<JobLayoutSlateLine>& lines = job.layout().slate().lines();
    log_debug("draw_slate_texts: " + std::to_string(lines.size()) + " line(s)");
    for (const JobLayoutSlateLine& line : lines) {
      DAILYBOY_RETURN_IF_ERROR(draw_one_line(canvas, line, job, tokens));
    }
    return Status::Ok();
  } catch (const std::exception& ex) {
    return Status::User(std::string(USER_ERROR_OVERLAY_4) + " " + ex.what());
  }
}

Status write_slates(const Job& job, Outputs& out) {
  const int duration = job.layout().slate().duration_frames();
  if (duration <= 0) {
    log_debug("write_slates: skip (duration 0)");
    return Status::Ok();
  }
  if (job.plans().plans().empty()) {
    return Status::User(std::string(USER_ERROR_RENDER_1));
  }
  log_debug("write_slates: duration " + std::to_string(duration));
  DAILYBOY_ASSIGN_OR_RETURN(Frame canvas, make_slate_canvas(job));
  return write_slate_copies(out, canvas, job, duration);
}

}  // namespace dailyboy
