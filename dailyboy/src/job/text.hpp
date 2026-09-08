#pragma once

#include <cstdint>
#include <filesystem>
#include <utility>
#include <variant>

#include "job/primitives.hpp"

namespace dailyboy {

/*!
 * \brief Text position anchored to a layout corner or center.
 */
class TextPositionModeLayout {
 public:
  /*!
   * \brief Layout anchor (maps to \c layout.*.position.anchor in the job YAML).
   */
  enum class Anchor : std::uint8_t {
    TopLeft = 0,
    TopCenter = 1,
    TopRight = 2,
    CenterLeft = 3,
    CenterCenter = 4,
    CenterRight = 5,
    BottomLeft = 6,
    BottomCenter = 7,
    BottomRight = 8,
  };

  TextPositionModeLayout() = default;
  Anchor anchor() const { return anchor_; }
  void set_anchor(Anchor anchor) { anchor_ = anchor; }

 private:
  Anchor anchor_ = Anchor::TopLeft;
};

/*!
 * \brief Text position as absolute pixel coordinates from the canvas origin.
 */
class TextPositionModePixel {
 public:
  TextPositionModePixel() = default;
  int x() const { return x_; }
  void set_x(int x) { x_ = x; }

  int y() const { return y_; }
  void set_y(int y) { y_ = y; }

 private:
  int x_ = 0;
  int y_ = 0;
};

/*!
 * \brief Text position as percentage of canvas width and height.
 */
class TextPositionModePercent {
 public:
  TextPositionModePercent() = default;
  double x() const { return x_; }
  void set_x(double x) { x_ = x; }

  double y() const { return y_; }
  void set_y(double y) { y_ = y; }

 private:
  double x_ = 0.0;
  double y_ = 0.0;
};

/*!
 * \brief Discriminated position payload for \c TextPosition::value().
 */
using TextPositionMode =
    std::variant<TextPositionModeLayout, TextPositionModePixel,
                 TextPositionModePercent>;

/*!
 * \brief Text placement mode and coordinates (burn-ins, slate lines).
 */
class TextPosition {
 public:
  /*!
   * \brief Position mode (maps to \c position.mode in the job YAML).
   */
  enum class Mode : std::uint8_t {
    Layout = 0,
    Pixel = 1,
    Percent = 2,
  };

  TextPosition() = default;
  Mode mode() const { return mode_; }
  void set_mode(Mode mode) { mode_ = mode; }

  const TextPositionMode& value() const { return value_; }
  TextPositionMode& value() { return value_; }
  void set_value(TextPositionMode value) { value_ = value; }

 private:
  Mode mode_ = Mode::Layout;
  TextPositionMode value_;
};

/*!
 * \brief Font file path and size for overlay text.
 */
class TextFont {
 public:
  TextFont() = default;
  const std::filesystem::path& path() const { return path_; }
  void set_path(std::filesystem::path path) { path_ = std::move(path); }

  int size_px() const { return size_px_; }
  void set_size_px(int size_px) { size_px_ = size_px; }

  const RGBColor& color() const { return color_; }
  RGBColor& color() { return color_; }
  void set_color(RGBColor color) { color_ = color; }

 private:
  std::filesystem::path path_;
  int size_px_ = 0;
  RGBColor color_{1.0, 1.0, 1.0};
};

}  // namespace dailyboy
