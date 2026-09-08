#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "job/primitives.hpp"
#include "job/text.hpp"

namespace dailyboy {

/*!
 * \brief Output canvas dimensions in pixels.
 */
class JobLayoutCanvas {
 public:
  JobLayoutCanvas() = default;
  int width() const { return width_; }
  void set_width(int width) { width_ = width; }

  int height() const { return height_; }
  void set_height(int height) { height_ = height; }

 private:
  int width_ = 0;
  int height_ = 0;
};

/*!
 * \brief Pixel aspect ratio of the output canvas.
 */
class JobLayoutPixelAspect {
 public:
  JobLayoutPixelAspect() = default;
  double aspect() const { return aspect_; }
  void set_aspect(double aspect) { aspect_ = aspect; }

 private:
  double aspect_ = 1.0;
};

/*!
 * \brief Source image fit and resampling filter on the canvas.
 */
class JobLayoutImage {
 public:
  /*!
   * \brief How the source image is scaled within the canvas.
   */
  enum class JobLayoutImageFitValue : std::uint8_t {
    Contain = 0,
    Cover = 1,
  };

  /*!
   * \brief Resampling filter applied when scaling the source image.
   */
  enum class JobLayoutImageFilterValue : std::uint8_t {
    Bilinear = 0,
    Lanczos3 = 1,
  };

  JobLayoutImage() = default;
  JobLayoutImageFitValue fit() const { return fit_; }
  void set_fit(JobLayoutImageFitValue fit) { fit_ = fit; }

  JobLayoutImageFilterValue filter() const { return filter_; }
  void set_filter(JobLayoutImageFilterValue filter) { filter_ = filter; }

  const Margin& min_margin_px() const { return min_margin_px_; }
  Margin& min_margin_px() { return min_margin_px_; }
  void set_min_margin_px(Margin min_margin_px) {
    min_margin_px_ = min_margin_px;
  }

 private:
  JobLayoutImageFitValue fit_ = JobLayoutImageFitValue::Contain;
  JobLayoutImageFilterValue filter_ = JobLayoutImageFilterValue::Lanczos3;
  Margin min_margin_px_;
};

using JobLayoutBackground = RGBColor;

using JobLayoutBurnInPositionModeLayout = TextPositionModeLayout;
using JobLayoutBurnInPositionModePixel = TextPositionModePixel;
using JobLayoutBurnInPositionModePercent = TextPositionModePercent;
using JobLayoutBurnInPositionMode = TextPositionMode;
using JobLayoutBurnInPosition = TextPosition;
using JobLayoutBurnInFont = TextFont;

using JobLayoutBurnInBoxColor = RGBColor;

using JobLayoutBurnInBoxMargin = Margin;

/*!
 * \brief Background box behind a burn-in text line (fill or outline).
 */
class JobLayoutBurnInBox {
 public:
  /*!
   * \brief Box rendering mode (maps to \c box.mode in the job YAML).
   */
  enum class JobLayoutBurnInBoxModeValue : std::uint8_t {
    Fill = 0,
    Outline = 1,
  };

  JobLayoutBurnInBox() = default;
  JobLayoutBurnInBoxModeValue mode() const { return mode_; }
  void set_mode(JobLayoutBurnInBoxModeValue mode) { mode_ = mode; }

  const JobLayoutBurnInBoxColor& color() const { return color_; }
  JobLayoutBurnInBoxColor& color() { return color_; }
  void set_color(JobLayoutBurnInBoxColor color) { color_ = color; }

  double opacity() const { return opacity_; }
  void set_opacity(double opacity) { opacity_ = opacity; }

  const JobLayoutBurnInBoxMargin& margin() const { return margin_; }
  JobLayoutBurnInBoxMargin& margin() { return margin_; }
  void set_margin(JobLayoutBurnInBoxMargin margin) { margin_ = margin; }

 private:
  JobLayoutBurnInBoxModeValue mode_ = JobLayoutBurnInBoxModeValue::Fill;
  JobLayoutBurnInBoxColor color_;
  double opacity_ = 1.0;
  JobLayoutBurnInBoxMargin margin_;
};

/*!
 * \brief One burn-in overlay (template, position, font, optional box).
 */
class JobLayoutBurnIn {
 public:
  JobLayoutBurnIn() = default;
  const std::string& template_text() const { return template_; }
  void set_template_text(std::string template_text) {
    template_ = std::move(template_text);
  }

  const JobLayoutBurnInPosition& position() const { return position_; }
  JobLayoutBurnInPosition& position() { return position_; }
  void set_position(JobLayoutBurnInPosition position) { position_ = position; }

