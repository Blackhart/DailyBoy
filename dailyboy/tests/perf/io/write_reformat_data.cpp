/*!
 * \file write_reformat_data.cpp
 * \brief Writes inspectable EXR plates for reformat_perf into tests/perf/data.
 *
 * \code
 * cmake --build build/release --target generate_reformat_perf_data
 * ./build/release/dailyboy/tests/perf/generate_reformat_perf_data
 * \endcode
 */

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>
#include <OpenImageIO/imageio.h>

#include <filesystem>
#include <iostream>
#include <string>
#include <system_error>

namespace {

constexpr int kHdWidth = 1920;
constexpr int kHdHeight = 1080;
constexpr int kCropW = 180;
constexpr int kCropH = 180;
constexpr int kCropX = 50;
constexpr int kCropY = 70;
constexpr float kRadius = 90.0f;
constexpr float kGreen[3] = {0.0f, 1.0f, 0.0f};
constexpr float kGreenA[4] = {0.0f, 1.0f, 0.0f, 1.0f};
constexpr float kBlue[4] = {0.0f, 0.35f, 1.0f, 1.0f};

std::filesystem::path out_dir() {
  return std::filesystem::path(DAILYBOY_PERF_DATA_DIR) / "reformat";
}

bool write_buf(OIIO::ImageBuf& buf, const std::filesystem::path& path,
               std::string* error) {
  buf.specmod().attribute("compression", "zip");
  if (!buf.write(path.string(), OIIO::TypeDesc::HALF)) {
    *error = buf.geterror();
    return false;
  }
  const OIIO::ImageSpec& spec = buf.spec();
  std::cout << path.filename().string() << "  data " << spec.width << "x"
            << spec.height << " at (" << spec.x << "," << spec.y
            << ")  display " << spec.full_width << "x" << spec.full_height
            << "  ch " << spec.nchannels << "\n";
  return true;
}

OIIO::ImageBuf make_solid(int width, int height, int nchannels, int x, int y,
                          int full_width, int full_height, const float* fill) {
  OIIO::ImageSpec spec(width, height, nchannels, OIIO::TypeDesc::HALF);
  spec.x = x;
  spec.y = y;
  spec.full_x = 0;
  spec.full_y = 0;
  spec.full_width = full_width;
  spec.full_height = full_height;
  OIIO::ImageBuf buf(spec);
  OIIO::ImageBufAlgo::fill(buf, OIIO::cspan<float>(fill, nchannels));
  return buf;
}

void paint_circle(OIIO::ImageBuf& buf, float cx, float cy, const float* rgba,
                  int nchannels) {
  const float r2 = kRadius * kRadius;
  for (OIIO::ImageBuf::Iterator<float> it(buf); !it.done(); ++it) {
    const float dx = static_cast<float>(it.x()) + 0.5f - cx;
    const float dy = static_cast<float>(it.y()) + 0.5f - cy;
    if (dx * dx + dy * dy > r2) {
      continue;
    }
    for (int c = 0; c < nchannels; ++c) {
      it[c] = rgba[c];
    }
  }
}

}  // namespace

int main() {
  const std::filesystem::path dir = out_dir();
  std::error_code ec;
  std::filesystem::create_directories(dir, ec);
  if (ec) {
    std::cerr << dir.string() << ": " << ec.message() << "\n";
    return 1;
  }

  std::string error;
  {
    OIIO::ImageBuf buf = make_solid(kHdWidth - 1, kHdHeight, 3, 0, 0,
                                    kHdWidth - 1, kHdHeight, kGreen);
    if (!write_buf(buf, dir / "hd_odd_rgb.exr", &error)) {
      std::cerr << error << "\n";
      return 1;
    }
  }
  {
    OIIO::ImageBuf buf =
        make_solid(kHdWidth, kHdHeight, 4, 0, 0, kHdWidth, kHdHeight, kGreenA);
    if (!write_buf(buf, dir / "hd_rgba.exr", &error)) {
      std::cerr << error << "\n";
      return 1;
    }
  }
  {
    OIIO::ImageBuf buf = make_solid(kCropW, kCropH, 3, kCropX, kCropY, kHdWidth,
                                    kHdHeight, kGreen);
    OIIO::ImageBufAlgo::zero(buf);
    const float cx = static_cast<float>(kCropX) + kRadius;
    const float cy = static_cast<float>(kCropY) + kRadius;
    paint_circle(buf, cx, cy, kGreenA, 3);
    if (!write_buf(buf, dir / "hd_cropped_rgb.exr", &error)) {
      std::cerr << error << "\n";
      return 1;
    }
  }
  {
    const int data_w = kCropW + 1;
    const int data_h = kCropH - 1;
    const int data_x = kCropX + 1;
    const int data_y = kCropY + 1;
    const int full_w = kHdWidth - 1;
    const int full_h = kHdHeight - 1;
    const float clear[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    OIIO::ImageBuf buf =
        make_solid(data_w, data_h, 4, data_x, data_y, full_w, full_h, clear);
    const float cx =
        static_cast<float>(data_x) + static_cast<float>(data_w) / 2;
    const float cy =
        static_cast<float>(data_y) + static_cast<float>(data_h) / 2;
    paint_circle(buf, cx, cy, kBlue, 4);
    if (!write_buf(buf, dir / "hd_cropped_odd_rgba.exr", &error)) {
      std::cerr << error << "\n";
      return 1;
    }
  }
  return 0;
}
