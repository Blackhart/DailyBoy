#pragma once

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>
#include <OpenImageIO/imageio.h>

#include <cstdio>
#include <filesystem>
#include <string>
#include <system_error>

#include "image/frame.hpp"

namespace dailyboy {
namespace test {

inline constexpr int kHdWidth = 1920;
inline constexpr int kHdHeight = 1080;
inline constexpr int kExamplePlateWidth = 2048;
inline constexpr int kExamplePlateHeight = 1080;

inline constexpr float kPerfGreen[3] = {0.0f, 1.0f, 0.0f};
inline constexpr float kPerfGray[3] = {0.5f, 0.5f, 0.5f};

inline OIIO::ImageBuf make_solid_rgb(int width, int height, OIIO::TypeDesc type,
                                     const float rgb[3]) {
  OIIO::ImageSpec spec(width, height, 3, type);
  OIIO::ImageBuf buf(spec);
  OIIO::ImageBufAlgo::fill(buf, OIIO::cspan<float>(rgb, 3));
  return buf;
}

inline Frame hd_rgb_float() {
  return Frame(
      make_solid_rgb(kHdWidth, kHdHeight, OIIO::TypeDesc::FLOAT, kPerfGreen));
}

inline Frame hd_rgb_uint8() {
  return Frame(
      make_solid_rgb(kHdWidth, kHdHeight, OIIO::TypeDesc::UINT8, kPerfGray));
}

inline Frame hd_rgba_float() {
  OIIO::ImageSpec spec(kHdWidth, kHdHeight, 4, OIIO::TypeDesc::FLOAT);
  OIIO::ImageBuf buf(spec);
  const float rgba[4] = {0.0f, 1.0f, 0.0f, 1.0f};
  OIIO::ImageBufAlgo::fill(buf, OIIO::cspan<float>(rgba, 4));
  return Frame(std::move(buf));
}

inline Frame hd_odd_rgb_float() {
  return Frame(make_solid_rgb(kHdWidth - 1, kHdHeight, OIIO::TypeDesc::FLOAT,
                              kPerfGreen));
}

inline std::filesystem::path perf_data_file(const char* relative) {
  return std::filesystem::path(DAILYBOY_PERF_DATA_DIR) / relative;
}

inline bool load_perf_frame(const std::filesystem::path& path, Frame& out) {
  OIIO::ImageBuf buf(path.string());
  if (!buf.read()) {
    return false;
  }
  out = Frame(std::move(buf));
  return true;
}

inline Frame example_plate_float() {
  return Frame(make_solid_rgb(kExamplePlateWidth, kExamplePlateHeight,
                              OIIO::TypeDesc::FLOAT, kPerfGreen));
}

inline Frame copy_frame(const Frame& src) {
  OIIO::ImageBuf buf;
  buf.copy(src.buf());
  return Frame(std::move(buf));
}

inline std::filesystem::path unique_perf_dir(const char* name) {
  const std::filesystem::path dir =
      std::filesystem::path(DAILYBOY_TEST_BINARY_DIR) / name;
  std::filesystem::remove_all(dir);
  std::filesystem::create_directories(dir);
  return dir;
}

inline constexpr int kPerfPlateFrameStart = 1001;

/*!
 * \brief Writes \c count identical solid plates, hard-linking after the first.
 */
inline bool write_example_plate_pngs(const std::filesystem::path& dir,
                                     int count) {
  if (count < 1) {
    return false;
  }
  std::error_code ec;
  std::filesystem::create_directories(dir, ec);
  if (ec) {
    return false;
  }
  OIIO::ImageBuf buf = make_solid_rgb(kExamplePlateWidth, kExamplePlateHeight,
                                      OIIO::TypeDesc::UINT8, kPerfGray);
  char name[32];
  std::snprintf(name, sizeof(name), "plate.%04d.png", kPerfPlateFrameStart);
  const std::filesystem::path first = dir / name;
  if (!buf.write(first.string())) {
    return false;
  }
  for (int i = 1; i < count; ++i) {
    std::snprintf(name, sizeof(name), "plate.%04d.png",
                  kPerfPlateFrameStart + i);
    const std::filesystem::path dest = dir / name;
    std::filesystem::create_hard_link(first, dest, ec);
    if (ec) {
      ec.clear();
      std::filesystem::copy_file(first, dest, ec);
      if (ec) {
        return false;
      }
    }
  }
  return true;
}

}  // namespace test
}  // namespace dailyboy
