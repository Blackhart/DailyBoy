/*!
 * \file burn_ins.cpp
 * \brief Prepare plates per display_view, compose, and write outputs.
 */

#include "process/burn_ins.hpp"

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imageio.h>

#include <dailyboy/log.hpp>
#include <map>
#include <string>
#include <vector>

#include "error/process.hpp"
#include "log_internal.hpp"
#include "process/colorimetry.hpp"
#include "process/compositing.hpp"
#include "process/reformat.hpp"
#include "status.hpp"

namespace dailyboy {

namespace {

OverlayTokenContext plate_tokens(const Sequence::Iterator& it, const Job& job) {
  OverlayTokenContext tokens;
  const JobSequence& sequence = job.plans().plans().front().sequence();
  tokens.frame = it.frame();
  tokens.frame_start = sequence.frame_start();
  tokens.frame_end = sequence.frame_end();
  tokens.source_file = it.path();
  tokens.plan_id = job.plans().plans().front().id();
  return tokens;
}

std::vector<JobOutputDisplayView> unique_enabled_display_views(const Job& job) {
  std::map<std::string, JobOutputDisplayView> views;
  for (const auto& video : job.output().videos().videos()) {
    if (video.enabled()) {
      views.emplace(display_view_key(video.display_view()),
                    video.display_view());
    }
  }
  for (const auto& item : job.output().image_sequences().image_sequences()) {
    if (item.enabled()) {
      views.emplace(display_view_key(item.display_view()), item.display_view());
    }
  }
  std::vector<JobOutputDisplayView> result;
  result.reserve(views.size());
  for (auto& [_, display_view] : views) {
    result.push_back(std::move(display_view));
  }
  return result;
}

StatusOr<Frame> load_working_plate(const Sequence::Iterator& it,
                                   const ColorPipeline& color_pipeline) {
  DAILYBOY_ASSIGN_OR_RETURN(Frame plate, load_plate(it));
  DAILYBOY_RETURN_IF_ERROR(reformat(plate));
  DAILYBOY_RETURN_IF_ERROR(
      convert_color_from_input_to_working(plate, color_pipeline));
  return plate;
}

StatusOr<FittedPlate> display_fitted_plate(
    const Frame& working_plate, const Job& job,
    const ColorPipeline& color_pipeline,
    const JobOutputDisplayView& display_view) {
  Frame copy(working_plate.buf().copy(OIIO::TypeDesc::UNKNOWN));
  DAILYBOY_ASSIGN_OR_RETURN(FittedPlate fitted, resize_plate(copy, job));
  DAILYBOY_RETURN_IF_ERROR(convert_color_from_working_to_display(
      fitted.frame, color_pipeline, display_view));
  return fitted;
}

Status write_plate_for_display(const FittedPlate& plate, const Job& job,
                               Outputs& out,
                               const JobOutputDisplayView& display_view,
                               const OverlayTokenContext& tokens,
                               int frame_number) {
  DAILYBOY_ASSIGN_OR_RETURN(Frame canvas, make_filled_canvas(job));
  DAILYBOY_ASSIGN_OR_RETURN(canvas,
                            compose(std::move(canvas), plate, job, tokens));
  return write_outputs(out, canvas, frame_number, display_view);
}

Status write_plate(const Sequence::Iterator& it, const Job& job, Outputs& out,
                   const ColorPipeline& color_pipeline) {
  log_debug_banner("burn-in frame " + std::to_string(it.frame()));
  DAILYBOY_ASSIGN_OR_RETURN(Frame working,
                            load_working_plate(it, color_pipeline));
  const OverlayTokenContext tokens = plate_tokens(it, job);
  for (const JobOutputDisplayView& display_view :
       unique_enabled_display_views(job)) {
    DAILYBOY_ASSIGN_OR_RETURN(
        FittedPlate plate,
        display_fitted_plate(working, job, color_pipeline, display_view));
    DAILYBOY_RETURN_IF_ERROR(write_plate_for_display(
        plate, job, out, display_view, tokens, it.frame()));
  }
  return Status::Ok();
}

Status write_plates(const Job& job, const Sequence& sequence, Outputs& out,
                    const ColorPipeline& color_pipeline) {
  bool any_frame = false;
  for (auto it = sequence.begin(); it != sequence.end(); ++it) {
    DAILYBOY_RETURN_IF_ERROR(write_plate(it, job, out, color_pipeline));
    any_frame = true;
  }
  if (!any_frame) {
    return Status::User(std::string(USER_ERROR_RENDER_1));
  }
  return Status::Ok();
}

}  // namespace

StatusOr<Frame> load_plate(const Sequence::Iterator& it) {
  log_debug("load_plate: frame " + std::to_string(it.frame()) + " " +
            it.path().string());
  return it.load();
}

StatusOr<FittedPlate> prepare_plate(const Sequence::Iterator& it,
                                    const Job& job,
                                    const ColorPipeline& color_pipeline,
                                    const JobOutputDisplayView& display_view) {
  DAILYBOY_ASSIGN_OR_RETURN(Frame working,
                            load_working_plate(it, color_pipeline));
  return display_fitted_plate(working, job, color_pipeline, display_view);
}

Status write_burnins(const Job& job, Outputs& out,
                     const ColorPipeline& color_pipeline) {
  if (job.plans().plans().empty()) {
    return Status::User(std::string(USER_ERROR_RENDER_1));
  }
  const JobPlan& plan = job.plans().plans().front();
  log_debug("write_burnins: plan " + plan.id() + " " + plan.sequence().path());
  DAILYBOY_ASSIGN_OR_RETURN(
      Sequence sequence,
      Sequence::open(plan.sequence(), job.metadata().substitutions()));
  log_info("render: plan " + plan.id() + " frames " +
           std::to_string(sequence.frame_start()) + "-" +
           std::to_string(sequence.frame_end()));
  return write_plates(job, sequence, out, color_pipeline);
}

}  // namespace dailyboy
