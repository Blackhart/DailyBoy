/*!
 * \file buffer.cpp
 * \brief Shared ImageBuf fill, channel, paste, and resize helpers.
 */

#include "image/buffer.hpp"

#include <OpenImageIO/imagebufalgo.h>
#include <OpenImageIO/imageio.h>

#include <algorithm>
#include <array>
#include <string>

#include "error/image.hpp"
#include "error/process.hpp"
#include "status.hpp"

namespace dailyboy {

namespace {

Status oiio_error(const OIIO::ImageBuf& buf, std::string_view prefix) {
  std::string detail = buf.geterror();
  if (detail.empty()) {
    return Status::User(std::string(prefix));
  }
  return Status::User(std::string(prefix) + " " + detail);
}

struct DisplayWindow {
  int x = 0;
  int y = 0;
  int width = 0;
  int height = 0;
};

DisplayWindow display_window(const OIIO::ImageSpec& spec) {
  if (spec.full_width <= 0 || spec.full_height <= 0) {
    return {spec.x, spec.y, spec.width, spec.height};
  }
  return {spec.full_x, spec.full_y, spec.full_width, spec.full_height};
}

std::array<int, 3> rgb_channel_order(const OIIO::ImageSpec& spec) {
  const int r = spec.channelindex("R");
  const int g = spec.channelindex("G");
  const int b = spec.channelindex("B");
  if (r >= 0 && g >= 0 && b >= 0) {
    return {r, g, b};
  }
  return {spec.nchannels > 0 ? 0 : -1, spec.nchannels > 1 ? 1 : -1,
          spec.nchannels > 2 ? 2 : -1};
}

}  // namespace

Status fill_background(OIIO::ImageBuf& canvas, const RGBColor& bg) {
  const int nchannels = canvas.spec().nchannels;
  float values[3] = {static_cast<float>(bg.r()), static_cast<float>(bg.g()),
                     static_cast<float>(bg.b())};
  if (!OIIO::ImageBufAlgo::fill(
          canvas, OIIO::cspan<float>(values, std::min(nchannels, 3)))) {
    return oiio_error(canvas, USER_ERROR_OVERLAY_2);
  }
  return Status::Ok();
}

Status drop_extra_channels(Frame& frame) {
  const OIIO::ImageSpec& spec = frame.buf().spec();
  if (spec.nchannels <= 3) {
    return Status::Ok();
  }
  const std::array<int, 3> order_arr = rgb_channel_order(spec);
  const int order[3] = {order_arr[0], order_arr[1], order_arr[2]};
  const float fill[3] = {0.0f, 0.0f, 0.0f};
  OIIO::ImageBuf rgb;
  if (!OIIO::ImageBufAlgo::channels(rgb, frame.buf(), 3, order, fill)) {
    return oiio_error(rgb, USER_ERROR_IO_9);
  }
  frame.buf() = std::move(rgb);
  return Status::Ok();
}

Status paste_to_display_window(Frame& frame) {
  const OIIO::ImageSpec& spec = frame.buf().spec();
  const DisplayWindow display = display_window(spec);
  if (display.width <= 0 || display.height <= 0) {
    return Status::User(std::string(USER_ERROR_IO_9) +
                        " invalid display size.");
  }
  if (spec.x == 0 && spec.y == 0 && spec.width == display.width &&
      spec.height == display.height) {
    return Status::Ok();
  }
  OIIO::ImageSpec canvas_spec(display.width, display.height, spec.nchannels,
                              spec.format);
  OIIO::ImageBuf canvas(canvas_spec);
  if (!OIIO::ImageBufAlgo::zero(canvas)) {
    return oiio_error(canvas, USER_ERROR_IO_9);
  }
  if (!OIIO::ImageBufAlgo::paste(canvas, -display.x, -display.y, 0, 0,
                                 frame.buf())) {
    return oiio_error(canvas, USER_ERROR_IO_9);
  }
  frame.buf() = std::move(canvas);
  return Status::Ok();
}

Status paste_into_rect(OIIO::ImageBuf& canvas, const OIIO::ImageBuf& plate,
                       const Point& origin, const Rect& image_frame) {
  const OIIO::ImageSpec& spec = plate.spec();
  const int x0 = std::max(origin.x, image_frame.x);
  const int y0 = std::max(origin.y, image_frame.y);
  const int x1 =
      std::min(origin.x + spec.width, image_frame.x + image_frame.width);
  const int y1 =
      std::min(origin.y + spec.height, image_frame.y + image_frame.height);
  if (x1 <= x0 || y1 <= y0) {
    return Status::Ok();
  }
  const OIIO::ROI cut_roi(spec.x + (x0 - origin.x), spec.x + (x1 - origin.x),
                          spec.y + (y0 - origin.y), spec.y + (y1 - origin.y));
  OIIO::ImageBuf piece;
  if (!OIIO::ImageBufAlgo::cut(piece, plate, cut_roi)) {
    return oiio_error(piece, USER_ERROR_OVERLAY_2);
  }
  if (!OIIO::ImageBufAlgo::paste(canvas, x0, y0, 0, 0, piece)) {
    return oiio_error(canvas, USER_ERROR_OVERLAY_2);
  }
  return Status::Ok();
}

Status resize_buffer(OIIO::ImageBuf& scaled, const OIIO::ImageBuf& src,
                     const Size& fitted, const char* filter) {
  OIIO::ImageSpec spec(fitted.width, fitted.height, src.spec().nchannels,
                       src.spec().format);
  scaled.reset(spec);
  if (!OIIO::ImageBufAlgo::resize(
          scaled, src, {{"filtername", filter}},
          OIIO::ROI(0, fitted.width, 0, fitted.height))) {
    return oiio_error(scaled, USER_ERROR_OVERLAY_2);
  }
  return Status::Ok();
}

}  // namespace dailyboy
