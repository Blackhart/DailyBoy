/*!
 * \file compositing.cpp
 * \brief Canvas, plate fit/paste, HUD burn-ins, and compose.
 */

#include "process/compositing.hpp"

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>
#include <OpenImageIO/imageio.h>

#include <algorithm>
#include <cmath>
#include <dailyboy/log.hpp>
#include <exception>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "error/process.hpp"
#include "image/buffer.hpp"
#include "image/frame.hpp"
#include "image/geom.hpp"
#include "image/text.hpp"
#include "job/layout.hpp"
#include "job/primitives.hpp"
#include "status.hpp"

namespace dailyboy {

namespace {

std::string size_text(const Size& size) {
  return std::to_string(size.width) + "x" + std::to_string(size.height);
}

std::string point_text(const Point& point) {
  return "(" + std::to_string(point.x) + "," + std::to_string(point.y) + ")";
}

std::string margin_text(const Margin& margin) {
  return std::to_string(margin.top()) + "/" + std::to_string(margin.right()) +
         "/" + std::to_string(margin.bottom()) + "/" +
         std::to_string(margin.left());
}

const char* fit_text(JobLayoutImage::JobLayoutImageFitValue fit) {
  if (fit == JobLayoutImage::JobLayoutImageFitValue::Cover) {
    return "cover";
  }
  return "contain";
}

const char* winning_axis(double scale_x, double scale_y, bool cover) {
  return (cover ? scale_x >= scale_y : scale_x <= scale_y) ? "width" : "height";
}

const char* filter_name(JobLayoutImage::JobLayoutImageFilterValue filter) {
  if (filter == JobLayoutImage::JobLayoutImageFilterValue::Bilinear) {
    return "triangle";
  }
  return "lanczos3";
}

Size clamp_fitted_to_frame(Size fitted, const Rect& image_frame,
                           JobLayoutImage::JobLayoutImageFitValue fit) {
  if (fit == JobLayoutImage::JobLayoutImageFitValue::Contain) {
    fitted.width = std::min(fitted.width, image_frame.width);
    fitted.height = std::min(fitted.height, image_frame.height);
  }
  return fitted;
}

Size stored_pixels_after_scale(const Size& plate, double scale,
                               double pixel_aspect, const Rect& image_frame,
                               JobLayoutImage::JobLayoutImageFitValue fit) {
  Size fitted;
  fitted.width = static_cast<int>(
      std::lround(static_cast<double>(plate.width) * scale / pixel_aspect));
  fitted.height =
      static_cast<int>(std::lround(static_cast<double>(plate.height) * scale));
  fitted.width = std::max(fitted.width, 1);
  fitted.height = std::max(fitted.height, 1);
  return clamp_fitted_to_frame(fitted, image_frame, fit);
}

std::string resize_debug_message(const JobLayout& layout, const Size& fitted,
                                 double sx, double sy, double par,
                                 JobLayoutImage::JobLayoutImageFitValue fit,
                                 bool cover) {
  return "resize: target " +
         size_text({layout.canvas().width(), layout.canvas().height()}) +
         " fit=" + fit_text(fit) +
         " margin=" + margin_text(layout.image().min_margin_px()) +
         " sx=" + std::to_string(sx) + " sy=" + std::to_string(sy) +
         " win=" + winning_axis(sx, sy, cover) + " par=" + std::to_string(par) +
         " -> plate " + size_text(fitted);
}

void log_resize_plate(const JobLayout& layout, const Size& src,
                      const Rect& frame, const Size& fitted) {
  const double par = layout.pixel_aspect().aspect();
  const auto fit = layout.image().fit();
  const double sx =
      (static_cast<double>(frame.width) * par) / static_cast<double>(src.width);
  const double sy =
      static_cast<double>(frame.height) / static_cast<double>(src.height);
  const bool cover = fit == JobLayoutImage::JobLayoutImageFitValue::Cover;
  log_debug(resize_debug_message(layout, fitted, sx, sy, par, fit, cover));
}

Point center_in_frame(const Size& fitted, const Rect& image_frame) {
  return {image_frame.x + (image_frame.width - fitted.width) / 2,
          image_frame.y + (image_frame.height - fitted.height) / 2};
}

StatusOr<Frame> resize_plate_buffer(const Frame& plate, const Size& fitted,
                                    const JobLayoutImage& image) {
  const OIIO::ImageSpec& src_spec = plate.buf().spec();
  if (fitted.width == src_spec.width && fitted.height == src_spec.height) {
    return Frame(plate.buf().copy(OIIO::TypeDesc::UNKNOWN));
  }
  OIIO::ImageBuf scaled;
  DAILYBOY_RETURN_IF_ERROR(
      resize_buffer(scaled, plate.buf(), fitted, filter_name(image.filter())));
  return Frame(std::move(scaled));
}

StatusOr<Rect> image_frame_from_margins(int width, int height,
                                        const Margin& margin) {
  Rect image_frame;
  image_frame.x = margin.left();
  image_frame.y = margin.top();
  image_frame.width = width - margin.left() - margin.right();
  image_frame.height = height - margin.top() - margin.bottom();
  if (image_frame.width < 1 || image_frame.height < 1) {
    return Status::User(std::string(USER_ERROR_OVERLAY_1));
  }
  return image_frame;
}

StatusOr<double> scale_axes(const Size& plate, const Rect& image_frame,
                            double pixel_aspect,
                            JobLayoutImage::JobLayoutImageFitValue fit) {
  const double scale_x =
      (static_cast<double>(image_frame.width) * pixel_aspect) /
      static_cast<double>(plate.width);
  const double scale_y = static_cast<double>(image_frame.height) /
                         static_cast<double>(plate.height);
  if (fit == JobLayoutImage::JobLayoutImageFitValue::Cover) {
    return std::max(scale_x, scale_y);
  }
  return std::min(scale_x, scale_y);
}

Status oiio_user_error(const OIIO::ImageBuf& buf) {
  std::string detail = buf.geterror();
  if (detail.empty()) {
    return Status::User(std::string(USER_ERROR_OVERLAY_4));
  }
  return Status::User(std::string(USER_ERROR_OVERLAY_4) + " " + detail);
}

Margin box_margin(const JobLayoutBurnIn& burn_in) {
  const std::optional<JobLayoutBurnInBox>& box = burn_in.box();
  if (!box) {
    return {};
  }
  return box->margin();
}

Size compute_box_size(const Size& text, const Margin& margin) {
  return {text.width + margin.left() + margin.right(),
          text.height + margin.top() + margin.bottom()};
}

Point compute_text_position(const Point& box_origin, const Margin& margin) {
  return {box_origin.x + margin.left(), box_origin.y + margin.top()};
}

Status draw_box(OIIO::ImageBuf& canvas, const Rect& box,
                const std::optional<JobLayoutBurnInBox>& style) {
  if (!style || style->opacity() <= 0.0 || box.width <= 0 || box.height <= 0) {
    return Status::Ok();
  }
  const float a = static_cast<float>(style->opacity());
  const float color[4] = {static_cast<float>(style->color().r()) * a,
                          static_cast<float>(style->color().g()) * a,
                          static_cast<float>(style->color().b()) * a, a};
  const bool fill =
      style->mode() != JobLayoutBurnInBox::JobLayoutBurnInBoxModeValue::Outline;
  if (!OIIO::ImageBufAlgo::render_box(
          canvas, box.x, box.y, box.x + box.width - 1, box.y + box.height - 1,
          OIIO::cspan<float>(color, 4), fill)) {
    return oiio_user_error(canvas);
  }
  return Status::Ok();
}

Status draw_one_burn_in(Frame& frame, const JobLayoutBurnIn& burn_in,
                        const Job& job, const OverlayTokenContext& tokens) {
  const std::string text = expand_overlay_tokens(
      burn_in.template_text(), tokens, job.metadata().substitutions());
  if (text.empty()) {
    return Status::Ok();
  }
  DAILYBOY_ASSIGN_OR_RETURN(const Size text_size,
                            compute_text_size(text, burn_in.font()));
  const Margin margin = box_margin(burn_in);
  const Size box_size = compute_box_size(text_size, margin);
  const Point box_origin = compute_box_position(burn_in.position(), box_size,
                                                frame.width(), frame.height());
  const Rect box{box_origin.x, box_origin.y, box_size.width, box_size.height};
  DAILYBOY_RETURN_IF_ERROR(draw_box(frame.buf(), box, burn_in.box()));
  return draw_text(frame.buf(), compute_text_position(box_origin, margin), text,
                   burn_in.font());
}

Rect paste_clip_rect(const FittedPlate& plate) {
  const OIIO::ImageSpec& spec = plate.frame.buf().spec();
  const int x0 = std::max(plate.origin.x, plate.image_frame.x);
  const int y0 = std::max(plate.origin.y, plate.image_frame.y);
  const int x1 = std::min(plate.origin.x + spec.width,
                          plate.image_frame.x + plate.image_frame.width);
  const int y1 = std::min(plate.origin.y + spec.height,
                          plate.image_frame.y + plate.image_frame.height);
  return {x0, y0, std::max(0, x1 - x0), std::max(0, y1 - y0)};
}

void log_paste_clip(const Rect& clip) {
  if (clip.width > 0 && clip.height > 0) {
    log_debug("compose: pasted " + size_text({clip.width, clip.height}) +
              " at " + point_text({clip.x, clip.y}));
    return;
  }
  log_debug("compose: paste skipped (empty clip)");
}

}  // namespace

StatusOr<Frame> make_canvas(const Job& job) {
  const int width = job.layout().canvas().width();
  const int height = job.layout().canvas().height();
  if (width < 1 || height < 1) {
    return Status::User(std::string(USER_ERROR_OVERLAY_2) +
                        " invalid canvas size.");
  }
  OIIO::ImageSpec spec(width, height, 3, OIIO::TypeDesc::FLOAT);
  log_debug("make_canvas: " + std::to_string(width) + "x" +
            std::to_string(height));
  return Frame(OIIO::ImageBuf(spec));
}

StatusOr<Frame> make_filled_canvas(const Job& job) {
  DAILYBOY_ASSIGN_OR_RETURN(Frame canvas, make_canvas(job));
  const RGBColor& bg = job.layout().background();
  DAILYBOY_RETURN_IF_ERROR(fill_background(canvas.buf(), bg));
  log_debug("compose: background r=" + std::to_string(bg.r()) +
            " g=" + std::to_string(bg.g()) + " b=" + std::to_string(bg.b()));
  return canvas;
}

StatusOr<Rect> usable_image_frame(const Job& job) {
  const JobLayoutCanvas& canvas = job.layout().canvas();
  return image_frame_from_margins(canvas.width(), canvas.height(),
                                  job.layout().image().min_margin_px());
}

StatusOr<FittedPlate> resize_plate(const Frame& plate, const Job& job) {
  const JobLayout& layout = job.layout();
  const Size src{plate.width(), plate.height()};
  DAILYBOY_ASSIGN_OR_RETURN(const Rect frame, usable_image_frame(job));
  DAILYBOY_ASSIGN_OR_RETURN(
      const double scale,
      scale_to_fit(src, frame, layout.pixel_aspect().aspect(),
                   layout.image().fit()));
  const Size fitted = stored_pixels_after_scale(
      src, scale, layout.pixel_aspect().aspect(), frame, layout.image().fit());
  log_resize_plate(layout, src, frame, fitted);
  DAILYBOY_ASSIGN_OR_RETURN(Frame scaled,
                            resize_plate_buffer(plate, fitted, layout.image()));
  return FittedPlate{std::move(scaled), center_in_frame(fitted, frame), frame};
}

Status paste_plate_into_canvas(Frame& canvas, const FittedPlate& plate) {
  DAILYBOY_RETURN_IF_ERROR(paste_into_rect(canvas.buf(), plate.frame.buf(),
                                           plate.origin, plate.image_frame));
  log_paste_clip(paste_clip_rect(plate));
  return Status::Ok();
}

Status draw_hud(Frame& frame, const Job& job,
                const OverlayTokenContext& tokens) {
  try {
    const std::vector<JobLayoutBurnIn>& list =
        job.layout().burn_ins().burn_ins();
    log_debug("draw_hud: " + std::to_string(list.size()) + " item(s)");
    for (const JobLayoutBurnIn& burn_in : list) {
      DAILYBOY_RETURN_IF_ERROR(draw_one_burn_in(frame, burn_in, job, tokens));
    }
    return Status::Ok();
  } catch (const std::exception& ex) {
    return Status::User(std::string(USER_ERROR_OVERLAY_4) + " " + ex.what());
  }
}

StatusOr<Frame> compose(Frame canvas, const FittedPlate& plate, const Job& job,
                        const OverlayTokenContext& tokens) {
  try {
    log_debug("compose: paste plate " +
              size_text({plate.frame.width(), plate.frame.height()}) + " at " +
              point_text(plate.origin));
    DAILYBOY_RETURN_IF_ERROR(paste_plate_into_canvas(canvas, plate));
    DAILYBOY_RETURN_IF_ERROR(draw_hud(canvas, job, tokens));
    return canvas;
  } catch (const std::exception& ex) {
    return Status::User(std::string(USER_ERROR_OVERLAY_2) + " " + ex.what());
  }
}

StatusOr<double> scale_to_fit(const Size& plate, const Rect& image_frame,
                              double pixel_aspect,
                              JobLayoutImage::JobLayoutImageFitValue fit) {
  if (plate.width < 1 || plate.height < 1) {
    return Status::User(std::string(USER_ERROR_OVERLAY_2) +
                        " invalid source size.");
  }
  if (pixel_aspect <= 0.0) {
    return Status::User(std::string(USER_ERROR_OVERLAY_2) +
                        " pixel_aspect must be greater than 0.");
  }
  return scale_axes(plate, image_frame, pixel_aspect, fit);
}

}  // namespace dailyboy
