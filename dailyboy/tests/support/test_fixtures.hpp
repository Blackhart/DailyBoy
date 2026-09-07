#pragma once

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>
#include <OpenImageIO/imageio.h>

#include <array>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>

namespace dailyboy {
namespace test {

inline constexpr int kPlateFrameStart = 1001;
inline constexpr int kPlateFrameEnd = 1003;
inline constexpr int kPlateWidth = 8;
inline constexpr int kPlateHeight = 8;
inline constexpr const char* kPlatePattern = "plate.%04d.png";

/*!
 * \brief RGB fills for frames 1001, 1002, 1003 (red, green, blue).
 */
inline constexpr std::array<std::array<float, 3>, 3> kPlateColors = {{
    {{1.0f, 0.0f, 0.0f}},
    {{0.0f, 1.0f, 0.0f}},
    {{0.0f, 0.0f, 1.0f}},
}};

inline std::filesystem::path plate_pattern_path(
    const std::filesystem::path& dir) {
  return dir / kPlatePattern;
}

inline std::filesystem::path plate_frame_path(const std::filesystem::path& dir,
                                              int frame) {
  char name[32];
  std::snprintf(name, sizeof(name), "plate.%04d.png", frame);
  return dir / name;
}

inline std::filesystem::path plate_preview_mov_path(
    const std::filesystem::path& dir) {
  return dir / "preview.mov";
}

/*!
 * \brief Writes 8x8 RGB PNG frames 1001–1003 under \a dir.
 * \return \c false if a file cannot be written (OIIO error in \a error).
 */
inline bool write_plate_png_sequence(const std::filesystem::path& dir,
                                     std::string* error,
                                     int width = kPlateWidth,
                                     int height = kPlateHeight) {
  std::error_code ec;
  std::filesystem::create_directories(dir, ec);
  if (ec) {
    if (error != nullptr) {
      *error = ec.message();
    }
    return false;
  }
  try {
    for (int i = 0; i < 3; ++i) {
      const int frame = kPlateFrameStart + i;
      OIIO::ImageSpec spec(width, height, 3, OIIO::TypeDesc::UINT8);
      OIIO::ImageBuf buf(spec);
      const float rgb[3] = {kPlateColors[static_cast<std::size_t>(i)][0],
                            kPlateColors[static_cast<std::size_t>(i)][1],
                            kPlateColors[static_cast<std::size_t>(i)][2]};
      if (!OIIO::ImageBufAlgo::fill(buf, rgb)) {
        if (error != nullptr) {
          *error = buf.geterror();
        }
        return false;
      }
      const std::string path = plate_frame_path(dir, frame).string();
      if (!buf.write(path)) {
        if (error != nullptr) {
          *error = buf.geterror();
        }
        return false;
      }
    }
  } catch (const std::exception& ex) {
    if (error != nullptr) {
      *error = ex.what();
    }
    return false;
  }
  return true;
}

/*!
 * \brief Writes a schema-valid job YAML whose sequence.path is the plate under \a dir.
 *
 * \a output_mov is written as \c output.videos[].path for the single
 * preview entry. Empty uses \c plate_preview_mov_path(\a plate_dir).
 */
inline bool write_plate_job_yaml(const std::filesystem::path& yaml_path,
                                 const std::filesystem::path& plate_dir,
                                 std::string* error,
                                 std::filesystem::path output_mov = {},
                                 std::string codec = "h264",
                                 int canvas_width = kPlateWidth,
                                 int canvas_height = kPlateHeight) {
  const std::string seq = plate_pattern_path(plate_dir).string();
  if (output_mov.empty()) {
    output_mov = plate_preview_mov_path(plate_dir);
  }
  std::ofstream out(yaml_path);
  if (!out) {
    if (error != nullptr) {
      *error = "cannot write " + yaml_path.string();
    }
    return false;
  }
  out << "dailyboy_version: 1\n"
      << "color:\n"
      << "  ocio_config: \"/unused/config.ocio\"\n"
      << "layout:\n"
      << "  canvas:\n"
      << "    width: " << canvas_width << "\n"
      << "    height: " << canvas_height << "\n"
      << "  image:\n"
      << "    fit: \"contain\"\n"
      << "  slate:\n"
      << "    duration_frames: 0\n"
      << "    lines: []\n"
      << "output:\n"
      << "  videos:\n"
      << "    - id: preview\n"
      << "      enabled: true\n"
      << "      display_view:\n"
      << "        display: \"passthrough\"\n"
      << "        view: \"passthrough\"\n"
      << "      signal:\n"
      << "        range: " << (codec == "mjpeg" ? "pc" : "tv") << "\n"
      << "        matrix: bt709\n"
      << "        primaries: bt709\n"
      << "        transfer: bt709\n"
      << "      path: \"" << output_mov.string() << "\"\n"
      << "      codec: " << codec << "\n"
      << "  image_sequences: []\n"
      << "plans:\n"
      << "  - id: plate\n"
      << "    input_colorspace: ACES - ACEScg\n"
      << "    sequence:\n"
      << "      path: \"" << seq << "\"\n"
      << "      frame_start: " << kPlateFrameStart << "\n"
      << "      frame_end: " << kPlateFrameEnd << "\n";
  if (!out) {
    if (error != nullptr) {
      *error = "write failed: " + yaml_path.string();
    }
    return false;
  }
  return true;
}

}  // namespace test
}  // namespace dailyboy
