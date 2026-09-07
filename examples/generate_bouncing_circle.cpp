/*!
 * \file generate_bouncing_circle.cpp
 * \brief Writes cropped EXR frames of a blue circle bouncing in 1920x1080.
 *
 * Each file's data window is the circle bbox; the display window stays
 * full HD. Rebuild and run:
 *
 * \code
 * cmake --build build/debug --target generate_bouncing_circle
 * ./build/debug/examples/generate_bouncing_circle
 * \endcode
 */

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>
#include <OpenImageIO/imageio.h>

#include <cmath>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <system_error>

namespace {

constexpr int kDisplayW = 1920;
constexpr int kDisplayH = 1080;
constexpr int kFrameStart = 1001;
constexpr int kFrameEnd = 1048;
constexpr float kRadius = 90.0f;
constexpr float kStartX = 140.0f;
constexpr float kStartY = 160.0f;
constexpr float kVelX = 61.0f;
constexpr float kVelY = 47.0f;
constexpr float kBlue[4] = {0.0f, 0.35f, 1.0f, 1.0f};

void bounce(float& cx, float& cy, float& vx, float& vy) {
  cx += vx;
  cy += vy;
  if (cx - kRadius < 0.0f) {
    cx = kRadius;
    vx = std::fabs(vx);
  } else if (cx + kRadius > static_cast<float>(kDisplayW)) {
    cx = static_cast<float>(kDisplayW) - kRadius;
    vx = -std::fabs(vx);
  }
  if (cy - kRadius < 0.0f) {
    cy = kRadius;
    vy = std::fabs(vy);
  } else if (cy + kRadius > static_cast<float>(kDisplayH)) {
    cy = static_cast<float>(kDisplayH) - kRadius;
    vy = -std::fabs(vy);
  }
}

bool write_frame(const std::filesystem::path& path, float cx, float cy,
                 std::string* error) {
  const int x0 = std::max(0, static_cast<int>(std::floor(cx - kRadius)));
  const int y0 = std::max(0, static_cast<int>(std::floor(cy - kRadius)));
  const int x1 = std::min(kDisplayW, static_cast<int>(std::ceil(cx + kRadius)));
  const int y1 = std::min(kDisplayH, static_cast<int>(std::ceil(cy + kRadius)));
  const int width = x1 - x0;
  const int height = y1 - y0;
  if (width <= 0 || height <= 0) {
    *error = "empty data window";
    return false;
  }

  OIIO::ImageSpec spec(width, height, 4, OIIO::TypeDesc::FLOAT);
  spec.x = x0;
  spec.y = y0;
  spec.full_x = 0;
  spec.full_y = 0;
  spec.full_width = kDisplayW;
  spec.full_height = kDisplayH;
  spec.channelnames = {"R", "G", "B", "A"};

  OIIO::ImageBuf buf(spec);
  const float clear[4] = {0.0f, 0.0f, 0.0f, 0.0f};
  if (!OIIO::ImageBufAlgo::fill(buf, clear)) {
    *error = buf.geterror();
    return false;
  }

  const float r2 = kRadius * kRadius;
  for (OIIO::ImageBuf::Iterator<float> it(buf); !it.done(); ++it) {
    const float dx = static_cast<float>(it.x()) + 0.5f - cx;
    const float dy = static_cast<float>(it.y()) + 0.5f - cy;
    if (dx * dx + dy * dy <= r2) {
      it[0] = kBlue[0];
      it[1] = kBlue[1];
      it[2] = kBlue[2];
      it[3] = kBlue[3];
    }
  }

  if (!buf.write(path.string())) {
    *error = buf.geterror();
    return false;
  }
  std::cout << path.string() << "  data " << width << "x" << height << " at ("
            << x0 << "," << y0 << ")\n";
  return true;
}

}  // namespace

int main(int argc, char** argv) {
  std::filesystem::path directory = DAILYBOY_BOUNCE_OUT_DIR;
  if (argc > 1) {
    directory = argv[1];
  }

  std::error_code ec;
  std::filesystem::create_directories(directory, ec);
  if (ec) {
    std::cerr << directory.string() << ": " << ec.message() << "\n";
    return 1;
  }

  try {
    float cx = kStartX;
    float cy = kStartY;
    float vx = kVelX;
    float vy = kVelY;
    std::string error;
    for (int frame = kFrameStart; frame <= kFrameEnd; ++frame) {
      char name[32];
      std::snprintf(name, sizeof(name), "bounce.%04d.exr", frame);
      if (!write_frame(directory / name, cx, cy, &error)) {
        std::cerr << error << "\n";
        return 1;
      }
      bounce(cx, cy, vx, vy);
    }
    const std::filesystem::path probe = directory / "bounce.1001.exr";
    OIIO::ImageBuf check(probe.string());
    if (!check.read()) {
      std::cerr << probe.string() << ": " << check.geterror() << "\n";
      return 1;
    }
    const OIIO::ImageSpec& spec = check.spec();
    std::cout << "ok  data " << spec.width << "x" << spec.height << " at ("
              << spec.x << "," << spec.y << ")  display " << spec.full_width
              << "x" << spec.full_height << "  channels " << spec.nchannels
              << "\n";
  } catch (const std::exception& ex) {
    std::cerr << ex.what() << "\n";
    return 1;
  }
  return 0;
}