  const JobLayoutBurnInFont& font() const { return font_; }
  JobLayoutBurnInFont& font() { return font_; }
  void set_font(JobLayoutBurnInFont font) { font_ = std::move(font); }

  const std::optional<JobLayoutBurnInBox>& box() const { return box_; }
  std::optional<JobLayoutBurnInBox>& box() { return box_; }
  void set_box(std::optional<JobLayoutBurnInBox> box) { box_ = box; }

 private:
  std::string template_;
  JobLayoutBurnInPosition position_;
  JobLayoutBurnInFont font_;
  std::optional<JobLayoutBurnInBox> box_;
};

/*!
 * \brief Ordered list of burn-in overlays (\c layout.burn_ins in the job YAML).
 */
class JobLayoutBurnIns {
 public:
  JobLayoutBurnIns() = default;
  const std::vector<JobLayoutBurnIn>& burn_ins() const { return burn_ins_; }
  std::vector<JobLayoutBurnIn>& burn_ins() { return burn_ins_; }
  void set_burn_ins(std::vector<JobLayoutBurnIn> burn_ins) {
    burn_ins_ = std::move(burn_ins);
  }

 private:
  std::vector<JobLayoutBurnIn> burn_ins_;
};

/*!
 * \brief One line of text on the optional head slate.
 */
class JobLayoutSlateLine {
 public:
  JobLayoutSlateLine() = default;
  const std::string& text() const { return text_; }
  void set_text(std::string text) { text_ = std::move(text); }

  const JobLayoutBurnInPosition& position() const { return position_; }
  JobLayoutBurnInPosition& position() { return position_; }
  void set_position(JobLayoutBurnInPosition position) { position_ = position; }

  const JobLayoutBurnInFont& font() const { return font_; }
  JobLayoutBurnInFont& font() { return font_; }
  void set_font(JobLayoutBurnInFont font) { font_ = std::move(font); }

 private:
  std::string text_;
  JobLayoutBurnInPosition position_;
  JobLayoutBurnInFont font_;
};

/*!
 * \brief Optional head slate prepended to enabled movies and image
 *        sequences (\c layout.slate).
 */
class JobLayoutSlate {
 public:
  JobLayoutSlate() = default;
  int duration_frames() const { return duration_frames_; }
  void set_duration_frames(int duration_frames) {
    duration_frames_ = duration_frames;
  }

  const std::vector<JobLayoutSlateLine>& lines() const { return lines_; }
  std::vector<JobLayoutSlateLine>& lines() { return lines_; }
  void set_lines(std::vector<JobLayoutSlateLine> lines) {
    lines_ = std::move(lines);
  }

 private:
  int duration_frames_ = 0;
  std::vector<JobLayoutSlateLine> lines_;
};

/*!
 * \brief Compositing layout section of the job (\c layout in the job YAML).
 */
class JobLayout {
 public:
  JobLayout() = default;
  const JobLayoutCanvas& canvas() const { return canvas_; }
  JobLayoutCanvas& canvas() { return canvas_; }
  void set_canvas(JobLayoutCanvas canvas) { canvas_ = canvas; }

  const JobLayoutPixelAspect& pixel_aspect() const { return pixel_aspect_; }
  JobLayoutPixelAspect& pixel_aspect() { return pixel_aspect_; }
  void set_pixel_aspect(JobLayoutPixelAspect pixel_aspect) {
    pixel_aspect_ = pixel_aspect;
  }

  const JobLayoutImage& image() const { return image_; }
  JobLayoutImage& image() { return image_; }
  void set_image(JobLayoutImage image) { image_ = image; }

  const JobLayoutBackground& background() const { return background_; }
  JobLayoutBackground& background() { return background_; }
  void set_background(JobLayoutBackground background) {
    background_ = background;
  }

  const JobLayoutBurnIns& burn_ins() const { return burn_ins_; }
  JobLayoutBurnIns& burn_ins() { return burn_ins_; }
  void set_burn_ins(JobLayoutBurnIns burn_ins) {
    burn_ins_ = std::move(burn_ins);
  }

  const JobLayoutSlate& slate() const { return slate_; }
  JobLayoutSlate& slate() { return slate_; }
  void set_slate(JobLayoutSlate slate) { slate_ = std::move(slate); }

 private:
  JobLayoutCanvas canvas_;
  JobLayoutPixelAspect pixel_aspect_;
  JobLayoutImage image_;
  JobLayoutBackground background_;
  JobLayoutBurnIns burn_ins_;
  JobLayoutSlate slate_;
};

}  // namespace dailyboy
