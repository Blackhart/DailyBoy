/*!
 * \file text.cpp
 * \brief Shared overlay text placement, measure, and draw.
 */

#include "image/text.hpp"

#include <OpenImageIO/imagebufalgo.h>
#include <OpenImageIO/imageio.h>

#include <filesystem>
#include <string>
#include <system_error>
#include <variant>

#include "error/process.hpp"
#include "status.hpp"

namespace dailyboy {

namespace {

constexpr int kLayoutInsetPx = 10;

enum class Horizontal { Left, Center, Right };
enum class Vertical { Top, Center, Bottom };

Status oiio_user_error(const OIIO::ImageBuf& buf) {
  std::string detail = buf.geterror();
  if (detail.empty()) {
    return Status::User(std::string(USER_ERROR_OVERLAY_4));
  }
  return Status::User(std::string(USER_ERROR_OVERLAY_4) + " " + detail);
}

Horizontal horizontal_from_anchor(TextPositionModeLayout::Anchor anchor) {
  switch (anchor) {
    case TextPositionModeLayout::Anchor::TopLeft:
    case TextPositionModeLayout::Anchor::CenterLeft:
    case TextPositionModeLayout::Anchor::BottomLeft:
      return Horizontal::Left;
    case TextPositionModeLayout::Anchor::TopRight:
    case TextPositionModeLayout::Anchor::CenterRight:
    case TextPositionModeLayout::Anchor::BottomRight:
      return Horizontal::Right;
    case TextPositionModeLayout::Anchor::TopCenter:
    case TextPositionModeLayout::Anchor::CenterCenter:
    case TextPositionModeLayout::Anchor::BottomCenter:
      return Horizontal::Center;
  }
  return Horizontal::Left;
}

Vertical vertical_from_anchor(TextPositionModeLayout::Anchor anchor) {
  switch (anchor) {
    case TextPositionModeLayout::Anchor::TopLeft:
    case TextPositionModeLayout::Anchor::TopCenter:
    case TextPositionModeLayout::Anchor::TopRight:
      return Vertical::Top;
    case TextPositionModeLayout::Anchor::BottomLeft:
    case TextPositionModeLayout::Anchor::BottomCenter:
    case TextPositionModeLayout::Anchor::BottomRight:
      return Vertical::Bottom;
    case TextPositionModeLayout::Anchor::CenterLeft:
    case TextPositionModeLayout::Anchor::CenterCenter:
    case TextPositionModeLayout::Anchor::CenterRight:
      return Vertical::Center;
  }
  return Vertical::Top;
}

int x_from_horizontal(Horizontal horizontal, int box_width, int canvas_width) {
  if (horizontal == Horizontal::Left) {
    return kLayoutInsetPx;
  }
  if (horizontal == Horizontal::Right) {
    return canvas_width - box_width - kLayoutInsetPx;
  }
  return (canvas_width - box_width) / 2;
}

int y_from_vertical(Vertical vertical, int box_height, int canvas_height) {
  if (vertical == Vertical::Top) {
    return kLayoutInsetPx;
  }
  if (vertical == Vertical::Bottom) {
    return canvas_height - box_height - kLayoutInsetPx;
  }
  return (canvas_height - box_height) / 2;
}

Point origin_from_layout(const TextPositionModeLayout& layout, const Size& box,
                         int canvas_width, int canvas_height) {
  const TextPositionModeLayout::Anchor anchor = layout.anchor();
  Point origin;
  origin.x = x_from_horizontal(horizontal_from_anchor(anchor), box.width,
                               canvas_width);
  origin.y =
      y_from_vertical(vertical_from_anchor(anchor), box.height, canvas_height);
  return origin;
}

Point origin_from_pixel(const TextPositionModePixel& pixel) {
  return {pixel.x(), pixel.y()};
}

Point origin_from_percent(const TextPositionModePercent& percent,
                          int canvas_width, int canvas_height) {
  return {static_cast<int>(percent.x() / 100.0 * canvas_width),
          static_cast<int>(percent.y() / 100.0 * canvas_height)};
}

}  // namespace

Point compute_box_position(const TextPosition& position, const Size& box,
                           int canvas_width, int canvas_height) {
  if (position.mode() == TextPosition::Mode::Layout) {
    return origin_from_layout(
        std::get<TextPositionModeLayout>(position.value()), box, canvas_width,
        canvas_height);
  }
  if (position.mode() == TextPosition::Mode::Pixel) {
    return origin_from_pixel(std::get<TextPositionModePixel>(position.value()));
  }
  return origin_from_percent(
      std::get<TextPositionModePercent>(position.value()), canvas_width,
      canvas_height);
}

StatusOr<Size> compute_text_size(const std::string& text,
                                 const TextFont& font) {
  std::error_code ec;
  if (!std::filesystem::is_regular_file(font.path(), ec) || ec) {
    return Status::User(std::string(USER_ERROR_OVERLAY_3) + " " +
                        font.path().string());
  }
  const OIIO::ROI roi =
      OIIO::ImageBufAlgo::text_size(text, font.size_px(), font.path().string());
  if (!roi.defined()) {
    std::string detail = OIIO::geterror();
    if (detail.empty()) {
      return Status::User(std::string(USER_ERROR_OVERLAY_4) +
                          " font measure failed.");
    }
    return Status::User(std::string(USER_ERROR_OVERLAY_4) + " " + detail);
  }
  return Size{roi.width(), roi.height()};
}

Status draw_text(OIIO::ImageBuf& canvas, const Point& origin,
                 const std::string& text, const TextFont& font) {
  const float color[3] = {static_cast<float>(font.color().r()),
                          static_cast<float>(font.color().g()),
                          static_cast<float>(font.color().b())};
  const OIIO::ROI roi(0, canvas.spec().width, 0, canvas.spec().height);
  if (!OIIO::ImageBufAlgo::render_text(
          canvas, origin.x, origin.y, text, font.size_px(),
          font.path().string(), OIIO::cspan<float>(color, 3),
          OIIO::ImageBufAlgo::TextAlignX::Left,
          OIIO::ImageBufAlgo::TextAlignY::Top, 0, roi)) {
    return oiio_user_error(canvas);
  }
  return Status::Ok();
}

}  // namespace dailyboy
