#include <gtest/gtest.h>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <variant>
#include <vector>

#include "error/job.hpp"
#include "job/job.hpp"
#include "job/layout.hpp"
#include "job/loader.hpp"
#include "job/metadata.hpp"
#include "job/output.hpp"
#include "job/plans.hpp"
#include "job/primitives.hpp"
#include "job/text.hpp"

namespace {

std::filesystem::path job_loader_fixture() {
  return std::filesystem::path(DAILYBOY_TEST_DATA_DIR) /
         "test__job_loader__load_job__valid_yaml__load_all_fields.yaml";
}

void expect_near(double actual, double expected) {
  EXPECT_NEAR(actual, expected, 1e-9);
}

void expect_rgb(const dailyboy::RGBColor& color, double r, double g, double b) {
  expect_near(color.r(), r);
  expect_near(color.g(), g);
  expect_near(color.b(), b);
}

void expect_margin_all(const dailyboy::Margin& margin, int value) {
  EXPECT_EQ(margin.top(), value);
  EXPECT_EQ(margin.right(), value);
  EXPECT_EQ(margin.bottom(), value);
  EXPECT_EQ(margin.left(), value);
}

void expect_margin(const dailyboy::Margin& margin, int top, int right,
                   int bottom, int left) {
  EXPECT_EQ(margin.top(), top);
  EXPECT_EQ(margin.right(), right);
  EXPECT_EQ(margin.bottom(), bottom);
  EXPECT_EQ(margin.left(), left);
}

std::string expect_string_sub(const dailyboy::JobMetadata& metadata,
                              const std::string& key) {
  const auto& subs = metadata.substitutions();
  const auto it = subs.find(key);
  EXPECT_NE(it, subs.end());
  if (it == subs.end()) {
    return {};
  }
  EXPECT_TRUE((std::holds_alternative<std::string>(it->second)));
  if (!std::holds_alternative<std::string>(it->second)) {
    return {};
  }
  return std::get<std::string>(it->second);
}

std::map<std::string, std::string> expect_frame_map_sub(
    const dailyboy::JobMetadata& metadata, const std::string& key) {
  const auto& subs = metadata.substitutions();
  const auto it = subs.find(key);
  EXPECT_NE(it, subs.end());
  if (it == subs.end()) {
    return {};
  }
  EXPECT_TRUE(
      (std::holds_alternative<std::map<std::string, std::string>>(it->second)));
  if (!std::holds_alternative<std::map<std::string, std::string>>(it->second)) {
    return {};
  }
  return std::get<std::map<std::string, std::string>>(it->second);
}

void expect_layout_anchor(const dailyboy::TextPosition& position,
                          dailyboy::TextPositionModeLayout::Anchor anchor) {
  EXPECT_EQ(position.mode(), dailyboy::TextPosition::Mode::Layout);
  const auto& layout =
      std::get<dailyboy::TextPositionModeLayout>(position.value());
  EXPECT_EQ(layout.anchor(), anchor);
}

void expect_pixel_position(const dailyboy::TextPosition& position, int x,
                           int y) {
  EXPECT_EQ(position.mode(), dailyboy::TextPosition::Mode::Pixel);
  const auto& pixel =
      std::get<dailyboy::TextPositionModePixel>(position.value());
  EXPECT_EQ(pixel.x(), x);
  EXPECT_EQ(pixel.y(), y);
}

std::filesystem::path write_dnxhd_job(const std::string& name,
                                      const std::string& codec_options) {
  const std::filesystem::path dir =
      std::filesystem::path(DAILYBOY_TEST_BINARY_DIR) / "dnxhd_job_loader";
  std::filesystem::create_directories(dir);
  const std::filesystem::path path = dir / name;
  std::ofstream out(path);
  out << "dailyboy_version: 1\n"
         "color:\n"
         "  ocio_config: \"/unused/config.ocio\"\n"
         "layout:\n"
         "  canvas:\n"
         "    width: 8\n"
         "    height: 8\n"
         "  image:\n"
         "    fit: \"contain\"\n"
         "  slate:\n"
         "    duration_frames: 0\n"
         "    lines: []\n"
         "output:\n"
         "  videos:\n"
         "    - id: preview\n"
         "      enabled: true\n"
         "      display_view:\n"
         "        display: \"passthrough\"\n"
         "        view: \"passthrough\"\n"
         "      signal:\n"
         "        range: tv\n"
         "        matrix: bt709\n"
         "        primaries: bt709\n"
         "        transfer: bt709\n"
         "      path: /tmp/dailyboy_dnxhd.mov\n"
         "      codec: dnxhd\n";
  if (!codec_options.empty()) {
    out << "      codec_options:\n" << codec_options;
  }
  out << "  image_sequences: []\n"
         "plans:\n"
         "  - id: plate\n"
         "    input_colorspace: ACES - ACEScg\n"
         "    sequence:\n"
         "      path: /tmp/plate.%04d.png\n"
         "      frame_start: 1001\n"
         "      frame_end: 1003\n";
  return path;
}

std::filesystem::path write_output_job(const std::string& name,
                                       const std::string& output_yaml,
                                       int dailyboy_version = 1,
                                       int canvas_width = 8,
                                       int canvas_height = 8) {
  const std::filesystem::path dir =
      std::filesystem::path(DAILYBOY_TEST_BINARY_DIR) / "output_contract";
  std::filesystem::create_directories(dir);
  const std::filesystem::path path = dir / name;
  std::ofstream out(path);
  out << "dailyboy_version: " << dailyboy_version << "\n"
      << "color:\n"
         "  ocio_config: \"/unused/config.ocio\"\n"
         "layout:\n"
         "  canvas:\n"
      << "    width: " << canvas_width << "\n"
      << "    height: " << canvas_height << "\n"
      << "  image:\n"
         "    fit: \"contain\"\n"
         "  slate:\n"
         "    duration_frames: 0\n"
         "    lines: []\n"
         "output:\n"
      << output_yaml
      << "plans:\n"
         "  - id: plate\n"
         "    input_colorspace: ACES - ACEScg\n"
         "    sequence:\n"
         "      path: /tmp/plate.%04d.png\n"
         "      frame_start: 1001\n"
         "      frame_end: 1003\n";
  return path;
}

struct ContractJobSpec {
  const char* metadata_yaml = nullptr;
  bool include_color = true;
  const char* color_yaml = nullptr;
  bool include_layout = true;
  const char* layout_yaml = nullptr;
  bool include_output = true;
  bool include_plans = true;
  const char* plans_yaml = nullptr;
};

std::filesystem::path write_contract_job(
    const std::string& name, const ContractJobSpec& spec = ContractJobSpec()) {
  const std::filesystem::path dir =
      std::filesystem::path(DAILYBOY_TEST_BINARY_DIR) / "output_contract";
  std::filesystem::create_directories(dir);
  const std::filesystem::path path = dir / name;
  std::ofstream out(path);
  out << "dailyboy_version: 1\n";
  if (spec.metadata_yaml != nullptr) {
    out << spec.metadata_yaml;
  }
  if (spec.include_color) {
    if (spec.color_yaml != nullptr) {
      out << spec.color_yaml;
    } else {
      out << "color:\n"
             "  ocio_config: \"/unused/config.ocio\"\n";
    }
  }
  if (spec.include_layout) {
    if (spec.layout_yaml != nullptr) {
      out << spec.layout_yaml;
    } else {
      out << "layout:\n"
             "  canvas:\n"
             "    width: 8\n"
             "    height: 8\n"
             "  image:\n"
             "    fit: \"contain\"\n"
             "  slate:\n"
             "    duration_frames: 0\n"
             "    lines: []\n";
    }
  }
  if (spec.include_output) {
    out << "output:\n"
           "  videos:\n"
           "    - id: preview\n"
           "      enabled: true\n"
           "      display_view:\n"
           "        display: \"passthrough\"\n"
           "        view: \"passthrough\"\n"
           "      signal:\n"
           "        range: tv\n"
           "        matrix: bt709\n"
           "        primaries: bt709\n"
           "        transfer: bt709\n"
           "      path: /tmp/dailyboy_contract.mov\n"
           "      codec: h264\n"
           "  image_sequences: []\n";
  }
  if (spec.include_plans) {
    if (spec.plans_yaml != nullptr) {
      out << spec.plans_yaml;
    } else {
      out << "plans:\n"
             "  - id: plate\n"
             "    input_colorspace: ACES - ACEScg\n"
             "    sequence:\n"
             "      path: /tmp/plate.%04d.png\n"
             "      frame_start: 1001\n"
             "      frame_end: 1003\n";
    }
  }
  return path;
}

std::filesystem::path write_image_contract_job(
    const std::string& name, const std::string& image_fields) {
  const std::string layout_yaml =
      "layout:\n"
      "  canvas:\n"
      "    width: 8\n"
      "    height: 8\n"
      "  image:\n" +
      image_fields;
  return write_contract_job(name, {.layout_yaml = layout_yaml.c_str()});
}

std::filesystem::path write_background_contract_job(
    const std::string& name, const std::string& background_block) {
  const std::string layout_yaml =
      "layout:\n"
      "  canvas:\n"
      "    width: 8\n"
      "    height: 8\n"
      "  image:\n"
      "    fit: \"contain\"\n" +
      background_block;
  return write_contract_job(name, {.layout_yaml = layout_yaml.c_str()});
}

std::filesystem::path write_burn_in_contract_job(
    const std::string& name, const std::string& burn_ins_block) {
  const std::string layout_yaml =
      "layout:\n"
      "  canvas:\n"
      "    width: 8\n"
      "    height: 8\n"
      "  image:\n"
      "    fit: \"contain\"\n" +
      burn_ins_block;
  return write_contract_job(name, {.layout_yaml = layout_yaml.c_str()});
}

std::filesystem::path write_burn_in_position_job(
    const std::string& name, const std::string& position_fields) {
  const std::string block = std::string(
                                "  burn_ins:\n"
                                "    - template: \"{frame}\"\n"
                                "      position:\n") +
                            position_fields +
                            "      font:\n"
                            "        path: \"/unused/font.ttf\"\n"
                            "        size_px: 12\n";
  return write_burn_in_contract_job(name, block);
}

std::filesystem::path write_burn_in_font_job(const std::string& name,
                                             const std::string& font_fields) {
  const std::string block = std::string(
                                "  burn_ins:\n"
                                "    - template: \"{frame}\"\n"
                                "      position:\n"
                                "        mode: \"layout\"\n"
                                "        anchor: \"top_left\"\n"
                                "      font:\n") +
                            font_fields;
  return write_burn_in_contract_job(name, block);
}

std::filesystem::path write_layout_job(const std::string& name,
                                       const std::string& burn_ins_yaml) {
  const std::filesystem::path dir =
      std::filesystem::path(DAILYBOY_TEST_BINARY_DIR) / "layout_job_loader";
  std::filesystem::create_directories(dir);
  const std::filesystem::path path = dir / name;
  std::ofstream out(path);
  out << "dailyboy_version: 1\n"
         "color:\n"
         "  ocio_config: \"/unused/config.ocio\"\n"
         "layout:\n"
         "  canvas:\n"
         "    width: 8\n"
         "    height: 8\n"
         "  image:\n"
         "    fit: \"contain\"\n"
         "  burn_ins:\n"
      << burn_ins_yaml
      << "  slate:\n"
         "    duration_frames: 0\n"
         "    lines: []\n"
         "output:\n"
         "  videos: []\n"
         "  image_sequences:\n"
         "    - id: preview\n"
         "      enabled: true\n"
         "      display_view:\n"
         "        display: \"passthrough\"\n"
         "        view: \"passthrough\"\n"
         "      path_pattern: /tmp/preview.%04d.png\n"
         "plans:\n"
         "  - id: plate\n"
         "    input_colorspace: ACES - ACEScg\n"
         "    sequence:\n"
         "      path: /tmp/plate.%04d.png\n"
         "      frame_start: 1001\n"
         "      frame_end: 1003\n";
  return path;
}

void expect_user_message(const dailyboy::Status& status,
                         const std::string& needle) {
  EXPECT_FALSE(status.ok()) << status.message();
  EXPECT_EQ(status.code(), dailyboy::Status::Code::kUser) << status.message();
  EXPECT_NE(status.message().find(needle), std::string::npos)
      << status.message();
}

dailyboy::JobOutputVideoDnxhd load_dnxhd_options(
    const std::filesystem::path& yaml) {
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  EXPECT_TRUE(schema.ok()) << schema.message();
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);
  EXPECT_TRUE(job.ok()) << job.status().message();
  if (!job.ok()) {
    return {};
  }
  return std::get<dailyboy::JobOutputVideoDnxhd>(
      job.value().output().videos().videos().front().codec_options());
}

void expect_percent_position(const dailyboy::TextPosition& position, double x,
                             double y) {
  EXPECT_EQ(position.mode(), dailyboy::TextPosition::Mode::Percent);
  const auto& percent =
      std::get<dailyboy::TextPositionModePercent>(position.value());
  expect_near(percent.x(), x);
  expect_near(percent.y(), y);
}

void expect_font(const dailyboy::TextFont& font,
                 const std::filesystem::path& path, int size_px) {
  EXPECT_EQ(font.path(), path);
  EXPECT_EQ(font.size_px(), size_px);
}

void expect_burn_in_box_fill(const dailyboy::JobLayoutBurnInBox& box, double r,
                             double g, double b, double opacity, int margin) {
  EXPECT_EQ(box.mode(),
            dailyboy::JobLayoutBurnInBox::JobLayoutBurnInBoxModeValue::Fill);
  expect_rgb(box.color(), r, g, b);
  expect_near(box.opacity(), opacity);
  expect_margin_all(box.margin(), margin);
}

void expect_burn_in_box_outline(const dailyboy::JobLayoutBurnInBox& box,
                                double r, double g, double b, double opacity,
                                int top, int right, int bottom, int left) {
  EXPECT_EQ(box.mode(),
            dailyboy::JobLayoutBurnInBox::JobLayoutBurnInBoxModeValue::Outline);
  expect_rgb(box.color(), r, g, b);
  expect_near(box.opacity(), opacity);
  expect_margin(box.margin(), top, right, bottom, left);
}

}  // namespace

/*!
 * \brief Accepts a complete job whose \c dailyboy_version is 1.
 */
TEST(JobLoader, ValidateJobSchema_DailyboyVersion1_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_output_job("version_1.yaml",
                       "  videos:\n"
                       "    - id: preview\n"
                       "      enabled: true\n"
                       "      display_view:\n"
                       "        display: \"passthrough\"\n"
                       "        view: \"passthrough\"\n"
                       "      signal:\n"
                       "        range: tv\n"
                       "        matrix: bt709\n"
                       "        primaries: bt709\n"
                       "        transfer: bt709\n"
                       "      path: /tmp/dailyboy_contract.mov\n"
                       "      codec: h264\n"
                       "  image_sequences: []\n",
                       1);

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  EXPECT_EQ(job.value().dailyboy_version(), 1);
}

/*!
 * \brief Rejects a complete job whose \c dailyboy_version is not 1.
 */
TEST(JobLoader, ValidateJobSchema_DailyboyVersionNot1_ReturnsUserError) {
  for (int version : {0, 2}) {
    // Prepare
    const std::filesystem::path yaml =
        write_output_job("version_" + std::to_string(version) + ".yaml",
                         "  videos:\n"
                         "    - id: preview\n"
                         "      enabled: true\n"
                         "      display_view:\n"
                         "        display: \"passthrough\"\n"
                         "        view: \"passthrough\"\n"
                         "      signal:\n"
                         "        range: tv\n"
                         "        matrix: bt709\n"
                         "        primaries: bt709\n"
                         "        transfer: bt709\n"
                         "      path: /tmp/dailyboy_contract.mov\n"
                         "      codec: h264\n"
                         "  image_sequences: []\n",
                         version);

    // Test
    dailyboy::Status schema = dailyboy::validate_job_schema(yaml);

    // Assert
    expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_38));
    expect_user_message(schema,
                        "(got integer " + std::to_string(version) + ")");
  }
}

/*!
 * \brief Rejects a layout block that omits \c canvas.
 */
TEST(JobLoader, ValidateJobSchema_OmittedCanvas_ReturnsUserError) {
  // Prepare
  const char* layout_yaml =
      "layout:\n"
      "  image:\n"
      "    fit: \"contain\"\n"
      "  slate:\n"
      "    duration_frames: 0\n"
      "    lines: []\n";
  const std::filesystem::path yaml =
      write_contract_job("no_canvas.yaml", {.layout_yaml = layout_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_54));
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_54));
}

/*!
 * \brief Accepts a layout block that includes \c canvas.
 */
TEST(JobLoader, ValidateJobSchema_PresentCanvas_Succeeds) {
  // Prepare
  const char* layout_yaml =
      "layout:\n"
      "  canvas:\n"
      "    width: 8\n"
      "    height: 8\n"
      "  image:\n"
      "    fit: \"contain\"\n"
      "  slate:\n"
      "    duration_frames: 0\n"
      "    lines: []\n";
  const std::filesystem::path yaml =
      write_contract_job("with_canvas.yaml", {.layout_yaml = layout_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  EXPECT_EQ(job.value().layout().canvas().width(), 8);
  EXPECT_EQ(job.value().layout().canvas().height(), 8);
}

/*!
 * \brief Accepts a canvas that includes \c width.
 */
TEST(JobLoader, ValidateJobSchema_PresentCanvasWidth_Succeeds) {
  // Prepare
  const char* layout_yaml =
      "layout:\n"
      "  canvas:\n"
      "    width: 8\n"
      "    height: 8\n"
      "  image:\n"
      "    fit: \"contain\"\n"
      "  slate:\n"
      "    duration_frames: 0\n"
      "    lines: []\n";
  const std::filesystem::path yaml = write_contract_job(
      "with_canvas_width.yaml", {.layout_yaml = layout_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  EXPECT_EQ(job.value().layout().canvas().width(), 8);
}

/*!
 * \brief Rejects a canvas that omits \c width.
 */
TEST(JobLoader, ValidateJobSchema_OmittedCanvasWidth_ReturnsUserError) {
  // Prepare
  const char* layout_yaml =
      "layout:\n"
      "  canvas:\n"
      "    height: 8\n"
      "  image:\n"
      "    fit: \"contain\"\n"
      "  slate:\n"
      "    duration_frames: 0\n"
      "    lines: []\n";
  const std::filesystem::path yaml =
      write_contract_job("no_canvas_width.yaml", {.layout_yaml = layout_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_55));
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_55));
}

/*!
 * \brief Rejects a canvas whose \c width is not an integer.
 */
TEST(JobLoader, ValidateJobSchema_NonIntegerCanvasWidth_ReturnsUserError) {
  // Prepare
  const char* layout_yaml =
      "layout:\n"
      "  canvas:\n"
      "    width: 8.5\n"
      "    height: 8\n"
      "  image:\n"
      "    fit: \"contain\"\n"
      "  slate:\n"
      "    duration_frames: 0\n"
      "    lines: []\n";
  const std::filesystem::path yaml = write_contract_job(
      "non_integer_canvas_width.yaml", {.layout_yaml = layout_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_56));
  expect_user_message(schema, "layout.canvas.width is number 8.5");
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_56));
  expect_user_message(job.status(), "layout.canvas.width is a number");
}

/*!
 * \brief Rejects a canvas whose \c width is an odd integer.
 */
TEST(JobLoader, ValidateJobSchema_OddCanvasWidth_ReturnsUserError) {
  // Prepare
  const char* layout_yaml =
      "layout:\n"
      "  canvas:\n"
      "    width: 7\n"
      "    height: 8\n"
      "  image:\n"
      "    fit: \"contain\"\n"
      "  slate:\n"
      "    duration_frames: 0\n"
      "    lines: []\n";
  const std::filesystem::path yaml =
      write_contract_job("odd_canvas_width.yaml", {.layout_yaml = layout_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_1));
  expect_user_message(schema, "layout.canvas.width");
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_1));
  expect_user_message(job.status(), "layout.canvas.width");
}

/*!
 * \brief Accepts a canvas whose \c width is an even integer.
 */
TEST(JobLoader, ValidateJobSchema_EvenCanvasWidth_Succeeds) {
  // Prepare
  const char* layout_yaml =
      "layout:\n"
      "  canvas:\n"
      "    width: 8\n"
      "    height: 8\n"
      "  image:\n"
      "    fit: \"contain\"\n"
      "  slate:\n"
      "    duration_frames: 0\n"
      "    lines: []\n";
  const std::filesystem::path yaml = write_contract_job(
      "even_canvas_width.yaml", {.layout_yaml = layout_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  EXPECT_EQ(job.value().layout().canvas().width() % 2, 0);
}

/*!
 * \brief Rejects a canvas whose \c width is less than 2.
 */
TEST(JobLoader, ValidateJobSchema_CanvasWidthBelow2_ReturnsUserError) {
  // Prepare
  const char* layout_yaml =
      "layout:\n"
      "  canvas:\n"
      "    width: 0\n"
      "    height: 8\n"
      "  image:\n"
      "    fit: \"contain\"\n"
      "  slate:\n"
      "    duration_frames: 0\n"
      "    lines: []\n";
  const std::filesystem::path yaml = write_contract_job(
      "below_min_canvas_width.yaml", {.layout_yaml = layout_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_60));
  expect_user_message(schema, "(got integer 0)");
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_60));
  expect_user_message(job.status(), "(got integer 0)");
}

/*!
 * \brief Accepts a canvas whose \c width is at least 2.
 */
TEST(JobLoader, ValidateJobSchema_CanvasWidthAtLeast2_Succeeds) {
  // Prepare
  const char* layout_yaml =
      "layout:\n"
      "  canvas:\n"
      "    width: 2\n"
      "    height: 8\n"
      "  image:\n"
      "    fit: \"contain\"\n"
      "  slate:\n"
      "    duration_frames: 0\n"
      "    lines: []\n";
  const std::filesystem::path yaml =
      write_contract_job("min_canvas_width.yaml", {.layout_yaml = layout_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  EXPECT_GE(job.value().layout().canvas().width(), 2);
}

/*!
 * \brief Accepts a canvas that includes \c height.
 */
TEST(JobLoader, ValidateJobSchema_PresentCanvasHeight_Succeeds) {
  // Prepare
  const char* layout_yaml =
      "layout:\n"
      "  canvas:\n"
      "    width: 8\n"
      "    height: 8\n"
      "  image:\n"
      "    fit: \"contain\"\n"
      "  slate:\n"
      "    duration_frames: 0\n"
      "    lines: []\n";
  const std::filesystem::path yaml = write_contract_job(
      "with_canvas_height.yaml", {.layout_yaml = layout_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  EXPECT_EQ(job.value().layout().canvas().height(), 8);
}

/*!
 * \brief Rejects a canvas that omits \c height.
 */
TEST(JobLoader, ValidateJobSchema_OmittedCanvasHeight_ReturnsUserError) {
  // Prepare
  const char* layout_yaml =
      "layout:\n"
      "  canvas:\n"
      "    width: 8\n"
      "  image:\n"
      "    fit: \"contain\"\n"
      "  slate:\n"
      "    duration_frames: 0\n"
      "    lines: []\n";
  const std::filesystem::path yaml =
      write_contract_job("no_canvas_height.yaml", {.layout_yaml = layout_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_57));
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_57));
}

/*!
 * \brief Rejects a canvas whose \c height is not an integer.
 */
TEST(JobLoader, ValidateJobSchema_NonIntegerCanvasHeight_ReturnsUserError) {
  // Prepare
  const char* layout_yaml =
      "layout:\n"
      "  canvas:\n"
      "    width: 8\n"
      "    height: 8.5\n"
      "  image:\n"
      "    fit: \"contain\"\n"
      "  slate:\n"
      "    duration_frames: 0\n"
      "    lines: []\n";
  const std::filesystem::path yaml = write_contract_job(
      "non_integer_canvas_height.yaml", {.layout_yaml = layout_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_58));
  expect_user_message(schema, "layout.canvas.height is number 8.5");
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_58));
  expect_user_message(job.status(), "layout.canvas.height is a number");
}

/*!
 * \brief Rejects a canvas whose \c height is an odd integer.
 */
TEST(JobLoader, ValidateJobSchema_OddCanvasHeight_ReturnsUserError) {
  // Prepare
  const char* layout_yaml =
      "layout:\n"
      "  canvas:\n"
      "    width: 8\n"
      "    height: 7\n"
      "  image:\n"
      "    fit: \"contain\"\n"
      "  slate:\n"
      "    duration_frames: 0\n"
      "    lines: []\n";
  const std::filesystem::path yaml = write_contract_job(
      "odd_canvas_height.yaml", {.layout_yaml = layout_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_1));
  expect_user_message(schema, "layout.canvas.height");
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_1));
  expect_user_message(job.status(), "layout.canvas.height");
}

/*!
 * \brief Accepts a canvas whose \c height is an even integer.
 */
TEST(JobLoader, ValidateJobSchema_EvenCanvasHeight_Succeeds) {
  // Prepare
  const char* layout_yaml =
      "layout:\n"
      "  canvas:\n"
      "    width: 8\n"
      "    height: 8\n"
      "  image:\n"
      "    fit: \"contain\"\n"
      "  slate:\n"
      "    duration_frames: 0\n"
      "    lines: []\n";
  const std::filesystem::path yaml = write_contract_job(
      "even_canvas_height.yaml", {.layout_yaml = layout_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  EXPECT_EQ(job.value().layout().canvas().height() % 2, 0);
}

/*!
 * \brief Rejects a canvas whose \c height is less than 2.
 */
TEST(JobLoader, ValidateJobSchema_CanvasHeightBelow2_ReturnsUserError) {
  // Prepare
  const char* layout_yaml =
      "layout:\n"
      "  canvas:\n"
      "    width: 8\n"
      "    height: 0\n"
      "  image:\n"
      "    fit: \"contain\"\n"
      "  slate:\n"
      "    duration_frames: 0\n"
      "    lines: []\n";
  const std::filesystem::path yaml = write_contract_job(
      "below_min_canvas_height.yaml", {.layout_yaml = layout_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_61));
  expect_user_message(schema, "(got integer 0)");
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_61));
  expect_user_message(job.status(), "(got integer 0)");
}

/*!
 * \brief Accepts a canvas whose \c height is at least 2.
 */
TEST(JobLoader, ValidateJobSchema_CanvasHeightAtLeast2_Succeeds) {
  // Prepare
  const char* layout_yaml =
      "layout:\n"
      "  canvas:\n"
      "    width: 8\n"
      "    height: 2\n"
      "  image:\n"
      "    fit: \"contain\"\n"
      "  slate:\n"
      "    duration_frames: 0\n"
      "    lines: []\n";
  const std::filesystem::path yaml = write_contract_job(
      "min_canvas_height.yaml", {.layout_yaml = layout_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  EXPECT_GE(job.value().layout().canvas().height(), 2);
}

/*!
 * \brief Accepts a layout that includes \c pixel_aspect.
 */
TEST(JobLoader, ValidateJobSchema_PresentPixelAspect_Succeeds) {
  // Prepare
  const char* layout_yaml =
      "layout:\n"
      "  canvas:\n"
      "    width: 8\n"
      "    height: 8\n"
      "  pixel_aspect: 2.0\n"
      "  image:\n"
      "    fit: \"contain\"\n"
      "  slate:\n"
      "    duration_frames: 0\n"
      "    lines: []\n";
  const std::filesystem::path yaml = write_contract_job(
      "with_pixel_aspect.yaml", {.layout_yaml = layout_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  expect_near(job.value().layout().pixel_aspect().aspect(), 2.0);
}

/*!
 * \brief Accepts a layout that omits \c pixel_aspect.
 */
TEST(JobLoader, ValidateJobSchema_OmittedPixelAspect_Succeeds) {
  // Prepare
  const char* layout_yaml =
      "layout:\n"
      "  canvas:\n"
      "    width: 8\n"
      "    height: 8\n"
      "  image:\n"
      "    fit: \"contain\"\n"
      "  slate:\n"
      "    duration_frames: 0\n"
      "    lines: []\n";
  const std::filesystem::path yaml =
      write_contract_job("no_pixel_aspect.yaml", {.layout_yaml = layout_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  expect_near(job.value().layout().pixel_aspect().aspect(), 1.0);
}

/*!
 * \brief Rejects \c pixel_aspect that is not a number.
 */
TEST(JobLoader, ValidateJobSchema_NonNumberPixelAspect_ReturnsUserError) {
  // Prepare
  const char* layout_yaml =
      "layout:\n"
      "  canvas:\n"
      "    width: 8\n"
      "    height: 8\n"
      "  pixel_aspect: hello\n"
      "  image:\n"
      "    fit: \"contain\"\n"
      "  slate:\n"
      "    duration_frames: 0\n"
      "    lines: []\n";
  const std::filesystem::path yaml = write_contract_job(
      "non_number_pixel_aspect.yaml", {.layout_yaml = layout_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_59));
  expect_user_message(schema, "layout.pixel_aspect is string \"hello\"");
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_59));
  expect_user_message(job.status(),
                      "layout.pixel_aspect is unquoted string \"hello\"");
}

/*!
 * \brief Accepts \c pixel_aspect as a number.
 */
TEST(JobLoader, ValidateJobSchema_NumberPixelAspect_Succeeds) {
  // Prepare
  const char* layout_yaml =
      "layout:\n"
      "  canvas:\n"
      "    width: 8\n"
      "    height: 8\n"
      "  pixel_aspect: 2.0\n"
      "  image:\n"
      "    fit: \"contain\"\n"
      "  slate:\n"
      "    duration_frames: 0\n"
      "    lines: []\n";
  const std::filesystem::path yaml = write_contract_job(
      "number_pixel_aspect.yaml", {.layout_yaml = layout_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  expect_near(job.value().layout().pixel_aspect().aspect(), 2.0);
}

/*!
 * \brief Accepts a layout block that includes \c image.
 */
TEST(JobLoader, ValidateJobSchema_PresentImage_Succeeds) {
  // Prepare
  const char* layout_yaml =
      "layout:\n"
      "  canvas:\n"
      "    width: 8\n"
      "    height: 8\n"
      "  image:\n"
      "    fit: \"contain\"\n"
      "  slate:\n"
      "    duration_frames: 0\n"
      "    lines: []\n";
  const std::filesystem::path yaml =
      write_contract_job("with_image.yaml", {.layout_yaml = layout_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  EXPECT_EQ(job.value().layout().image().fit(),
            dailyboy::JobLayoutImage::JobLayoutImageFitValue::Contain);
}

/*!
 * \brief Rejects a layout block that omits \c image.
 */
TEST(JobLoader, ValidateJobSchema_OmittedImage_ReturnsUserError) {
  // Prepare
  const char* layout_yaml =
      "layout:\n"
      "  canvas:\n"
      "    width: 8\n"
      "    height: 8\n"
      "  slate:\n"
      "    duration_frames: 0\n"
      "    lines: []\n";
  const std::filesystem::path yaml =
      write_contract_job("no_image.yaml", {.layout_yaml = layout_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_62));
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_62));
}

/*!
 * \brief Accepts a layout block that includes \c burn_ins.
 */
TEST(JobLoader, ValidateJobSchema_PresentBurnIns_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_contract_job("with_burn_ins.yaml",
                                 "  burn_ins:\n"
                                 "    - template: \"{frame}\"\n"
                                 "      position:\n"
                                 "        mode: \"layout\"\n"
                                 "        anchor: \"top_left\"\n"
                                 "      font:\n"
                                 "        path: \"/unused/font.ttf\"\n"
                                 "        size_px: 12\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  ASSERT_EQ(job.value().layout().burn_ins().burn_ins().size(), 1u);
}

/*!
 * \brief Accepts a layout block that omits \c burn_ins.
 */
TEST(JobLoader, ValidateJobSchema_OmittedBurnIns_Succeeds) {
  // Prepare
  const char* layout_yaml =
      "layout:\n"
      "  canvas:\n"
      "    width: 8\n"
      "    height: 8\n"
      "  image:\n"
      "    fit: \"contain\"\n"
      "  slate:\n"
      "    duration_frames: 0\n"
      "    lines: []\n";
  const std::filesystem::path yaml =
      write_contract_job("no_burn_ins.yaml", {.layout_yaml = layout_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  EXPECT_TRUE(job.value().layout().burn_ins().burn_ins().empty());
}

/*!
 * \brief Accepts \c burn_ins with at least one burn-in.
 */
TEST(JobLoader, ValidateJobSchema_BurnInsAtLeastOne_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_contract_job("burn_ins_at_least_one.yaml",
                                 "  burn_ins:\n"
                                 "    - template: \"{frame}\"\n"
                                 "      position:\n"
                                 "        mode: \"layout\"\n"
                                 "        anchor: \"top_left\"\n"
                                 "      font:\n"
                                 "        path: \"/unused/font.ttf\"\n"
                                 "        size_px: 12\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  EXPECT_GE(job.value().layout().burn_ins().burn_ins().size(), 1u);
}

/*!
 * \brief Rejects \c burn_ins that contains no burn-in.
 */
TEST(JobLoader, ValidateJobSchema_EmptyBurnIns_ReturnsUserError) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_contract_job("empty_burn_ins.yaml", "  burn_ins: []\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_72));
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_72));
}

/*!
 * \brief Accepts a layout block that includes \c slate.
 */
TEST(JobLoader, ValidateJobSchema_PresentSlate_Succeeds) {
  // Prepare
  const char* layout_yaml =
      "layout:\n"
      "  canvas:\n"
      "    width: 8\n"
      "    height: 8\n"
      "  image:\n"
      "    fit: \"contain\"\n"
      "  slate:\n"
      "    duration_frames: 0\n"
      "    lines: []\n";
  const std::filesystem::path yaml =
      write_contract_job("with_slate.yaml", {.layout_yaml = layout_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  EXPECT_EQ(job.value().layout().slate().duration_frames(), 0);
  EXPECT_TRUE(job.value().layout().slate().lines().empty());
}

/*!
 * \brief Accepts a layout block that omits \c slate.
 */
TEST(JobLoader, ValidateJobSchema_OmittedSlate_Succeeds) {
  // Prepare
  const char* layout_yaml =
      "layout:\n"
      "  canvas:\n"
      "    width: 8\n"
      "    height: 8\n"
      "  image:\n"
      "    fit: \"contain\"\n";
  const std::filesystem::path yaml =
      write_contract_job("no_slate.yaml", {.layout_yaml = layout_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  EXPECT_EQ(job.value().layout().slate().duration_frames(), 0);
  EXPECT_TRUE(job.value().layout().slate().lines().empty());
}

/*!
 * \brief Accepts a layout block that includes \c background.
 */
TEST(JobLoader, ValidateJobSchema_PresentBackground_Succeeds) {
  // Prepare
  const char* layout_yaml =
      "layout:\n"
      "  canvas:\n"
      "    width: 8\n"
      "    height: 8\n"
      "  image:\n"
      "    fit: \"contain\"\n"
      "  background:\n"
      "    r: 0.1\n"
      "    g: 0.2\n"
      "    b: 0.3\n"
      "  slate:\n"
      "    duration_frames: 0\n"
      "    lines: []\n";
  const std::filesystem::path yaml =
      write_contract_job("with_background.yaml", {.layout_yaml = layout_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  expect_rgb(job.value().layout().background(), 0.1, 0.2, 0.3);
}

/*!
 * \brief Accepts a layout block that omits \c background.
 */
TEST(JobLoader, ValidateJobSchema_OmittedBackground_Succeeds) {
  // Prepare
  const char* layout_yaml =
      "layout:\n"
      "  canvas:\n"
      "    width: 8\n"
      "    height: 8\n"
      "  image:\n"
      "    fit: \"contain\"\n"
      "  slate:\n"
      "    duration_frames: 0\n"
      "    lines: []\n";
  const std::filesystem::path yaml =
      write_contract_job("no_background.yaml", {.layout_yaml = layout_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  expect_rgb(job.value().layout().background(), 0.0, 0.0, 0.0);
}

/*!
 * \brief Accepts an \c image block that includes \c fit.
 */
TEST(JobLoader, ValidateJobSchema_PresentFit_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_image_contract_job("with_fit.yaml", "    fit: \"contain\"\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  EXPECT_EQ(job.value().layout().image().fit(),
            dailyboy::JobLayoutImage::JobLayoutImageFitValue::Contain);
}

/*!
 * \brief Rejects an \c image block that omits \c fit.
 */
TEST(JobLoader, ValidateJobSchema_OmittedFit_ReturnsUserError) {
  // Prepare
  const char* layout_yaml =
      "layout:\n"
      "  canvas:\n"
      "    width: 8\n"
      "    height: 8\n"
      "  image: {}\n";
  const std::filesystem::path yaml =
      write_contract_job("no_fit.yaml", {.layout_yaml = layout_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_64));
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_64));
}

/*!
 * \brief Rejects \c fit as an unquoted YAML scalar.
 */
TEST(JobLoader, ValidateJobSchema_UnquotedFit_ReturnsUserError) {
  // Prepare
  const std::filesystem::path yaml =
      write_image_contract_job("unquoted_fit.yaml", "    fit: contain\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_65));
  expect_user_message(schema,
                      "layout.image.fit is unquoted string \"contain\"");
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_65));
  expect_user_message(job.status(),
                      "layout.image.fit is unquoted string \"contain\"");
}

/*!
 * \brief Accepts \c fit as a quoted YAML string.
 */
TEST(JobLoader, ValidateJobSchema_QuotedFit_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_image_contract_job("quoted_fit.yaml", "    fit: \"contain\"\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  EXPECT_EQ(job.value().layout().image().fit(),
            dailyboy::JobLayoutImage::JobLayoutImageFitValue::Contain);
}

/*!
 * \brief Rejects \c fit that is not \c contain or \c cover.
 */
TEST(JobLoader, ValidateJobSchema_UnsupportedFit_ReturnsUserError) {
  // Prepare
  const std::filesystem::path yaml = write_image_contract_job(
      "unsupported_fit.yaml", "    fit: \"stretch\"\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_22));
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_22));
}

/*!
 * \brief Accepts \c fit \c contain.
 */
TEST(JobLoader, ValidateJobSchema_FitContain_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_image_contract_job("fit_contain.yaml", "    fit: \"contain\"\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  EXPECT_EQ(job.value().layout().image().fit(),
            dailyboy::JobLayoutImage::JobLayoutImageFitValue::Contain);
}

/*!
 * \brief Accepts \c fit \c cover.
 */
TEST(JobLoader, ValidateJobSchema_FitCover_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_image_contract_job("fit_cover.yaml", "    fit: \"cover\"\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  EXPECT_EQ(job.value().layout().image().fit(),
            dailyboy::JobLayoutImage::JobLayoutImageFitValue::Cover);
}

/*!
 * \brief Accepts an \c image block that includes \c filter.
 */
TEST(JobLoader, ValidateJobSchema_PresentFilter_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_image_contract_job("with_filter.yaml",
                               "    fit: \"contain\"\n"
                               "    filter: \"bilinear\"\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  EXPECT_EQ(job.value().layout().image().filter(),
            dailyboy::JobLayoutImage::JobLayoutImageFilterValue::Bilinear);
}

/*!
 * \brief Sets omitted \c filter to \c lanczos3.
 */
TEST(JobLoader, ValidateJobSchema_OmittedFilter_DefaultsToLanczos3) {
  // Prepare
  const std::filesystem::path yaml =
      write_image_contract_job("no_filter.yaml", "    fit: \"contain\"\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  EXPECT_EQ(job.value().layout().image().filter(),
            dailyboy::JobLayoutImage::JobLayoutImageFilterValue::Lanczos3);
}

/*!
 * \brief Rejects \c filter as an unquoted YAML scalar.
 */
TEST(JobLoader, ValidateJobSchema_UnquotedFilter_ReturnsUserError) {
  // Prepare
  const std::filesystem::path yaml =
      write_image_contract_job("unquoted_filter.yaml",
                               "    fit: \"contain\"\n"
                               "    filter: bilinear\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_66));
  expect_user_message(schema,
                      "layout.image.filter is unquoted string \"bilinear\"");
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_66));
  expect_user_message(job.status(),
                      "layout.image.filter is unquoted string \"bilinear\"");
}

/*!
 * \brief Accepts \c filter as a quoted YAML string.
 */
TEST(JobLoader, ValidateJobSchema_QuotedFilter_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_image_contract_job("quoted_filter.yaml",
                               "    fit: \"contain\"\n"
                               "    filter: \"lanczos3\"\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  EXPECT_EQ(job.value().layout().image().filter(),
            dailyboy::JobLayoutImage::JobLayoutImageFilterValue::Lanczos3);
}

/*!
 * \brief Rejects \c filter that is not \c bilinear or \c lanczos3.
 */
TEST(JobLoader, ValidateJobSchema_UnsupportedFilter_ReturnsUserError) {
  // Prepare
  const std::filesystem::path yaml =
      write_image_contract_job("unsupported_filter.yaml",
                               "    fit: \"contain\"\n"
                               "    filter: \"nearest\"\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_23));
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_23));
}

/*!
 * \brief Accepts \c filter \c bilinear.
 */
TEST(JobLoader, ValidateJobSchema_FilterBilinear_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_image_contract_job("filter_bilinear.yaml",
                               "    fit: \"contain\"\n"
                               "    filter: \"bilinear\"\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  EXPECT_EQ(job.value().layout().image().filter(),
            dailyboy::JobLayoutImage::JobLayoutImageFilterValue::Bilinear);
}

/*!
 * \brief Accepts \c filter \c lanczos3.
 */
TEST(JobLoader, ValidateJobSchema_FilterLanczos3_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_image_contract_job("filter_lanczos3.yaml",
                               "    fit: \"contain\"\n"
                               "    filter: \"lanczos3\"\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  EXPECT_EQ(job.value().layout().image().filter(),
            dailyboy::JobLayoutImage::JobLayoutImageFilterValue::Lanczos3);
}

/*!
 * \brief Accepts an \c image block that includes \c min_margin_px.
 */
TEST(JobLoader, ValidateJobSchema_PresentMinMarginPx_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_image_contract_job("with_min_margin_px.yaml",
                               "    fit: \"contain\"\n"
                               "    min_margin_px:\n"
                               "      top: 8\n"
                               "      right: 4\n"
                               "      bottom: 8\n"
                               "      left: 4\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  expect_margin(job.value().layout().image().min_margin_px(), 8, 4, 8, 4);
}

/*!
 * \brief Accepts an \c image block that omits \c min_margin_px.
 */
TEST(JobLoader, ValidateJobSchema_OmittedMinMarginPx_Succeeds) {
  // Prepare
  const std::filesystem::path yaml = write_image_contract_job(
      "no_min_margin_px.yaml", "    fit: \"contain\"\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  expect_margin(job.value().layout().image().min_margin_px(), 0, 0, 0, 0);
}

/*!
 * \brief Accepts \c min_margin_px as a map of side keys.
 */
TEST(JobLoader, ValidateJobSchema_MinMarginPxMap_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_image_contract_job("min_margin_px_map.yaml",
                               "    fit: \"contain\"\n"
                               "    min_margin_px:\n"
                               "      top: 2\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  expect_margin(job.value().layout().image().min_margin_px(), 2, 0, 0, 0);
}

/*!
 * \brief Rejects \c min_margin_px that is not a map.
 */
TEST(JobLoader, ValidateJobSchema_MinMarginPxNotMap_ReturnsUserError) {
  // Prepare
  const std::filesystem::path yaml =
      write_image_contract_job("min_margin_px_not_map.yaml",
                               "    fit: \"contain\"\n"
                               "    min_margin_px: 8\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_67));
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_67));
}

/*!
 * \brief Rejects \c min_margin_px keys other than top, right, bottom, left.
 */
TEST(JobLoader, ValidateJobSchema_MinMarginPxUnknownKey_ReturnsUserError) {
  // Prepare
  const std::filesystem::path yaml =
      write_image_contract_job("min_margin_px_unknown_key.yaml",
                               "    fit: \"contain\"\n"
                               "    min_margin_px:\n"
                               "      top: 8\n"
                               "      diag: 1\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_40));
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_40));
}

/*!
 * \brief Accepts integer \c min_margin_px side values.
 */
TEST(JobLoader, ValidateJobSchema_MinMarginPxIntegerValues_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_image_contract_job("min_margin_px_integers.yaml",
                               "    fit: \"contain\"\n"
                               "    min_margin_px:\n"
                               "      top: 1\n"
                               "      right: 2\n"
                               "      bottom: 3\n"
                               "      left: 4\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  expect_margin(job.value().layout().image().min_margin_px(), 1, 2, 3, 4);
}

/*!
 * \brief Rejects non-integer \c min_margin_px side values.
 */
TEST(JobLoader,
     ValidateJobSchema_MinMarginPxNonIntegerValues_ReturnsUserError) {
  // Prepare
  const std::filesystem::path yaml =
      write_image_contract_job("min_margin_px_non_integer.yaml",
                               "    fit: \"contain\"\n"
                               "    min_margin_px:\n"
                               "      top: 8.5\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_68));
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_68));
}

/*!
 * \brief Accepts \c min_margin_px with only \c top.
 */
TEST(JobLoader, ValidateJobSchema_MinMarginPxTopOnly_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_image_contract_job("min_margin_px_top.yaml",
                               "    fit: \"contain\"\n"
                               "    min_margin_px:\n"
                               "      top: 6\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  expect_margin(job.value().layout().image().min_margin_px(), 6, 0, 0, 0);
}

/*!
 * \brief Accepts \c min_margin_px with only \c right.
 */
TEST(JobLoader, ValidateJobSchema_MinMarginPxRightOnly_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_image_contract_job("min_margin_px_right.yaml",
                               "    fit: \"contain\"\n"
                               "    min_margin_px:\n"
                               "      right: 6\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  expect_margin(job.value().layout().image().min_margin_px(), 0, 6, 0, 0);
}

/*!
 * \brief Accepts \c min_margin_px with only \c bottom.
 */
TEST(JobLoader, ValidateJobSchema_MinMarginPxBottomOnly_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_image_contract_job("min_margin_px_bottom.yaml",
                               "    fit: \"contain\"\n"
                               "    min_margin_px:\n"
                               "      bottom: 6\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  expect_margin(job.value().layout().image().min_margin_px(), 0, 0, 6, 0);
}

/*!
 * \brief Accepts \c min_margin_px with only \c left.
 */
TEST(JobLoader, ValidateJobSchema_MinMarginPxLeftOnly_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_image_contract_job("min_margin_px_left.yaml",
                               "    fit: \"contain\"\n"
                               "    min_margin_px:\n"
                               "      left: 6\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  expect_margin(job.value().layout().image().min_margin_px(), 0, 0, 0, 6);
}

/*!
 * \brief Sets omitted \c top to 0; the other sides keep their values.
 */
TEST(JobLoader, ValidateJobSchema_OmittedMinMarginPxTop_DefaultsTo0) {
  // Prepare
  const std::filesystem::path yaml =
      write_image_contract_job("omitted_min_margin_px_top.yaml",
                               "    fit: \"contain\"\n"
                               "    min_margin_px:\n"
                               "      right: 2\n"
                               "      bottom: 3\n"
                               "      left: 4\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  const dailyboy::Margin& margin = job.value().layout().image().min_margin_px();
  EXPECT_EQ(margin.top(), 0);
  EXPECT_EQ(margin.right(), 2);
  EXPECT_EQ(margin.bottom(), 3);
  EXPECT_EQ(margin.left(), 4);
}

/*!
 * \brief Sets omitted \c right to 0; the other sides keep their values.
 */
TEST(JobLoader, ValidateJobSchema_OmittedMinMarginPxRight_DefaultsTo0) {
  // Prepare
  const std::filesystem::path yaml =
      write_image_contract_job("omitted_min_margin_px_right.yaml",
                               "    fit: \"contain\"\n"
                               "    min_margin_px:\n"
                               "      top: 1\n"
                               "      bottom: 3\n"
                               "      left: 4\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  const dailyboy::Margin& margin = job.value().layout().image().min_margin_px();
  EXPECT_EQ(margin.top(), 1);
  EXPECT_EQ(margin.right(), 0);
  EXPECT_EQ(margin.bottom(), 3);
  EXPECT_EQ(margin.left(), 4);
}

/*!
 * \brief Sets omitted \c bottom to 0; the other sides keep their values.
 */
TEST(JobLoader, ValidateJobSchema_OmittedMinMarginPxBottom_DefaultsTo0) {
  // Prepare
  const std::filesystem::path yaml =
      write_image_contract_job("omitted_min_margin_px_bottom.yaml",
                               "    fit: \"contain\"\n"
                               "    min_margin_px:\n"
                               "      top: 1\n"
                               "      right: 2\n"
                               "      left: 4\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  const dailyboy::Margin& margin = job.value().layout().image().min_margin_px();
  EXPECT_EQ(margin.top(), 1);
  EXPECT_EQ(margin.right(), 2);
  EXPECT_EQ(margin.bottom(), 0);
  EXPECT_EQ(margin.left(), 4);
}

/*!
 * \brief Sets omitted \c left to 0; the other sides keep their values.
 */
TEST(JobLoader, ValidateJobSchema_OmittedMinMarginPxLeft_DefaultsTo0) {
  // Prepare
  const std::filesystem::path yaml =
      write_image_contract_job("omitted_min_margin_px_left.yaml",
                               "    fit: \"contain\"\n"
                               "    min_margin_px:\n"
                               "      top: 1\n"
                               "      right: 2\n"
                               "      bottom: 3\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  const dailyboy::Margin& margin = job.value().layout().image().min_margin_px();
  EXPECT_EQ(margin.top(), 1);
  EXPECT_EQ(margin.right(), 2);
  EXPECT_EQ(margin.bottom(), 3);
  EXPECT_EQ(margin.left(), 0);
}

/*!
 * \brief Accepts \c min_margin_px with all four sides.
 */
TEST(JobLoader, ValidateJobSchema_MinMarginPxAllSides_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_image_contract_job("min_margin_px_all_sides.yaml",
                               "    fit: \"contain\"\n"
                               "    min_margin_px:\n"
                               "      top: 1\n"
                               "      right: 2\n"
                               "      bottom: 3\n"
                               "      left: 4\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  expect_margin(job.value().layout().image().min_margin_px(), 1, 2, 3, 4);
}

/*!
 * \brief Accepts \c top of 0.
 */
TEST(JobLoader, ValidateJobSchema_MinMarginPxTopAtLeast0_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_image_contract_job("min_margin_px_top_0.yaml",
                               "    fit: \"contain\"\n"
                               "    min_margin_px:\n"
                               "      top: 0\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  EXPECT_EQ(job.value().layout().image().min_margin_px().top(), 0);
}

/*!
 * \brief Accepts \c right of 0.
 */
TEST(JobLoader, ValidateJobSchema_MinMarginPxRightAtLeast0_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_image_contract_job("min_margin_px_right_0.yaml",
                               "    fit: \"contain\"\n"
                               "    min_margin_px:\n"
                               "      right: 0\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  EXPECT_EQ(job.value().layout().image().min_margin_px().right(), 0);
}

/*!
 * \brief Accepts \c bottom of 0.
 */
TEST(JobLoader, ValidateJobSchema_MinMarginPxBottomAtLeast0_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_image_contract_job("min_margin_px_bottom_0.yaml",
                               "    fit: \"contain\"\n"
                               "    min_margin_px:\n"
                               "      bottom: 0\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  EXPECT_EQ(job.value().layout().image().min_margin_px().bottom(), 0);
}

/*!
 * \brief Accepts \c left of 0.
 */
TEST(JobLoader, ValidateJobSchema_MinMarginPxLeftAtLeast0_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_image_contract_job("min_margin_px_left_0.yaml",
                               "    fit: \"contain\"\n"
                               "    min_margin_px:\n"
                               "      left: 0\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  EXPECT_EQ(job.value().layout().image().min_margin_px().left(), 0);
}

/*!
 * \brief Rejects \c top below 0.
 */
TEST(JobLoader, ValidateJobSchema_MinMarginPxTopBelow0_ReturnsUserError) {
  // Prepare
  const std::filesystem::path yaml =
      write_image_contract_job("min_margin_px_top_neg.yaml",
                               "    fit: \"contain\"\n"
                               "    min_margin_px:\n"
                               "      top: -1\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_69));
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_69));
}

/*!
 * \brief Rejects \c right below 0.
 */
TEST(JobLoader, ValidateJobSchema_MinMarginPxRightBelow0_ReturnsUserError) {
  // Prepare
  const std::filesystem::path yaml =
      write_image_contract_job("min_margin_px_right_neg.yaml",
                               "    fit: \"contain\"\n"
                               "    min_margin_px:\n"
                               "      right: -1\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_69));
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_69));
}

/*!
 * \brief Rejects \c bottom below 0.
 */
TEST(JobLoader, ValidateJobSchema_MinMarginPxBottomBelow0_ReturnsUserError) {
  // Prepare
  const std::filesystem::path yaml =
      write_image_contract_job("min_margin_px_bottom_neg.yaml",
                               "    fit: \"contain\"\n"
                               "    min_margin_px:\n"
                               "      bottom: -1\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_69));
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_69));
}

/*!
 * \brief Rejects \c left below 0.
 */
TEST(JobLoader, ValidateJobSchema_MinMarginPxLeftBelow0_ReturnsUserError) {
  // Prepare
  const std::filesystem::path yaml =
      write_image_contract_job("min_margin_px_left_neg.yaml",
                               "    fit: \"contain\"\n"
                               "    min_margin_px:\n"
                               "      left: -1\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_69));
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_69));
}

/*!
 * \brief Accepts \c background as a map of r, g, b.
 */
TEST(JobLoader, ValidateJobSchema_BackgroundMap_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_background_contract_job("background_map.yaml",
                                    "  background:\n"
                                    "    r: 0.1\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  expect_rgb(job.value().layout().background(), 0.1, 0.0, 0.0);
}

/*!
 * \brief Rejects \c background that is not a map.
 */
TEST(JobLoader, ValidateJobSchema_BackgroundNotMap_ReturnsUserError) {
  // Prepare
  const std::filesystem::path yaml = write_background_contract_job(
      "background_not_map.yaml", "  background: 0.5\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_70));
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_70));
}

/*!
 * \brief Rejects \c background keys other than r, g, b.
 */
TEST(JobLoader, ValidateJobSchema_BackgroundUnknownKey_ReturnsUserError) {
  // Prepare
  const std::filesystem::path yaml =
      write_background_contract_job("background_unknown_key.yaml",
                                    "  background:\n"
                                    "    r: 0.1\n"
                                    "    alpha: 1\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_40));
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_40));
}

/*!
 * \brief Sets omitted \c r to 0; the other channels keep their values.
 */
TEST(JobLoader, ValidateJobSchema_OmittedBackgroundR_DefaultsTo0) {
  // Prepare
  const std::filesystem::path yaml =
      write_background_contract_job("omitted_background_r.yaml",
                                    "  background:\n"
                                    "    g: 0.2\n"
                                    "    b: 0.3\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  const dailyboy::JobLayoutBackground& bg = job.value().layout().background();
  EXPECT_EQ(bg.r(), 0.0);
  expect_near(bg.g(), 0.2);
  expect_near(bg.b(), 0.3);
}

/*!
 * \brief Sets omitted \c g to 0; the other channels keep their values.
 */
TEST(JobLoader, ValidateJobSchema_OmittedBackgroundG_DefaultsTo0) {
  // Prepare
  const std::filesystem::path yaml =
      write_background_contract_job("omitted_background_g.yaml",
                                    "  background:\n"
                                    "    r: 0.1\n"
                                    "    b: 0.3\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  const dailyboy::JobLayoutBackground& bg = job.value().layout().background();
  expect_near(bg.r(), 0.1);
  EXPECT_EQ(bg.g(), 0.0);
  expect_near(bg.b(), 0.3);
}

/*!
 * \brief Sets omitted \c b to 0; the other channels keep their values.
 */
TEST(JobLoader, ValidateJobSchema_OmittedBackgroundB_DefaultsTo0) {
  // Prepare
  const std::filesystem::path yaml =
      write_background_contract_job("omitted_background_b.yaml",
                                    "  background:\n"
                                    "    r: 0.1\n"
                                    "    g: 0.2\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  const dailyboy::JobLayoutBackground& bg = job.value().layout().background();
  expect_near(bg.r(), 0.1);
  expect_near(bg.g(), 0.2);
  EXPECT_EQ(bg.b(), 0.0);
}

/*!
 * \brief Accepts \c background that includes \c r.
 */
TEST(JobLoader, ValidateJobSchema_PresentBackgroundR_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_background_contract_job("present_background_r.yaml",
                                    "  background:\n"
                                    "    r: 0.4\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  expect_rgb(job.value().layout().background(), 0.4, 0.0, 0.0);
}

/*!
 * \brief Accepts \c background that includes \c g.
 */
TEST(JobLoader, ValidateJobSchema_PresentBackgroundG_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_background_contract_job("present_background_g.yaml",
                                    "  background:\n"
                                    "    g: 0.5\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  expect_rgb(job.value().layout().background(), 0.0, 0.5, 0.0);
}

/*!
 * \brief Accepts \c background that includes \c b.
 */
TEST(JobLoader, ValidateJobSchema_PresentBackgroundB_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_background_contract_job("present_background_b.yaml",
                                    "  background:\n"
                                    "    b: 0.6\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  expect_rgb(job.value().layout().background(), 0.0, 0.0, 0.6);
}

/*!
 * \brief Rejects \c r that is not a number between 0 and 1.
 */
TEST(JobLoader, ValidateJobSchema_BackgroundRNotInRange_ReturnsUserError) {
  // Prepare
  const std::filesystem::path yaml =
      write_background_contract_job("background_r_out_of_range.yaml",
                                    "  background:\n"
                                    "    r: 1.5\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_71));
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_71));
}

/*!
 * \brief Rejects \c g that is not a number between 0 and 1.
 */
TEST(JobLoader, ValidateJobSchema_BackgroundGNotInRange_ReturnsUserError) {
  // Prepare
  const std::filesystem::path yaml =
      write_background_contract_job("background_g_out_of_range.yaml",
                                    "  background:\n"
                                    "    g: 1.5\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_71));
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_71));
}

/*!
 * \brief Rejects \c b that is not a number between 0 and 1.
 */
TEST(JobLoader, ValidateJobSchema_BackgroundBNotInRange_ReturnsUserError) {
  // Prepare
  const std::filesystem::path yaml =
      write_background_contract_job("background_b_out_of_range.yaml",
                                    "  background:\n"
                                    "    b: 1.5\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_71));
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_71));
}

/*!
 * \brief Accepts \c r as a number between 0 and 1.
 */
TEST(JobLoader, ValidateJobSchema_BackgroundRInRange_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_background_contract_job("background_r_in_range.yaml",
                                    "  background:\n"
                                    "    r: 0.0\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  expect_rgb(job.value().layout().background(), 0.0, 0.0, 0.0);
}

/*!
 * \brief Accepts \c g as a number between 0 and 1.
 */
TEST(JobLoader, ValidateJobSchema_BackgroundGInRange_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_background_contract_job("background_g_in_range.yaml",
                                    "  background:\n"
                                    "    g: 0.5\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  expect_rgb(job.value().layout().background(), 0.0, 0.5, 0.0);
}

/*!
 * \brief Accepts \c b as a number between 0 and 1.
 */
TEST(JobLoader, ValidateJobSchema_BackgroundBInRange_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_background_contract_job("background_b_in_range.yaml",
                                    "  background:\n"
                                    "    b: 1.0\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  expect_rgb(job.value().layout().background(), 0.0, 0.0, 1.0);
}

/*!
 * \brief Accepts decimal \c r, \c g, and \c b.
 */
TEST(JobLoader, ValidateJobSchema_BackgroundDecimalChannels_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_background_contract_job("background_decimal.yaml",
                                    "  background:\n"
                                    "    r: 0.12\n"
                                    "    g: 0.14\n"
                                    "    b: 0.18\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  expect_rgb(job.value().layout().background(), 0.12, 0.14, 0.18);
}

/*!
 * \brief Accepts a burn-in that includes \c template.
 */
TEST(JobLoader, ValidateJobSchema_PresentBurnInTemplate_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_contract_job("present_burn_in_template.yaml",
                                 "  burn_ins:\n"
                                 "    - template: \"{frame}\"\n"
                                 "      position:\n"
                                 "        mode: \"layout\"\n"
                                 "        anchor: \"top_left\"\n"
                                 "      font:\n"
                                 "        path: \"/unused/font.ttf\"\n"
                                 "        size_px: 12\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  EXPECT_EQ(job.value().layout().burn_ins().burn_ins().front().template_text(),
            "{frame}");
}

/*!
 * \brief Rejects a burn-in that omits \c template.
 */
TEST(JobLoader, ValidateJobSchema_OmittedBurnInTemplate_ReturnsUserError) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_contract_job("omitted_burn_in_template.yaml",
                                 "  burn_ins:\n"
                                 "    - position:\n"
                                 "        mode: \"layout\"\n"
                                 "        anchor: \"top_left\"\n"
                                 "      font:\n"
                                 "        path: \"/unused/font.ttf\"\n"
                                 "        size_px: 12\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_73));
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_73));
}

/*!
 * \brief Accepts a burn-in that includes \c position.
 */
TEST(JobLoader, ValidateJobSchema_PresentBurnInPosition_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_contract_job("present_burn_in_position.yaml",
                                 "  burn_ins:\n"
                                 "    - template: \"{frame}\"\n"
                                 "      position:\n"
                                 "        mode: \"layout\"\n"
                                 "        anchor: \"top_left\"\n"
                                 "      font:\n"
                                 "        path: \"/unused/font.ttf\"\n"
                                 "        size_px: 12\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  expect_layout_anchor(
      job.value().layout().burn_ins().burn_ins().front().position(),
      dailyboy::TextPositionModeLayout::Anchor::TopLeft);
}

/*!
 * \brief Rejects a burn-in that omits \c position.
 */
TEST(JobLoader, ValidateJobSchema_OmittedBurnInPosition_ReturnsUserError) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_contract_job("omitted_burn_in_position.yaml",
                                 "  burn_ins:\n"
                                 "    - template: \"{frame}\"\n"
                                 "      font:\n"
                                 "        path: \"/unused/font.ttf\"\n"
                                 "        size_px: 12\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_74));
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_74));
}

/*!
 * \brief Accepts a burn-in that includes \c font.
 */
TEST(JobLoader, ValidateJobSchema_PresentBurnInFont_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_contract_job("present_burn_in_font.yaml",
                                 "  burn_ins:\n"
                                 "    - template: \"{frame}\"\n"
                                 "      position:\n"
                                 "        mode: \"layout\"\n"
                                 "        anchor: \"top_left\"\n"
                                 "      font:\n"
                                 "        path: \"/unused/font.ttf\"\n"
                                 "        size_px: 12\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  expect_font(job.value().layout().burn_ins().burn_ins().front().font(),
              "/unused/font.ttf", 12);
}

/*!
 * \brief Rejects a burn-in that omits \c font.
 */
TEST(JobLoader, ValidateJobSchema_OmittedBurnInFont_ReturnsUserError) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_contract_job("omitted_burn_in_font.yaml",
                                 "  burn_ins:\n"
                                 "    - template: \"{frame}\"\n"
                                 "      position:\n"
                                 "        mode: \"layout\"\n"
                                 "        anchor: \"top_left\"\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_75));
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_75));
}

/*!
 * \brief Accepts a burn-in that includes \c box.
 */
TEST(JobLoader, ValidateJobSchema_PresentBurnInBox_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_contract_job("present_burn_in_box.yaml",
                                 "  burn_ins:\n"
                                 "    - template: \"{frame}\"\n"
                                 "      position:\n"
                                 "        mode: \"layout\"\n"
                                 "        anchor: \"top_left\"\n"
                                 "      font:\n"
                                 "        path: \"/unused/font.ttf\"\n"
                                 "        size_px: 12\n"
                                 "      box:\n"
                                 "        mode: \"fill\"\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  ASSERT_TRUE(
      job.value().layout().burn_ins().burn_ins().front().box().has_value());
}

/*!
 * \brief Accepts a burn-in that omits \c box.
 */
TEST(JobLoader, ValidateJobSchema_OmittedBurnInBox_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_contract_job("omitted_burn_in_box.yaml",
                                 "  burn_ins:\n"
                                 "    - template: \"{frame}\"\n"
                                 "      position:\n"
                                 "        mode: \"layout\"\n"
                                 "        anchor: \"top_left\"\n"
                                 "      font:\n"
                                 "        path: \"/unused/font.ttf\"\n"
                                 "        size_px: 12\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  EXPECT_FALSE(
      job.value().layout().burn_ins().burn_ins().front().box().has_value());
}

/*!
 * \brief Accepts \c template as a quoted YAML string.
 */
TEST(JobLoader, ValidateJobSchema_QuotedBurnInTemplate_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_contract_job("quoted_burn_in_template.yaml",
                                 "  burn_ins:\n"
                                 "    - template: \"{frame}\"\n"
                                 "      position:\n"
                                 "        mode: \"layout\"\n"
                                 "        anchor: \"top_left\"\n"
                                 "      font:\n"
                                 "        path: \"/unused/font.ttf\"\n"
                                 "        size_px: 12\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  EXPECT_EQ(job.value().layout().burn_ins().burn_ins().front().template_text(),
            "{frame}");
}

/*!
 * \brief Rejects \c template as an unquoted YAML scalar.
 */
TEST(JobLoader, ValidateJobSchema_UnquotedBurnInTemplate_ReturnsUserError) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_contract_job("unquoted_burn_in_template.yaml",
                                 "  burn_ins:\n"
                                 "    - template: hello\n"
                                 "      position:\n"
                                 "        mode: \"layout\"\n"
                                 "        anchor: \"top_left\"\n"
                                 "      font:\n"
                                 "        path: \"/unused/font.ttf\"\n"
                                 "        size_px: 12\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_76));
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_76));
}

/*!
 * \brief Rejects \c position.mode that is not layout, pixel, or percent.
 */
TEST(JobLoader,
     ValidateJobSchema_UnsupportedBurnInPositionMode_ReturnsUserError) {
  // Prepare
  const std::filesystem::path yaml = write_burn_in_position_job(
      "unsupported_burn_in_position_mode.yaml", "        mode: \"stretch\"\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_20));
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_20));
}

/*!
 * \brief Rejects \c position.mode as an unquoted YAML scalar.
 */
TEST(JobLoader, ValidateJobSchema_UnquotedBurnInPositionMode_ReturnsUserError) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_position_job("unquoted_burn_in_position_mode.yaml",
                                 "        mode: layout\n"
                                 "        anchor: \"top_left\"\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_77));
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_77));
}

/*!
 * \brief Accepts \c position.mode as a quoted YAML string.
 */
TEST(JobLoader, ValidateJobSchema_QuotedBurnInPositionMode_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_position_job("quoted_burn_in_position_mode.yaml",
                                 "        mode: \"layout\"\n"
                                 "        anchor: \"top_left\"\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  EXPECT_EQ(
      job.value().layout().burn_ins().burn_ins().front().position().mode(),
      dailyboy::TextPosition::Mode::Layout);
}

/*!
 * \brief Accepts \c position.mode \c layout.
 */
TEST(JobLoader, ValidateJobSchema_BurnInPositionModeLayout_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_position_job("burn_in_position_mode_layout.yaml",
                                 "        mode: \"layout\"\n"
                                 "        anchor: \"top_left\"\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  EXPECT_EQ(
      job.value().layout().burn_ins().burn_ins().front().position().mode(),
      dailyboy::TextPosition::Mode::Layout);
}

/*!
 * \brief Accepts \c position.mode \c pixel.
 */
TEST(JobLoader, ValidateJobSchema_BurnInPositionModePixel_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_position_job("burn_in_position_mode_pixel.yaml",
                                 "        mode: \"pixel\"\n"
                                 "        x: 4\n"
                                 "        y: 4\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  EXPECT_EQ(
      job.value().layout().burn_ins().burn_ins().front().position().mode(),
      dailyboy::TextPosition::Mode::Pixel);
}

/*!
 * \brief Accepts \c position.mode \c percent.
 */
TEST(JobLoader, ValidateJobSchema_BurnInPositionModePercent_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_position_job("burn_in_position_mode_percent.yaml",
                                 "        mode: \"percent\"\n"
                                 "        x: 50\n"
                                 "        y: 50\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  EXPECT_EQ(
      job.value().layout().burn_ins().burn_ins().front().position().mode(),
      dailyboy::TextPosition::Mode::Percent);
}

/*!
 * \brief Rejects \c position.anchor when \c mode is not layout.
 */
TEST(JobLoader,
     ValidateJobSchema_BurnInPositionAnchorWithoutLayoutMode_ReturnsUserError) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_position_job("burn_in_position_anchor_without_layout.yaml",
                                 "        mode: \"pixel\"\n"
                                 "        anchor: \"top_left\"\n"
                                 "        x: 4\n"
                                 "        y: 4\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_79));
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_79));
}

/*!
 * \brief Accepts \c position.anchor when \c mode is layout.
 */
TEST(JobLoader, ValidateJobSchema_BurnInPositionAnchorWithLayoutMode_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_position_job("burn_in_position_anchor_with_layout.yaml",
                                 "        mode: \"layout\"\n"
                                 "        anchor: \"top_left\"\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  expect_layout_anchor(
      job.value().layout().burn_ins().burn_ins().front().position(),
      dailyboy::TextPositionModeLayout::Anchor::TopLeft);
}

/*!
 * \brief Rejects \c position.x and \c y when \c mode is layout.
 */
TEST(JobLoader,
     ValidateJobSchema_BurnInPositionXYWithLayoutMode_ReturnsUserError) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_position_job("burn_in_position_xy_with_layout.yaml",
                                 "        mode: \"layout\"\n"
                                 "        anchor: \"top_left\"\n"
                                 "        x: 4\n"
                                 "        y: 4\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_80));
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_80));
}

/*!
 * \brief Accepts \c position.x and \c y when \c mode is pixel.
 */
TEST(JobLoader, ValidateJobSchema_BurnInPositionXYWithPixelMode_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_position_job("burn_in_position_xy_with_pixel.yaml",
                                 "        mode: \"pixel\"\n"
                                 "        x: 4\n"
                                 "        y: 4\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  expect_pixel_position(
      job.value().layout().burn_ins().burn_ins().front().position(), 4, 4);
}

/*!
 * \brief Accepts \c position.x and \c y when \c mode is percent.
 */
TEST(JobLoader, ValidateJobSchema_BurnInPositionXYWithPercentMode_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_position_job("burn_in_position_xy_with_percent.yaml",
                                 "        mode: \"percent\"\n"
                                 "        x: 50\n"
                                 "        y: 50\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  expect_percent_position(
      job.value().layout().burn_ins().burn_ins().front().position(), 50, 50);
}

/*!
 * \brief Rejects \c position.anchor that is not a nine-point name.
 */
TEST(JobLoader,
     ValidateJobSchema_UnsupportedBurnInPositionAnchor_ReturnsUserError) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_position_job("unsupported_burn_in_position_anchor.yaml",
                                 "        mode: \"layout\"\n"
                                 "        anchor: \"center\"\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_19));
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_19));
}

/*!
 * \brief Accepts \c position.anchor \c top_left.
 */
TEST(JobLoader, ValidateJobSchema_BurnInPositionAnchorTopLeft_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_position_job("burn_in_position_anchor_top_left.yaml",
                                 "        mode: \"layout\"\n"
                                 "        anchor: \"top_left\"\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  expect_layout_anchor(
      job.value().layout().burn_ins().burn_ins().front().position(),
      dailyboy::TextPositionModeLayout::Anchor::TopLeft);
}

/*!
 * \brief Accepts \c position.anchor \c top_center.
 */
TEST(JobLoader, ValidateJobSchema_BurnInPositionAnchorTopCenter_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_position_job("burn_in_position_anchor_top_center.yaml",
                                 "        mode: \"layout\"\n"
                                 "        anchor: \"top_center\"\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  expect_layout_anchor(
      job.value().layout().burn_ins().burn_ins().front().position(),
      dailyboy::TextPositionModeLayout::Anchor::TopCenter);
}

/*!
 * \brief Accepts \c position.anchor \c top_right.
 */
TEST(JobLoader, ValidateJobSchema_BurnInPositionAnchorTopRight_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_position_job("burn_in_position_anchor_top_right.yaml",
                                 "        mode: \"layout\"\n"
                                 "        anchor: \"top_right\"\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  expect_layout_anchor(
      job.value().layout().burn_ins().burn_ins().front().position(),
      dailyboy::TextPositionModeLayout::Anchor::TopRight);
}

/*!
 * \brief Accepts \c position.anchor \c center_left.
 */
TEST(JobLoader, ValidateJobSchema_BurnInPositionAnchorCenterLeft_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_position_job("burn_in_position_anchor_center_left.yaml",
                                 "        mode: \"layout\"\n"
                                 "        anchor: \"center_left\"\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  expect_layout_anchor(
      job.value().layout().burn_ins().burn_ins().front().position(),
      dailyboy::TextPositionModeLayout::Anchor::CenterLeft);
}

/*!
 * \brief Accepts \c position.anchor \c center_center.
 */
TEST(JobLoader, ValidateJobSchema_BurnInPositionAnchorCenterCenter_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_position_job("burn_in_position_anchor_center_center.yaml",
                                 "        mode: \"layout\"\n"
                                 "        anchor: \"center_center\"\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  expect_layout_anchor(
      job.value().layout().burn_ins().burn_ins().front().position(),
      dailyboy::TextPositionModeLayout::Anchor::CenterCenter);
}

/*!
 * \brief Accepts \c position.anchor \c center_right.
 */
TEST(JobLoader, ValidateJobSchema_BurnInPositionAnchorCenterRight_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_position_job("burn_in_position_anchor_center_right.yaml",
                                 "        mode: \"layout\"\n"
                                 "        anchor: \"center_right\"\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  expect_layout_anchor(
      job.value().layout().burn_ins().burn_ins().front().position(),
      dailyboy::TextPositionModeLayout::Anchor::CenterRight);
}

/*!
 * \brief Accepts \c position.anchor \c bottom_left.
 */
TEST(JobLoader, ValidateJobSchema_BurnInPositionAnchorBottomLeft_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_position_job("burn_in_position_anchor_bottom_left.yaml",
                                 "        mode: \"layout\"\n"
                                 "        anchor: \"bottom_left\"\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  expect_layout_anchor(
      job.value().layout().burn_ins().burn_ins().front().position(),
      dailyboy::TextPositionModeLayout::Anchor::BottomLeft);
}

/*!
 * \brief Accepts \c position.anchor \c bottom_center.
 */
TEST(JobLoader, ValidateJobSchema_BurnInPositionAnchorBottomCenter_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_position_job("burn_in_position_anchor_bottom_center.yaml",
                                 "        mode: \"layout\"\n"
                                 "        anchor: \"bottom_center\"\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  expect_layout_anchor(
      job.value().layout().burn_ins().burn_ins().front().position(),
      dailyboy::TextPositionModeLayout::Anchor::BottomCenter);
}

/*!
 * \brief Accepts \c position.anchor \c bottom_right.
 */
TEST(JobLoader, ValidateJobSchema_BurnInPositionAnchorBottomRight_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_position_job("burn_in_position_anchor_bottom_right.yaml",
                                 "        mode: \"layout\"\n"
                                 "        anchor: \"bottom_right\"\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  expect_layout_anchor(
      job.value().layout().burn_ins().burn_ins().front().position(),
      dailyboy::TextPositionModeLayout::Anchor::BottomRight);
}

/*!
 * \brief Rejects \c position.anchor as an unquoted YAML scalar.
 */
TEST(JobLoader,
     ValidateJobSchema_UnquotedBurnInPositionAnchor_ReturnsUserError) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_position_job("unquoted_burn_in_position_anchor.yaml",
                                 "        mode: \"layout\"\n"
                                 "        anchor: top_left\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_78));
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_78));
}

/*!
 * \brief Accepts \c position.anchor as a quoted YAML string.
 */
TEST(JobLoader, ValidateJobSchema_QuotedBurnInPositionAnchor_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_position_job("quoted_burn_in_position_anchor.yaml",
                                 "        mode: \"layout\"\n"
                                 "        anchor: \"top_left\"\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  expect_layout_anchor(
      job.value().layout().burn_ins().burn_ins().front().position(),
      dailyboy::TextPositionModeLayout::Anchor::TopLeft);
}

/*!
 * \brief Rejects \c position.x that is not an integer.
 */
TEST(JobLoader, ValidateJobSchema_NonIntegerBurnInPositionX_ReturnsUserError) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_position_job("non_integer_burn_in_position_x.yaml",
                                 "        mode: \"pixel\"\n"
                                 "        x: 1.5\n"
                                 "        y: 4\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_81));
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_81));
}

/*!
 * \brief Rejects \c position.y that is not an integer.
 */
TEST(JobLoader, ValidateJobSchema_NonIntegerBurnInPositionY_ReturnsUserError) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_position_job("non_integer_burn_in_position_y.yaml",
                                 "        mode: \"pixel\"\n"
                                 "        x: 4\n"
                                 "        y: 1.5\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_82));
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_82));
}

/*!
 * \brief Accepts integer \c position.x.
 */
TEST(JobLoader, ValidateJobSchema_IntegerBurnInPositionX_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_position_job("integer_burn_in_position_x.yaml",
                                 "        mode: \"pixel\"\n"
                                 "        x: 3\n"
                                 "        y: 4\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  expect_pixel_position(
      job.value().layout().burn_ins().burn_ins().front().position(), 3, 4);
}

/*!
 * \brief Accepts integer \c position.y.
 */
TEST(JobLoader, ValidateJobSchema_IntegerBurnInPositionY_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_position_job("integer_burn_in_position_y.yaml",
                                 "        mode: \"pixel\"\n"
                                 "        x: 4\n"
                                 "        y: 3\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  expect_pixel_position(
      job.value().layout().burn_ins().burn_ins().front().position(), 4, 3);
}

/*!
 * \brief Rejects pixel \c position.x outside \c [0, canvas.width].
 */
TEST(JobLoader,
     ValidateJobSchema_BurnInPositionPixelXOutOfRange_ReturnsUserError) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_position_job("burn_in_position_pixel_x_out_of_range.yaml",
                                 "        mode: \"pixel\"\n"
                                 "        x: 9\n"
                                 "        y: 4\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_83));
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_83));
}

/*!
 * \brief Rejects pixel \c position.y outside \c [0, canvas.height].
 */
TEST(JobLoader,
     ValidateJobSchema_BurnInPositionPixelYOutOfRange_ReturnsUserError) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_position_job("burn_in_position_pixel_y_out_of_range.yaml",
                                 "        mode: \"pixel\"\n"
                                 "        x: 4\n"
                                 "        y: 9\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_84));
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_84));
}

/*!
 * \brief Rejects percent \c position.x outside \c [0, 100].
 */
TEST(JobLoader,
     ValidateJobSchema_BurnInPositionPercentXOutOfRange_ReturnsUserError) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_position_job("burn_in_position_percent_x_out_of_range.yaml",
                                 "        mode: \"percent\"\n"
                                 "        x: 101\n"
                                 "        y: 50\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_85));
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_85));
}

/*!
 * \brief Rejects percent \c position.y outside \c [0, 100].
 */
TEST(JobLoader,
     ValidateJobSchema_BurnInPositionPercentYOutOfRange_ReturnsUserError) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_position_job("burn_in_position_percent_y_out_of_range.yaml",
                                 "        mode: \"percent\"\n"
                                 "        x: 50\n"
                                 "        y: 101\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_86));
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_86));
}

/*!
 * \brief Accepts pixel \c position.x in \c [0, canvas.width].
 */
TEST(JobLoader, ValidateJobSchema_BurnInPositionPixelXInRange_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_position_job("burn_in_position_pixel_x_in_range.yaml",
                                 "        mode: \"pixel\"\n"
                                 "        x: 0\n"
                                 "        y: 4\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  expect_pixel_position(
      job.value().layout().burn_ins().burn_ins().front().position(), 0, 4);
}

/*!
 * \brief Accepts pixel \c position.y in \c [0, canvas.height].
 */
TEST(JobLoader, ValidateJobSchema_BurnInPositionPixelYInRange_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_position_job("burn_in_position_pixel_y_in_range.yaml",
                                 "        mode: \"pixel\"\n"
                                 "        x: 4\n"
                                 "        y: 8\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  expect_pixel_position(
      job.value().layout().burn_ins().burn_ins().front().position(), 4, 8);
}

/*!
 * \brief Accepts percent \c position.x in \c [0, 100].
 */
TEST(JobLoader, ValidateJobSchema_BurnInPositionPercentXInRange_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_position_job("burn_in_position_percent_x_in_range.yaml",
                                 "        mode: \"percent\"\n"
                                 "        x: 0\n"
                                 "        y: 50\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  expect_percent_position(
      job.value().layout().burn_ins().burn_ins().front().position(), 0, 50);
}

/*!
 * \brief Accepts percent \c position.y in \c [0, 100].
 */
TEST(JobLoader, ValidateJobSchema_BurnInPositionPercentYInRange_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_position_job("burn_in_position_percent_y_in_range.yaml",
                                 "        mode: \"percent\"\n"
                                 "        x: 50\n"
                                 "        y: 100\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  expect_percent_position(
      job.value().layout().burn_ins().burn_ins().front().position(), 50, 100);
}

/*!
 * \brief Rejects \c font.path as an unquoted YAML scalar.
 */
TEST(JobLoader, ValidateJobSchema_UnquotedBurnInFontPath_ReturnsUserError) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_font_job("unquoted_burn_in_font_path.yaml",
                             "        path: /unused/font.ttf\n"
                             "        size_px: 12\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_87));
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_87));
}

/*!
 * \brief Accepts \c font.path as a quoted YAML string.
 */
TEST(JobLoader, ValidateJobSchema_QuotedBurnInFontPath_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_font_job("quoted_burn_in_font_path.yaml",
                             "        path: \"/unused/font.ttf\"\n"
                             "        size_px: 12\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  const dailyboy::TextFont& font =
      job.value().layout().burn_ins().burn_ins().front().font();
  EXPECT_EQ(font.path(), std::filesystem::path("/unused/font.ttf"));
}

/*!
 * \brief Rejects \c font.size_px that is not an integer.
 */
TEST(JobLoader, ValidateJobSchema_NonIntegerBurnInFontSizePx_ReturnsUserError) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_font_job("non_integer_burn_in_font_size_px.yaml",
                             "        path: \"/unused/font.ttf\"\n"
                             "        size_px: 12.5\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_88));
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_88));
}

/*!
 * \brief Accepts integer \c font.size_px.
 */
TEST(JobLoader, ValidateJobSchema_IntegerBurnInFontSizePx_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_font_job("integer_burn_in_font_size_px.yaml",
                             "        path: \"/unused/font.ttf\"\n"
                             "        size_px: 12\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  const dailyboy::TextFont& font =
      job.value().layout().burn_ins().burn_ins().front().font();
  EXPECT_EQ(font.size_px(), 12);
}

/*!
 * \brief Rejects \c font.size_px below 4.
 */
TEST(JobLoader, ValidateJobSchema_BurnInFontSizePxBelow4_ReturnsUserError) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_font_job("burn_in_font_size_px_below_4.yaml",
                             "        path: \"/unused/font.ttf\"\n"
                             "        size_px: 3\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_89));
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_89));
}

/*!
 * \brief Accepts \c font.size_px of at least 4.
 */
TEST(JobLoader, ValidateJobSchema_BurnInFontSizePxAtLeast4_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_font_job("burn_in_font_size_px_at_least_4.yaml",
                             "        path: \"/unused/font.ttf\"\n"
                             "        size_px: 4\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  const dailyboy::TextFont& font =
      job.value().layout().burn_ins().burn_ins().front().font();
  EXPECT_EQ(font.size_px(), 4);
}

/*!
 * \brief Accepts \c font with an object \c color.
 */
TEST(JobLoader, ValidateJobSchema_BurnInFontWithObjectColor_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_font_job("burn_in_font_with_object_color.yaml",
                             "        path: \"/unused/font.ttf\"\n"
                             "        size_px: 12\n"
                             "        color:\n"
                             "          r: 0.2\n"
                             "          g: 0.4\n"
                             "          b: 0.6\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  const dailyboy::TextFont& font =
      job.value().layout().burn_ins().burn_ins().front().font();
  expect_rgb(font.color(), 0.2, 0.4, 0.6);
}

/*!
 * \brief Accepts \c font that omits \c color.
 */
TEST(JobLoader, ValidateJobSchema_OmittedBurnInFontColor_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_font_job("omitted_burn_in_font_color.yaml",
                             "        path: \"/unused/font.ttf\"\n"
                             "        size_px: 12\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  const dailyboy::TextFont& font =
      job.value().layout().burn_ins().burn_ins().front().font();
  expect_rgb(font.color(), 1.0, 1.0, 1.0);
}

/*!
 * \brief Accepts \c font.color as a map of r, g, b.
 */
TEST(JobLoader, ValidateJobSchema_BurnInFontColorMap_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_font_job("burn_in_font_color_map.yaml",
                             "        path: \"/unused/font.ttf\"\n"
                             "        size_px: 12\n"
                             "        color:\n"
                             "          r: 0.1\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  const dailyboy::TextFont& font =
      job.value().layout().burn_ins().burn_ins().front().font();
  expect_rgb(font.color(), 0.1, 1.0, 1.0);
}

/*!
 * \brief Rejects \c font.color that is not a map.
 */
TEST(JobLoader, ValidateJobSchema_BurnInFontColorNotMap_ReturnsUserError) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_font_job("burn_in_font_color_not_map.yaml",
                             "        path: \"/unused/font.ttf\"\n"
                             "        size_px: 12\n"
                             "        color: 0.5\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_90));
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_90));
}

/*!
 * \brief Rejects \c font.color keys other than r, g, b.
 */
TEST(JobLoader, ValidateJobSchema_BurnInFontColorUnknownKey_ReturnsUserError) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_font_job("burn_in_font_color_unknown_key.yaml",
                             "        path: \"/unused/font.ttf\"\n"
                             "        size_px: 12\n"
                             "        color:\n"
                             "          r: 0.1\n"
                             "          alpha: 1\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_40));
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_40));
}

/*!
 * \brief Sets omitted \c r to 1; the other channels keep their values.
 */
TEST(JobLoader, ValidateJobSchema_OmittedBurnInFontColorR_DefaultsTo1) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_font_job("omitted_burn_in_font_color_r.yaml",
                             "        path: \"/unused/font.ttf\"\n"
                             "        size_px: 12\n"
                             "        color:\n"
                             "          g: 0.2\n"
                             "          b: 0.3\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  const dailyboy::TextFont& font =
      job.value().layout().burn_ins().burn_ins().front().font();
  EXPECT_EQ(font.color().r(), 1.0);
  expect_near(font.color().g(), 0.2);
  expect_near(font.color().b(), 0.3);
}

/*!
 * \brief Sets omitted \c g to 1; the other channels keep their values.
 */
TEST(JobLoader, ValidateJobSchema_OmittedBurnInFontColorG_DefaultsTo1) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_font_job("omitted_burn_in_font_color_g.yaml",
                             "        path: \"/unused/font.ttf\"\n"
                             "        size_px: 12\n"
                             "        color:\n"
                             "          r: 0.1\n"
                             "          b: 0.3\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  const dailyboy::TextFont& font =
      job.value().layout().burn_ins().burn_ins().front().font();
  EXPECT_EQ(font.color().g(), 1.0);
  expect_near(font.color().r(), 0.1);
  expect_near(font.color().b(), 0.3);
}

/*!
 * \brief Sets omitted \c b to 1; the other channels keep their values.
 */
TEST(JobLoader, ValidateJobSchema_OmittedBurnInFontColorB_DefaultsTo1) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_font_job("omitted_burn_in_font_color_b.yaml",
                             "        path: \"/unused/font.ttf\"\n"
                             "        size_px: 12\n"
                             "        color:\n"
                             "          r: 0.1\n"
                             "          g: 0.2\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  const dailyboy::TextFont& font =
      job.value().layout().burn_ins().burn_ins().front().font();
  EXPECT_EQ(font.color().b(), 1.0);
  expect_near(font.color().r(), 0.1);
  expect_near(font.color().g(), 0.2);
}

/*!
 * \brief Accepts \c font.color that includes \c r.
 */
TEST(JobLoader, ValidateJobSchema_PresentBurnInFontColorR_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_font_job("present_burn_in_font_color_r.yaml",
                             "        path: \"/unused/font.ttf\"\n"
                             "        size_px: 12\n"
                             "        color:\n"
                             "          r: 0.4\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  const dailyboy::TextFont& font =
      job.value().layout().burn_ins().burn_ins().front().font();
  expect_rgb(font.color(), 0.4, 1.0, 1.0);
}

/*!
 * \brief Accepts \c font.color that includes \c g.
 */
TEST(JobLoader, ValidateJobSchema_PresentBurnInFontColorG_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_font_job("present_burn_in_font_color_g.yaml",
                             "        path: \"/unused/font.ttf\"\n"
                             "        size_px: 12\n"
                             "        color:\n"
                             "          g: 0.5\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  const dailyboy::TextFont& font =
      job.value().layout().burn_ins().burn_ins().front().font();
  expect_rgb(font.color(), 1.0, 0.5, 1.0);
}

/*!
 * \brief Accepts \c font.color that includes \c b.
 */
TEST(JobLoader, ValidateJobSchema_PresentBurnInFontColorB_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_font_job("present_burn_in_font_color_b.yaml",
                             "        path: \"/unused/font.ttf\"\n"
                             "        size_px: 12\n"
                             "        color:\n"
                             "          b: 0.6\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  const dailyboy::TextFont& font =
      job.value().layout().burn_ins().burn_ins().front().font();
  expect_rgb(font.color(), 1.0, 1.0, 0.6);
}

/*!
 * \brief Rejects \c r that is not a number between 0 and 1.
 */
TEST(JobLoader, ValidateJobSchema_BurnInFontColorRNotInRange_ReturnsUserError) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_font_job("burn_in_font_color_r_out_of_range.yaml",
                             "        path: \"/unused/font.ttf\"\n"
                             "        size_px: 12\n"
                             "        color:\n"
                             "          r: 1.5\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_91));
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_91));
}

/*!
 * \brief Rejects \c g that is not a number between 0 and 1.
 */
TEST(JobLoader, ValidateJobSchema_BurnInFontColorGNotInRange_ReturnsUserError) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_font_job("burn_in_font_color_g_out_of_range.yaml",
                             "        path: \"/unused/font.ttf\"\n"
                             "        size_px: 12\n"
                             "        color:\n"
                             "          g: 1.5\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_91));
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_91));
}

/*!
 * \brief Rejects \c b that is not a number between 0 and 1.
 */
TEST(JobLoader, ValidateJobSchema_BurnInFontColorBNotInRange_ReturnsUserError) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_font_job("burn_in_font_color_b_out_of_range.yaml",
                             "        path: \"/unused/font.ttf\"\n"
                             "        size_px: 12\n"
                             "        color:\n"
                             "          b: 1.5\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_91));
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_91));
}

/*!
 * \brief Accepts \c r as a number between 0 and 1.
 */
TEST(JobLoader, ValidateJobSchema_BurnInFontColorRInRange_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_font_job("burn_in_font_color_r_in_range.yaml",
                             "        path: \"/unused/font.ttf\"\n"
                             "        size_px: 12\n"
                             "        color:\n"
                             "          r: 0.0\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  const dailyboy::TextFont& font =
      job.value().layout().burn_ins().burn_ins().front().font();
  expect_rgb(font.color(), 0.0, 1.0, 1.0);
}

/*!
 * \brief Accepts \c g as a number between 0 and 1.
 */
TEST(JobLoader, ValidateJobSchema_BurnInFontColorGInRange_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_font_job("burn_in_font_color_g_in_range.yaml",
                             "        path: \"/unused/font.ttf\"\n"
                             "        size_px: 12\n"
                             "        color:\n"
                             "          g: 0.5\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  const dailyboy::TextFont& font =
      job.value().layout().burn_ins().burn_ins().front().font();
  expect_rgb(font.color(), 1.0, 0.5, 1.0);
}

/*!
 * \brief Accepts \c b as a number between 0 and 1.
 */
TEST(JobLoader, ValidateJobSchema_BurnInFontColorBInRange_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_font_job("burn_in_font_color_b_in_range.yaml",
                             "        path: \"/unused/font.ttf\"\n"
                             "        size_px: 12\n"
                             "        color:\n"
                             "          b: 1.0\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  const dailyboy::TextFont& font =
      job.value().layout().burn_ins().burn_ins().front().font();
  expect_rgb(font.color(), 1.0, 1.0, 1.0);
}

/*!
 * \brief Accepts decimal \c r, \c g, and \c b.
 */
TEST(JobLoader, ValidateJobSchema_BurnInFontColorDecimalChannels_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_burn_in_font_job("burn_in_font_color_decimal.yaml",
                             "        path: \"/unused/font.ttf\"\n"
                             "        size_px: 12\n"
                             "        color:\n"
                             "          r: 0.12\n"
                             "          g: 0.14\n"
                             "          b: 0.18\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  const dailyboy::TextFont& font =
      job.value().layout().burn_ins().burn_ins().front().font();
  expect_rgb(font.color(), 0.12, 0.14, 0.18);
}

/*!
 * \brief Accepts a complete job that omits \c metadata.
 */
TEST(JobLoader, ValidateJobSchema_OmittedMetadata_Succeeds) {
  // Prepare
  const std::filesystem::path yaml = write_contract_job("no_metadata.yaml");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  EXPECT_TRUE(job.value().metadata().substitutions().empty());
}

/*!
 * \brief Accepts a complete job that includes \c metadata.substitutions.
 */
TEST(JobLoader, ValidateJobSchema_PresentMetadata_Succeeds) {
  // Prepare
  const char* metadata_yaml =
      "metadata:\n"
      "  substitutions:\n"
      "    project: \"DEMO\"\n";
  const std::filesystem::path yaml = write_contract_job(
      "with_metadata.yaml", {.metadata_yaml = metadata_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  EXPECT_EQ(expect_string_sub(job.value().metadata(), "project"), "DEMO");
}

/*!
 * \brief Rejects a complete job whose \c metadata omits \c substitutions.
 */
TEST(JobLoader, ValidateJobSchema_OmittedSubstitutions_ReturnsUserError) {
  // Prepare
  const char* metadata_yaml = "metadata: {}\n";
  const std::filesystem::path yaml = write_contract_job(
      "no_substitutions.yaml", {.metadata_yaml = metadata_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_5));
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_5));
}

/*!
 * \brief Accepts a complete job that includes \c metadata.substitutions.
 */
TEST(JobLoader, ValidateJobSchema_PresentSubstitutions_Succeeds) {
  // Prepare
  const char* metadata_yaml =
      "metadata:\n"
      "  substitutions:\n"
      "    project: \"DEMO\"\n";
  const std::filesystem::path yaml = write_contract_job(
      "with_substitutions.yaml", {.metadata_yaml = metadata_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  EXPECT_FALSE(job.value().metadata().substitutions().empty());
}

/*!
 * \brief Rejects a complete job whose \c metadata.substitutions is empty.
 */
TEST(JobLoader, ValidateJobSchema_EmptySubstitutions_ReturnsUserError) {
  // Prepare
  const char* metadata_yaml =
      "metadata:\n"
      "  substitutions: {}\n";
  const std::filesystem::path yaml = write_contract_job(
      "empty_substitutions.yaml", {.metadata_yaml = metadata_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_6));
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_6));
}

/*!
 * \brief Accepts a complete job whose \c metadata.substitutions has at least
 *        one key.
 */
TEST(JobLoader, ValidateJobSchema_OneSubstitution_Succeeds) {
  // Prepare
  const char* metadata_yaml =
      "metadata:\n"
      "  substitutions:\n"
      "    project: \"DEMO\"\n";
  const std::filesystem::path yaml = write_contract_job(
      "one_substitution.yaml", {.metadata_yaml = metadata_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  EXPECT_GE(job.value().metadata().substitutions().size(), 1u);
  EXPECT_EQ(expect_string_sub(job.value().metadata(), "project"), "DEMO");
}

/*!
 * \brief Rejects a substitution whose key is not a string.
 */
TEST(JobLoader, ValidateJobSchema_NonStringSubstitutionKey_ReturnsUserError) {
  // Prepare
  const char* metadata_yaml =
      "metadata:\n"
      "  substitutions:\n"
      "    123: DEMO\n";
  const std::filesystem::path yaml = write_contract_job(
      "non_string_sub_key.yaml", {.metadata_yaml = metadata_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_2));
  expect_user_message(schema, "'123' is integer 123");
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_2));
  expect_user_message(job.status(), "'123' is integer 123");
}

/*!
 * \brief Rejects a substitution whose key is a YAML boolean.
 */
TEST(JobLoader, ValidateJobSchema_BooleanSubstitutionKey_ReturnsUserError) {
  // Prepare
  const char* metadata_yaml =
      "metadata:\n"
      "  substitutions:\n"
      "    true: \"DEMO\"\n";
  const std::filesystem::path yaml = write_contract_job(
      "boolean_sub_key.yaml", {.metadata_yaml = metadata_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_2));
  expect_user_message(schema, "'true' is a boolean");
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_2));
  expect_user_message(job.status(), "'true' is a boolean");
}

/*!
 * \brief Accepts a substitution whose key is a string.
 */
TEST(JobLoader, ValidateJobSchema_StringSubstitutionKey_Succeeds) {
  // Prepare
  const char* metadata_yaml =
      "metadata:\n"
      "  substitutions:\n"
      "    project: \"DEMO\"\n";
  const std::filesystem::path yaml = write_contract_job(
      "string_sub_key.yaml", {.metadata_yaml = metadata_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  EXPECT_EQ(expect_string_sub(job.value().metadata(), "project"), "DEMO");
}

/*!
 * \brief Rejects a substitution whose value is neither a string nor a frame
 *        map.
 */
TEST(JobLoader, ValidateJobSchema_InvalidSubstitutionValue_ReturnsUserError) {
  // Prepare
  const char* metadata_yaml =
      "metadata:\n"
      "  substitutions:\n"
      "    project:\n"
      "      - not-a-string\n";
  const std::filesystem::path yaml = write_contract_job(
      "invalid_sub_value.yaml", {.metadata_yaml = metadata_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_3));
  expect_user_message(schema, "metadata.substitutions.project is a list");
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_3));
  expect_user_message(job.status(), "metadata.substitutions.project is a list");
}

/*!
 * \brief Accepts a substitution whose value is a quoted YAML string.
 */
TEST(JobLoader, ValidateJobSchema_StringSubstitutionValue_Succeeds) {
  // Prepare
  const char* metadata_yaml =
      "metadata:\n"
      "  substitutions:\n"
      "    project: \"DEMO\"\n";
  const std::filesystem::path yaml = write_contract_job(
      "string_sub_value.yaml", {.metadata_yaml = metadata_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  EXPECT_EQ(expect_string_sub(job.value().metadata(), "project"), "DEMO");
}

/*!
 * \brief Rejects a substitution whose value is an unquoted integer.
 */
TEST(JobLoader, ValidateJobSchema_IntegerSubstitutionValue_ReturnsUserError) {
  // Prepare
  const char* metadata_yaml =
      "metadata:\n"
      "  substitutions:\n"
      "    episode: 103\n";
  const std::filesystem::path yaml = write_contract_job(
      "integer_sub_value.yaml", {.metadata_yaml = metadata_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_3));
  expect_user_message(schema, "metadata.substitutions.episode is integer 103");
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_3));
  expect_user_message(job.status(),
                      "metadata.substitutions.episode is integer 103");
}

/*!
 * \brief Rejects a substitution whose value is a YAML boolean.
 */
TEST(JobLoader, ValidateJobSchema_BooleanSubstitutionValue_ReturnsUserError) {
  // Prepare
  const char* metadata_yaml =
      "metadata:\n"
      "  substitutions:\n"
      "    \"project\": true\n";
  const std::filesystem::path yaml = write_contract_job(
      "boolean_sub_value.yaml", {.metadata_yaml = metadata_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_3));
  expect_user_message(schema, "metadata.substitutions.project is boolean true");
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_3));
  expect_user_message(job.status(),
                      "metadata.substitutions.project is a boolean");
}

/*!
 * \brief Rejects a substitution whose value is an unquoted YAML scalar.
 */
TEST(JobLoader, ValidateJobSchema_UnquotedSubstitutionValue_ReturnsUserError) {
  // Prepare
  const char* metadata_yaml =
      "metadata:\n"
      "  substitutions:\n"
      "    \"project\": DEMO\n";
  const std::filesystem::path yaml = write_contract_job(
      "unquoted_sub_value.yaml", {.metadata_yaml = metadata_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_3));
  expect_user_message(schema,
                      "metadata.substitutions.project is unquoted "
                      "string \"DEMO\"");
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_3));
  expect_user_message(job.status(),
                      "metadata.substitutions.project is unquoted string "
                      "\"DEMO\"");
}

/*!
 * \brief Rejects a quoted numeric substitution name whose value is an unquoted
 *        integer.
 */
TEST(JobLoader,
     ValidateJobSchema_QuotedNumericKeyIntegerValue_ReturnsUserError) {
  // Prepare
  const char* metadata_yaml =
      "metadata:\n"
      "  substitutions:\n"
      "    \"project\": \"DEMO\"\n"
      "    \"1\": 1\n"
      "    \"note_frame\":\n"
      "      \"1001\": \"Start\"\n";
  const std::filesystem::path yaml =
      write_contract_job("quoted_numeric_key_integer_value.yaml",
                         {.metadata_yaml = metadata_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_3));
  expect_user_message(schema, "metadata.substitutions[\"1\"] is integer 1");
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_3));
  expect_user_message(job.status(),
                      "metadata.substitutions[\"1\"] is integer 1");
}

/*!
 * \brief Accepts a substitution whose value is a quoted numeric string.
 */
TEST(JobLoader, ValidateJobSchema_QuotedNumericSubstitutionValue_Succeeds) {
  // Prepare
  const char* metadata_yaml =
      "metadata:\n"
      "  substitutions:\n"
      "    episode: \"103\"\n";
  const std::filesystem::path yaml = write_contract_job(
      "quoted_numeric_sub_value.yaml", {.metadata_yaml = metadata_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  EXPECT_EQ(expect_string_sub(job.value().metadata(), "episode"), "103");
}

/*!
 * \brief Accepts a substitution whose value is a frame map.
 */
TEST(JobLoader, ValidateJobSchema_FrameMapSubstitutionValue_Succeeds) {
  // Prepare
  const char* metadata_yaml =
      "metadata:\n"
      "  substitutions:\n"
      "    note_frame:\n"
      "      \"1001\": \"Start\"\n"
      "      \"1005-1010\": \"Action\"\n";
  const std::filesystem::path yaml = write_contract_job(
      "frame_map_sub_value.yaml", {.metadata_yaml = metadata_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  const std::map<std::string, std::string> note_frame =
      expect_frame_map_sub(job.value().metadata(), "note_frame");
  ASSERT_EQ(note_frame.size(), 2u);
  EXPECT_EQ(note_frame.at("1001"), "Start");
  EXPECT_EQ(note_frame.at("1005-1010"), "Action");
}

/*!
 * \brief Accepts a frame map whose keys are YAML strings.
 */
TEST(JobLoader, ValidateJobSchema_StringFrameMapKey_Succeeds) {
  // Prepare
  const char* metadata_yaml =
      "metadata:\n"
      "  substitutions:\n"
      "    note_frame:\n"
      "      \"1001\": \"Start\"\n"
      "      \"1005-1010\": \"Action\"\n";
  const std::filesystem::path yaml = write_contract_job(
      "string_frame_map_key.yaml", {.metadata_yaml = metadata_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  const std::map<std::string, std::string> note_frame =
      expect_frame_map_sub(job.value().metadata(), "note_frame");
  ASSERT_EQ(note_frame.size(), 2u);
  EXPECT_EQ(note_frame.at("1001"), "Start");
}

/*!
 * \brief Rejects a frame map whose key is not a YAML string.
 */
TEST(JobLoader, ValidateJobSchema_NonStringFrameMapKey_ReturnsUserError) {
  // Prepare
  const char* metadata_yaml =
      "metadata:\n"
      "  substitutions:\n"
      "    note_frame:\n"
      "      1001: \"Start\"\n";
  const std::filesystem::path yaml = write_contract_job(
      "non_string_frame_map_key.yaml", {.metadata_yaml = metadata_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_45));
  expect_user_message(schema, "'1001' is integer 1001");
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_45));
  expect_user_message(job.status(), "'1001' is integer 1001");
}

/*!
 * \brief Accepts a frame map whose values are quoted YAML strings.
 */
TEST(JobLoader, ValidateJobSchema_StringFrameMapValue_Succeeds) {
  // Prepare
  const char* metadata_yaml =
      "metadata:\n"
      "  substitutions:\n"
      "    note_frame:\n"
      "      \"1001\": \"Start\"\n"
      "      \"1005-1010\": \"Action\"\n";
  const std::filesystem::path yaml = write_contract_job(
      "string_frame_map_value.yaml", {.metadata_yaml = metadata_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  const std::map<std::string, std::string> note_frame =
      expect_frame_map_sub(job.value().metadata(), "note_frame");
  ASSERT_EQ(note_frame.size(), 2u);
  EXPECT_EQ(note_frame.at("1001"), "Start");
  EXPECT_EQ(note_frame.at("1005-1010"), "Action");
}

/*!
 * \brief Rejects a frame map whose value is not a YAML string.
 */
TEST(JobLoader, ValidateJobSchema_NonStringFrameMapValue_ReturnsUserError) {
  // Prepare
  const char* metadata_yaml =
      "metadata:\n"
      "  substitutions:\n"
      "    note_frame:\n"
      "      \"1001\": 1\n";
  const std::filesystem::path yaml = write_contract_job(
      "non_string_frame_map_value.yaml", {.metadata_yaml = metadata_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_46));
  expect_user_message(
      schema, "metadata.substitutions.note_frame[\"1001\"] is integer 1");
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_46));
  expect_user_message(
      job.status(), "metadata.substitutions.note_frame[\"1001\"] is integer 1");
}

/*!
 * \brief Rejects a frame map whose value is an unquoted YAML scalar.
 */
TEST(JobLoader, ValidateJobSchema_UnquotedFrameMapValue_ReturnsUserError) {
  // Prepare
  const char* metadata_yaml =
      "metadata:\n"
      "  substitutions:\n"
      "    note_frame:\n"
      "      \"1001\": Start\n";
  const std::filesystem::path yaml = write_contract_job(
      "unquoted_frame_map_value.yaml", {.metadata_yaml = metadata_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_46));
  expect_user_message(
      schema,
      "metadata.substitutions.note_frame[\"1001\"] is unquoted string "
      "\"Start\"");
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_46));
  expect_user_message(
      job.status(),
      "metadata.substitutions.note_frame[\"1001\"] is unquoted string "
      "\"Start\"");
}

/*!
 * \brief Rejects a frame map whose value is a YAML boolean.
 */
TEST(JobLoader, ValidateJobSchema_BooleanFrameMapValue_ReturnsUserError) {
  // Prepare
  const char* metadata_yaml =
      "metadata:\n"
      "  substitutions:\n"
      "    note_frame:\n"
      "      \"1001\": true\n";
  const std::filesystem::path yaml = write_contract_job(
      "boolean_frame_map_value.yaml", {.metadata_yaml = metadata_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_46));
  expect_user_message(schema, "boolean true");
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_46));
  expect_user_message(job.status(), "a boolean");
}

/*!
 * \brief Rejects a frame map whose key is not a frame number.
 */
TEST(JobLoader, ValidateJobSchema_NonFrameFrameMapKey_ReturnsUserError) {
  // Prepare
  const char* metadata_yaml =
      "metadata:\n"
      "  substitutions:\n"
      "    note_frame:\n"
      "      \"hello\": \"Start\"\n";
  const std::filesystem::path yaml = write_contract_job(
      "non_frame_frame_map_key.yaml", {.metadata_yaml = metadata_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_47));
  expect_user_message(schema, "'hello' is not a frame or range");
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_47));
  expect_user_message(job.status(), "'hello' is not a frame or range");
}

/*!
 * \brief Rejects a frame map whose key is not a frame range.
 */
TEST(JobLoader, ValidateJobSchema_NonFrameRangeFrameMapKey_ReturnsUserError) {
  // Prepare
  const char* metadata_yaml =
      "metadata:\n"
      "  substitutions:\n"
      "    note_frame:\n"
      "      \"1005-abc\": \"Action\"\n";
  const std::filesystem::path yaml = write_contract_job(
      "non_frame_range_frame_map_key.yaml", {.metadata_yaml = metadata_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_47));
  expect_user_message(schema, "'1005-abc' is not a frame or range");
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_47));
  expect_user_message(job.status(), "'1005-abc' is not a frame or range");
}

/*!
 * \brief Accepts a frame map whose keys are a frame or a frame range.
 */
TEST(JobLoader, ValidateJobSchema_FrameOrRangeFrameMapKey_Succeeds) {
  // Prepare
  const char* metadata_yaml =
      "metadata:\n"
      "  substitutions:\n"
      "    note_frame:\n"
      "      \"1001\": \"Start\"\n"
      "      \"1005-1010\": \"Action\"\n";
  const std::filesystem::path yaml = write_contract_job(
      "frame_or_range_frame_map_key.yaml", {.metadata_yaml = metadata_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  const std::map<std::string, std::string> note_frame =
      expect_frame_map_sub(job.value().metadata(), "note_frame");
  ASSERT_EQ(note_frame.size(), 2u);
  EXPECT_EQ(note_frame.at("1001"), "Start");
  EXPECT_EQ(note_frame.at("1005-1010"), "Action");
}

/*!
 * \brief Rejects a complete job that omits \c plans.
 */
TEST(JobLoader, ValidateJobSchema_OmittedPlans_ReturnsUserError) {
  // Prepare
  const std::filesystem::path yaml =
      write_contract_job("no_plans.yaml", {.include_plans = false});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_12));
  ASSERT_FALSE(job.ok());
  EXPECT_EQ(job.status().code(), dailyboy::Status::Code::kUser);
}

/*!
 * \brief Accepts a complete job whose \c plans array contains one plan.
 */
TEST(JobLoader, ValidateJobSchema_OnePlan_Succeeds) {
  // Prepare
  const std::filesystem::path yaml = write_contract_job("one_plan.yaml");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  const std::vector<dailyboy::JobPlan>& plans = job.value().plans().plans();
  ASSERT_EQ(plans.size(), 1u);
  EXPECT_EQ(plans[0].id(), "plate");
}

/*!
 * \brief Rejects a complete job whose \c plans array is empty.
 */
TEST(JobLoader, ValidateJobSchema_EmptyPlans_ReturnsUserError) {
  // Prepare
  const char* plans_yaml = "plans: []\n";
  const std::filesystem::path yaml =
      write_contract_job("empty_plans.yaml", {.plans_yaml = plans_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_37));
  expect_user_message(schema, "(got 0)");
}

/*!
 * \brief Rejects a complete job whose \c plans array contains more than one
 *        plan.
 */
TEST(JobLoader, ValidateJobSchema_MultiplePlans_ReturnsUserError) {
  // Prepare
  const char* plans_yaml =
      "plans:\n"
      "  - id: plate\n"
      "    input_colorspace: ACES - ACEScg\n"
      "    sequence:\n"
      "      path: /tmp/plate.%04d.png\n"
      "      frame_start: 1001\n"
      "      frame_end: 1003\n"
      "  - id: grade\n"
      "    input_colorspace: ACES - ACEScg\n"
      "    sequence:\n"
      "      path: /tmp/grade.%04d.png\n"
      "      frame_start: 1001\n"
      "      frame_end: 1003\n";
  const std::filesystem::path yaml =
      write_contract_job("two_plans.yaml", {.plans_yaml = plans_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_37));
  expect_user_message(schema, "(got 2)");
}

/*!
 * \brief Accepts a complete job that includes \c color.
 */
TEST(JobLoader, ValidateJobSchema_PresentColor_Succeeds) {
  // Prepare
  const std::filesystem::path yaml = write_contract_job("with_color.yaml");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  EXPECT_EQ(job.value().colorimetry().ocio_config(),
            std::filesystem::path("/unused/config.ocio"));
}

/*!
 * \brief Rejects a complete job that omits \c color.
 */
TEST(JobLoader, ValidateJobSchema_OmittedColor_ReturnsUserError) {
  // Prepare
  const std::filesystem::path yaml =
      write_contract_job("no_color.yaml", {.include_color = false});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_13));
  ASSERT_FALSE(job.ok());
  EXPECT_EQ(job.status().code(), dailyboy::Status::Code::kUser);
}

/*!
 * \brief Rejects a color block that omits \c ocio_config.
 */
TEST(JobLoader, ValidateJobSchema_OmittedOcioConfig_ReturnsUserError) {
  // Prepare
  const char* color_yaml =
      "color:\n"
      "  working_colorspace: \"ACES - ACEScg\"\n";
  const std::filesystem::path yaml =
      write_contract_job("no_ocio_config.yaml", {.color_yaml = color_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_17));
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_17));
}

/*!
 * \brief Accepts a color block that includes \c ocio_config.
 */
TEST(JobLoader, ValidateJobSchema_PresentOcioConfig_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_contract_job("with_ocio_config.yaml");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  EXPECT_EQ(job.value().colorimetry().ocio_config(),
            std::filesystem::path("/unused/config.ocio"));
}

/*!
 * \brief Accepts a color block that omits \c working_colorspace.
 */
TEST(JobLoader, ValidateJobSchema_OmittedWorkingColorspace_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_contract_job("no_working_colorspace.yaml");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  EXPECT_TRUE(job.value().colorimetry().working_colorspace().empty());
}

/*!
 * \brief Accepts a color block that includes \c working_colorspace.
 */
TEST(JobLoader, ValidateJobSchema_PresentWorkingColorspace_Succeeds) {
  // Prepare
  const char* color_yaml =
      "color:\n"
      "  ocio_config: \"/unused/config.ocio\"\n"
      "  working_colorspace: \"ACES - ACEScg\"\n";
  const std::filesystem::path yaml = write_contract_job(
      "with_working_colorspace.yaml", {.color_yaml = color_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  EXPECT_EQ(job.value().colorimetry().working_colorspace(), "ACES - ACEScg");
}

/*!
 * \brief Accepts a color block that omits \c context.
 */
TEST(JobLoader, ValidateJobSchema_OmittedContext_Succeeds) {
  // Prepare
  const std::filesystem::path yaml = write_contract_job("no_context.yaml");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  EXPECT_TRUE(job.value().colorimetry().context().empty());
}

/*!
 * \brief Accepts a color block whose \c context has at least one pair.
 */
TEST(JobLoader, ValidateJobSchema_PresentContext_Succeeds) {
  // Prepare
  const char* color_yaml =
      "color:\n"
      "  ocio_config: \"/unused/config.ocio\"\n"
      "  context:\n"
      "    \"SHOT\": \"sh010\"\n";
  const std::filesystem::path yaml =
      write_contract_job("with_context.yaml", {.color_yaml = color_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  ASSERT_EQ(job.value().colorimetry().context().size(), 1u);
  EXPECT_EQ(job.value().colorimetry().context().at("SHOT"), "sh010");
}

/*!
 * \brief Accepts \c ocio_config as a quoted YAML string.
 */
TEST(JobLoader, ValidateJobSchema_QuotedOcioConfig_Succeeds) {
  // Prepare
  const std::filesystem::path yaml =
      write_contract_job("quoted_ocio_config.yaml");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  EXPECT_EQ(job.value().colorimetry().ocio_config(),
            std::filesystem::path("/unused/config.ocio"));
}

/*!
 * \brief Rejects \c ocio_config as an unquoted YAML scalar.
 */
TEST(JobLoader, ValidateJobSchema_UnquotedOcioConfig_ReturnsUserError) {
  // Prepare
  const char* color_yaml =
      "color:\n"
      "  ocio_config: /unused/config.ocio\n";
  const std::filesystem::path yaml = write_contract_job(
      "unquoted_ocio_config.yaml", {.color_yaml = color_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_48));
  expect_user_message(
      schema, "color.ocio_config is unquoted string \"/unused/config.ocio\"");
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_48));
  expect_user_message(
      job.status(),
      "color.ocio_config is unquoted string \"/unused/config.ocio\"");
}

/*!
 * \brief Rejects \c ocio_config as an empty quoted string.
 */
TEST(JobLoader, ValidateJobSchema_EmptyOcioConfig_ReturnsUserError) {
  // Prepare
  const char* color_yaml =
      "color:\n"
      "  ocio_config: \"\"\n";
  const std::filesystem::path yaml =
      write_contract_job("empty_ocio_config.yaml", {.color_yaml = color_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_48));
  expect_user_message(schema, "color.ocio_config is empty");
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_48));
  expect_user_message(job.status(), "color.ocio_config is empty");
}

/*!
 * \brief Accepts \c working_colorspace as a quoted YAML string.
 */
TEST(JobLoader, ValidateJobSchema_QuotedWorkingColorspace_Succeeds) {
  // Prepare
  const char* color_yaml =
      "color:\n"
      "  ocio_config: \"/unused/config.ocio\"\n"
      "  working_colorspace: \"ACES - ACEScg\"\n";
  const std::filesystem::path yaml = write_contract_job(
      "quoted_working_colorspace.yaml", {.color_yaml = color_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  EXPECT_EQ(job.value().colorimetry().working_colorspace(), "ACES - ACEScg");
}

/*!
 * \brief Rejects \c working_colorspace as an unquoted YAML scalar.
 */
TEST(JobLoader, ValidateJobSchema_UnquotedWorkingColorspace_ReturnsUserError) {
  // Prepare
  const char* color_yaml =
      "color:\n"
      "  ocio_config: \"/unused/config.ocio\"\n"
      "  working_colorspace: ACES - ACEScg\n";
  const std::filesystem::path yaml = write_contract_job(
      "unquoted_working_colorspace.yaml", {.color_yaml = color_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_49));
  expect_user_message(
      schema, "color.working_colorspace is unquoted string \"ACES - ACEScg\"");
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_49));
  expect_user_message(
      job.status(),
      "color.working_colorspace is unquoted string \"ACES - ACEScg\"");
}

/*!
 * \brief Rejects \c working_colorspace as an empty quoted string.
 */
TEST(JobLoader, ValidateJobSchema_EmptyWorkingColorspace_ReturnsUserError) {
  // Prepare
  const char* color_yaml =
      "color:\n"
      "  ocio_config: \"/unused/config.ocio\"\n"
      "  working_colorspace: \"\"\n";
  const std::filesystem::path yaml = write_contract_job(
      "empty_working_colorspace.yaml", {.color_yaml = color_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_49));
  expect_user_message(schema, "color.working_colorspace is empty");
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_49));
  expect_user_message(job.status(), "color.working_colorspace is empty");
}

/*!
 * \brief Rejects \c context as an empty map.
 */
TEST(JobLoader, ValidateJobSchema_EmptyContext_ReturnsUserError) {
  // Prepare
  const char* color_yaml =
      "color:\n"
      "  ocio_config: \"/unused/config.ocio\"\n"
      "  context: {}\n";
  const std::filesystem::path yaml =
      write_contract_job("empty_context.yaml", {.color_yaml = color_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_51));
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_51));
}

/*!
 * \brief Accepts a \c context key as a quoted YAML string.
 */
TEST(JobLoader, ValidateJobSchema_QuotedContextKey_Succeeds) {
  // Prepare
  const char* color_yaml =
      "color:\n"
      "  ocio_config: \"/unused/config.ocio\"\n"
      "  context:\n"
      "    \"SHOT\": \"sh010\"\n";
  const std::filesystem::path yaml =
      write_contract_job("quoted_context_key.yaml", {.color_yaml = color_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  EXPECT_EQ(job.value().colorimetry().context().at("SHOT"), "sh010");
}

/*!
 * \brief Rejects a \c context key as an unquoted YAML scalar.
 */
TEST(JobLoader, ValidateJobSchema_UnquotedContextKey_ReturnsUserError) {
  // Prepare
  const char* color_yaml =
      "color:\n"
      "  ocio_config: \"/unused/config.ocio\"\n"
      "  context:\n"
      "    SHOT: \"sh010\"\n";
  const std::filesystem::path yaml = write_contract_job(
      "unquoted_context_key.yaml", {.color_yaml = color_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_52));
  expect_user_message(schema, "'SHOT' is unquoted string \"SHOT\"");
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_52));
  expect_user_message(job.status(), "'SHOT' is unquoted string \"SHOT\"");
}

/*!
 * \brief Accepts a \c context value as a quoted YAML string.
 */
TEST(JobLoader, ValidateJobSchema_QuotedContextValue_Succeeds) {
  // Prepare
  const char* color_yaml =
      "color:\n"
      "  ocio_config: \"/unused/config.ocio\"\n"
      "  context:\n"
      "    \"SHOT\": \"sh010\"\n";
  const std::filesystem::path yaml = write_contract_job(
      "quoted_context_value.yaml", {.color_yaml = color_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  EXPECT_EQ(job.value().colorimetry().context().at("SHOT"), "sh010");
}

/*!
 * \brief Rejects a \c context value as an unquoted YAML scalar.
 */
TEST(JobLoader, ValidateJobSchema_UnquotedContextValue_ReturnsUserError) {
  // Prepare
  const char* color_yaml =
      "color:\n"
      "  ocio_config: \"/unused/config.ocio\"\n"
      "  context:\n"
      "    \"SHOT\": sh010\n";
  const std::filesystem::path yaml = write_contract_job(
      "unquoted_context_value.yaml", {.color_yaml = color_yaml});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_53));
  expect_user_message(schema,
                      "color.context.SHOT is unquoted string \"sh010\"");
  expect_user_message(job.status(), std::string(dailyboy::USER_ERROR_JOB_53));
  expect_user_message(job.status(),
                      "color.context.SHOT is unquoted string \"sh010\"");
}

/*!
 * \brief Accepts a complete job that includes \c layout.
 */
TEST(JobLoader, ValidateJobSchema_PresentLayout_Succeeds) {
  // Prepare
  const std::filesystem::path yaml = write_contract_job("with_layout.yaml");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  EXPECT_EQ(job.value().layout().canvas().width(), 8);
  EXPECT_EQ(job.value().layout().canvas().height(), 8);
}

/*!
 * \brief Rejects a complete job that omits \c layout.
 */
TEST(JobLoader, ValidateJobSchema_OmittedLayout_ReturnsUserError) {
  // Prepare
  const std::filesystem::path yaml =
      write_contract_job("no_layout.yaml", {.include_layout = false});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_14));
  ASSERT_FALSE(job.ok());
  EXPECT_EQ(job.status().code(), dailyboy::Status::Code::kUser);
}

/*!
 * \brief Accepts a complete job that includes \c output.
 */
TEST(JobLoader, ValidateJobSchema_PresentOutput_Succeeds) {
  // Prepare
  const std::filesystem::path yaml = write_contract_job("with_output.yaml");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  const std::vector<dailyboy::JobOutputVideo>& videos =
      job.value().output().videos().videos();
  ASSERT_EQ(videos.size(), 1u);
  EXPECT_EQ(videos[0].id(), "preview");
}

/*!
 * \brief Rejects a complete job that omits \c output.
 */
TEST(JobLoader, ValidateJobSchema_OmittedOutput_ReturnsUserError) {
  // Prepare
  const std::filesystem::path yaml =
      write_contract_job("no_output.yaml", {.include_output = false});

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  expect_user_message(schema, std::string(dailyboy::USER_ERROR_JOB_15));
  ASSERT_FALSE(job.ok());
  EXPECT_EQ(job.status().code(), dailyboy::Status::Code::kUser);
}

/*!
 * \brief Loads the all-fields fixture and stores every documented value.
 */
TEST(JobLoader, LoadJob_ValidYaml_LoadsAllFields) {
  // Prepare
  const std::filesystem::path yaml = job_loader_fixture();

  // Test
  dailyboy::StatusOr<dailyboy::Job> job_or = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(job_or.ok()) << job_or.status().message();
  const dailyboy::Job& job = job_or.value();
  // ############################################################
  // dailyboy_version: 1
  // ############################################################

  EXPECT_EQ(job.dailyboy_version(), 1);
  // ############################################################
  // metadata:
  // substitutions:
  //   project: "JOB_LOADER_TEST"
  //   episode: "101"
  //   shot: "sh010"
  //   version: "v004"
  //   artist: "tester"
  //   facility: "dailyboy_unit"
  //   note_prod: "Global production note"
  //   dailies_root: "/tmp/dailyboy/job_loader_test"
  //   working_colorspace: "ACES - ACEScg"
  //   review_mov_path: "{dailies_root}/deliver/review.mov"
  //   note_frame:
  //     1001: "Frame 1001"
  //     1005-1010: "Frames 100-1010"
  //     1048: "Last frame"
  // ############################################################

  const dailyboy::JobMetadata& metadata = job.metadata();
  EXPECT_EQ(metadata.substitutions().size(), 11u);
  // Should match definition

  EXPECT_EQ(expect_string_sub(metadata, "project"), "JOB_LOADER_TEST");
  // project: "JOB_LOADER_TEST"
  EXPECT_EQ(expect_string_sub(metadata, "episode"), "101");
  // episode: "101"
  EXPECT_EQ(expect_string_sub(metadata, "shot"), "sh010");
  // shot: "sh010"
  EXPECT_EQ(expect_string_sub(metadata, "version"), "v004");
  // version: "v004"
  EXPECT_EQ(expect_string_sub(metadata, "artist"), "tester");
  // artist: "tester"
  EXPECT_EQ(expect_string_sub(metadata, "facility"), "dailyboy_unit");
  // facility: "dailyboy_unit"
  EXPECT_EQ(expect_string_sub(metadata, "note_prod"), "Global production note");
  // note_prod: global note
  EXPECT_EQ(expect_string_sub(metadata, "dailies_root"),
            "/tmp/dailyboy/job_loader_test");
  // dailies_root:
  // "/tmp/dailyboy/job_loader_test"
  EXPECT_EQ(expect_string_sub(metadata, "working_colorspace"), "ACES - ACEScg");
  // working_colorspace: "ACES - ACEScg"
  EXPECT_EQ(expect_string_sub(metadata, "review_mov_path"),
            "{dailies_root}/deliver/review.mov");
  // review_mov_path: templated path

  const std::map<std::string, std::string> note_frame =
      expect_frame_map_sub(metadata, "note_frame");
  ASSERT_EQ(note_frame.size(), 3u);
  EXPECT_EQ(note_frame.at("1001"), "Frame 1001");
  // "1001": "Frame 1001"
  EXPECT_EQ(note_frame.at("1005-1010"), "Frames 100-1010");
  // "1005-1010": "Frames 100-1010"
  EXPECT_EQ(note_frame.at("1048"), "Last frame");
  // "1048": "Last frame"

  // ############################################################
  // color:
  //   ocio_config: "${OCIO_CONFIGS}/aces_1.2/config.ocio"
  //   working_colorspace: "{working_colorspace}"
  //   context:
  //     "SHOT": "sh010"
  //     "SEQUENCE": "seq010"
  //     "FACILITY": "dailyboy"
  // ############################################################

  const dailyboy::JobColorimetry& color = job.colorimetry();
  EXPECT_EQ(color.ocio_config(),
            std::filesystem::path("${OCIO_CONFIGS}/aces_1.2/config.ocio"));
  // ocio_config
  EXPECT_EQ(color.working_colorspace(), "{working_colorspace}");
  // working_colorspace (variable)
  ASSERT_EQ(color.context().size(), 3u);
  // context has 3 variables as defined
  EXPECT_EQ(color.context().at("SHOT"), "sh010");
  EXPECT_EQ(color.context().at("SEQUENCE"), "seq010");
  EXPECT_EQ(color.context().at("FACILITY"), "dailyboy");
  // ############################################################
  // layout:
  // canvas:
  //   width: 1920
  //   height: 1080
  // pixel_aspect: 1.0
  // image:
  //   fit: cover
  //   filter: bilinear
  // background:
  //   r: 0.12
  //   g: 0.14
  //   b: 0.18
  // burn_ins:
  //   - template: "{shot}  {frame}  {plan_id}  {source_file}"
  //     position:
  //       mode: layout
  //       anchor: bottom_left
  //     font:
  //       path: /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf
  //       size_px: 22
  //     box:
  //       mode: fill
  //       color:
  //         r: 0
  //         g: 0
  //         b: 0
  //       opacity: 0.5
  //       margin: 8
  //   - template: "{project} — {version}"
  //     position:
  //       mode: layout
  //       anchor: bottom_right
  //     font:
  //       path: /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf
  //       size_px: 18
  //     box:
  //       mode: outline
  //       color:
  //         r: 1
  //         g: 1
  //         b: 1
  //       opacity: 0.85
  //       margin:
  //         top: 4
  //         right: 12
  //         bottom: 4
  //         left: 8
  //   - template: "{note_frame}"
  //     position:
  //       mode: layout
  //       anchor: center_center
  //     font:
  //       path: /usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf
  //       size_px: 24
  //     box:
  //       mode: fill
  //       color:
  //         r: 0.1
  //         g: 0.1
  //         b: 0.2
  //       opacity: 0.4
  //       margin: 6
  //   - template: "PIXEL {frame}"
  //     position:
  //       mode: pixel
  //       x: 64
  //       y: 48
  //     font:
  //       path: /usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf
  //       size_px: 16
  //     box:
  //       mode: outline
  //       color:
  //         r: 0.9
  //         g: 0.9
  //         b: 0.2
  //       opacity: 0.7
  //       margin: 4
  //   - template: "PCT {frame_start}-{frame_end}"
  //     position:
  //       mode: percent
  //       x: 5
  //       y: 92
  //     font:
  //       path: /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf
  //       size_px: 20
  //     box:
  //       mode: fill
  //       color:
  //         r: 0
  //         g: 0
  //         b: 0
  //       opacity: 0.35
  //       margin:
  //         top: 2
  //         right: 2
  //         bottom: 2
  //         left: 2
  //   - template: "TOP {shot}"
  //     position:
  //       mode: layout
  //       anchor: top_left
  //     font:
  //       path: /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf
  //       size_px: 14
  //     box:
  //       mode: fill
  //       color:
  //         r: 0
  //         g: 0
  //         b: 0
  //       opacity: 0.25
  //       margin: 0
  //   - template: "TOP-R {episode}"
  //     position:
  //       mode: layout
  //       anchor: top_right
  //     font:
  //       path: /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf
  //       size_px: 14
  //     box:
  //       mode: outline
  //       color:
  //         r: 1
  //         g: 0.5
  //         b: 0
  //       opacity: 0.6
  //       margin: 10
  // slate:
  //   duration_frames: 16
  //   lines:
  //     - text: "PROJECT — {project}"
  //       position:
  //         mode: layout
  //         anchor: top_left
  //       font:
  //         path: /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf
  //         size_px: 36
  //     - text: "SHOT — {shot}"
  //       position:
  //         mode: layout
  //         anchor: top_right
  //       font:
  //         path: /usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf
  //         size_px: 32
  //     - text: "CENTER — {artist}"
  //       position:
  //         mode: layout
  //         anchor: center_center
  //       font:
  //         path: /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf
  //         size_px: 28
  //     - text: "NOTE — {note_prod}"
  //       position:
  //         mode: layout
  //         anchor: bottom_left
  //       font:
  //         path: /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf
  //         size_px: 22
  //     - text: "FRAME NOTE — {note_frame}"
  //       position:
  //         mode: layout
  //         anchor: bottom_right
  //       font:
  //         path: /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf
  //         size_px: 20
  //     - text: "SLATE PIXEL"
  //       position:
  //         mode: pixel
  //         x: 120
  //         y: 900
  //       font:
  //         path: /usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf
  //         size_px: 18
  //     - text: "SLATE %"
  //       position:
  //         mode: percent
  //         x: 50
  //         y: 50
  //       font:
  //         path: /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf
  //         size_px: 18
  // ############################################################

  const dailyboy::JobLayout& layout = job.layout();
  //   canvas:
  //     width: 1920
  //     height: 1080
  EXPECT_EQ(layout.canvas().width(), 1920);
  EXPECT_EQ(layout.canvas().height(), 1080);
  //   pixel_aspect: 1.0
  expect_near(layout.pixel_aspect().aspect(), 1.0);
  // image:
  //   fit: cover
  //   filter: bilinear
  //   min_margin_px:
  //     top: 40
  //     right: 24
  //     bottom: 64
  //     left: 24
  EXPECT_EQ(layout.image().fit(),
            dailyboy::JobLayoutImage::JobLayoutImageFitValue::Cover);
  EXPECT_EQ(layout.image().filter(),
            dailyboy::JobLayoutImage::JobLayoutImageFilterValue::Bilinear);
  expect_margin(layout.image().min_margin_px(), 40, 24, 64, 24);
  // background:
  //   r: 0.12
  //   g: 0.14
  //   b: 0.18
  expect_rgb(layout.background(), 0.12, 0.14, 0.18);
  // BURN-IN block: user defines 7 burn-in texts, each with specific settings
  const std::vector<dailyboy::JobLayoutBurnIn>& burn_ins =
      layout.burn_ins().burn_ins();
  ASSERT_EQ(burn_ins.size(), 7u);
  //   - template: "{shot}  {frame}  {plan_id}  {source_file}"
  //     position:
  //       mode: layout
  //       anchor: bottom_left
  //     font:
  //       path: /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf
  //       size_px: 22
  //     box:
  //       mode: fill
  //       color:
  //         r: 0
  //         g: 0
  //         b: 0
  //       opacity: 0.5
  //       margin: 8
  EXPECT_EQ(burn_ins[0].template_text(),
            "{shot}  {frame}  {plan_id}  {source_file}");
  expect_layout_anchor(burn_ins[0].position(),
                       dailyboy::TextPositionModeLayout::Anchor::BottomLeft);
  // defined
  // BottomLeft
  expect_font(burn_ins[0].font(),
              "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 22);
  // user font choice
  expect_rgb(burn_ins[0].font().color(), 1.0, 1.0, 1.0);
  ASSERT_TRUE(burn_ins[0].box().has_value());
  expect_burn_in_box_fill(*burn_ins[0].box(), 0, 0, 0, 0.5, 8);
  // Black box, 0.5 opacity, 8px margin

  //   - template: "{project} — {version}"
  //     position:
  //       mode: layout
  //       anchor: bottom_right
  //     font:
  //       path: /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf
  //       size_px: 18
  //     box:
  //       mode: outline
  //       color:
  //         r: 1
  //         g: 1
  //         b: 1
  //       opacity: 0.85
  //       margin:
  //         top: 4
  //         right: 12
  //         bottom: 4
  //         left: 8
  EXPECT_EQ(burn_ins[1].template_text(), "{project} — {version}");
  expect_layout_anchor(burn_ins[1].position(),
                       dailyboy::TextPositionModeLayout::Anchor::BottomRight);
  expect_font(burn_ins[1].font(),
              "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 18);
  ASSERT_TRUE(burn_ins[1].box().has_value());
  expect_burn_in_box_outline(*burn_ins[1].box(), 1, 1, 1, 0.85, 4, 12, 4, 8);
  // custom outline color and margins

  //   - template: "{note_frame}"
  //     position:
  //       mode: layout
  //       anchor: center_center
  //     font:
  //       path: /usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf
  //       size_px: 24
  //     box:
  //       mode: fill
  //       color:
  //         r: 0.1
  //         g: 0.1
  //         b: 0.2
  //       opacity: 0.4
  //       margin: 6
  EXPECT_EQ(burn_ins[2].template_text(), "{note_frame}");
  expect_layout_anchor(burn_ins[2].position(),
                       dailyboy::TextPositionModeLayout::Anchor::CenterCenter);
  expect_font(burn_ins[2].font(),
              "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 24);
  ASSERT_TRUE(burn_ins[2].box().has_value());
  expect_burn_in_box_fill(*burn_ins[2].box(), 0.1, 0.1, 0.2, 0.4, 6);
  //   - template: "PIXEL {frame}"
  //     position:
  //       mode: pixel
  //       x: 64
  //       y: 48
  //     font:
  //       path: /usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf
  //       size_px: 16
  //     box:
  //       mode: outline
  //       color:
  //         r: 0.9
  //         g: 0.9
  //         b: 0.2
  //       opacity: 0.7
  //       margin: 4
  EXPECT_EQ(burn_ins[3].template_text(), "PIXEL {frame}");
  expect_pixel_position(burn_ins[3].position(), 64, 48);
  expect_font(burn_ins[3].font(),
              "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", 16);
  ASSERT_TRUE(burn_ins[3].box().has_value());
  expect_burn_in_box_outline(*burn_ins[3].box(), 0.9, 0.9, 0.2, 0.7, 4, 4, 4,
                             4);
  //   - template: "PCT {frame_start}-{frame_end}"
  //     position:
  //       mode: percent
  //       x: 5
  //       y: 92
  //     font:
  //       path: /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf
  //       size_px: 20
  //     box:
  //       mode: fill
  //       color:
  //         r: 0
  //         g: 0
  //         b: 0
  //       opacity: 0.35
  //       margin:
  //         top: 2
  //         right: 2
  //         bottom: 2
  //         left: 2
  EXPECT_EQ(burn_ins[4].template_text(), "PCT {frame_start}-{frame_end}");
  expect_percent_position(burn_ins[4].position(), 5, 92);
  expect_font(burn_ins[4].font(),
              "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 20);
  ASSERT_TRUE(burn_ins[4].box().has_value());
  expect_burn_in_box_fill(*burn_ins[4].box(), 0, 0, 0, 0.35, 2);
  //   - template: "TOP {shot}"
  //     position:
  //       mode: layout
  //       anchor: top_left
  //     font:
  //       path: /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf
  //       size_px: 14
  //     box:
  //       mode: fill
  //       color:
  //         r: 0
  //         g: 0
  //         b: 0
  //       opacity: 0.25
  //       margin: 0
  EXPECT_EQ(burn_ins[5].template_text(), "TOP {shot}");
  expect_layout_anchor(burn_ins[5].position(),
                       dailyboy::TextPositionModeLayout::Anchor::TopLeft);
  expect_font(burn_ins[5].font(),
              "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 14);
  ASSERT_TRUE(burn_ins[5].box().has_value());
  expect_burn_in_box_fill(*burn_ins[5].box(), 0, 0, 0, 0.25, 0);
  //   - template: "TOP-R {episode}"
  //     position:
  //       mode: layout
  //       anchor: top_right
  //     font:
  //       path: /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf
  //       size_px: 14
  //     box:
  //       mode: outline
  //       color:
  //         r: 1
  //         g: 0.5
  //         b: 0
  //       opacity: 0.6
  //       margin: 10
  EXPECT_EQ(burn_ins[6].template_text(), "TOP-R {episode}");
  expect_layout_anchor(burn_ins[6].position(),
                       dailyboy::TextPositionModeLayout::Anchor::TopRight);
  expect_font(burn_ins[6].font(),
              "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 14);
  ASSERT_TRUE(burn_ins[6].box().has_value());
  expect_burn_in_box_outline(*burn_ins[6].box(), 1, 0.5, 0, 0.6, 10, 10, 10,
                             10);
  // SLATE block: duration and 7 lines
  const dailyboy::JobLayoutSlate& slate = layout.slate();
  EXPECT_EQ(slate.duration_frames(), 16);
  // duration_frames: 16
  const std::vector<dailyboy::JobLayoutSlateLine>& lines = slate.lines();
  ASSERT_EQ(lines.size(), 7u);
  // 7 lines as defined

  //     - text: "PROJECT — {project}"
  //       position:
  //         mode: layout
  //         anchor: top_left
  //       font:
  //         path: /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf
  //         size_px: 36
  EXPECT_EQ(lines[0].text(), "PROJECT — {project}");
  expect_layout_anchor(lines[0].position(),
                       dailyboy::TextPositionModeLayout::Anchor::TopLeft);
  expect_font(lines[0].font(),
              "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 36);
  //     - text: "SHOT — {shot}"
  //       position:
  //         mode: layout
  //         anchor: top_right
  //       font:
  //         path: /usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf
  //         size_px: 32
  EXPECT_EQ(lines[1].text(), "SHOT — {shot}");
  expect_layout_anchor(lines[1].position(),
                       dailyboy::TextPositionModeLayout::Anchor::TopRight);
  expect_font(lines[1].font(),
              "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 32);
  //     - text: "CENTER — {artist}"
  //       position:
  //         mode: layout
  //         anchor: center_center
  //       font:
  //         path: /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf
  //         size_px: 28
  EXPECT_EQ(lines[2].text(), "CENTER — {artist}");
  expect_layout_anchor(lines[2].position(),
                       dailyboy::TextPositionModeLayout::Anchor::CenterCenter);
  expect_font(lines[2].font(),
              "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 28);
  //     - text: "NOTE — {note_prod}"
  //       position:
  //         mode: layout
  //         anchor: bottom_left
  //       font:
  //         path: /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf
  //         size_px: 22
  EXPECT_EQ(lines[3].text(), "NOTE — {note_prod}");
  expect_layout_anchor(lines[3].position(),
                       dailyboy::TextPositionModeLayout::Anchor::BottomLeft);
  expect_font(lines[3].font(),
              "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 22);
  //     - text: "FRAME NOTE — {note_frame}"
  //       position:
  //         mode: layout
  //         anchor: bottom_right
  //       font:
  //         path: /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf
  //         size_px: 20
  EXPECT_EQ(lines[4].text(), "FRAME NOTE — {note_frame}");
  expect_layout_anchor(lines[4].position(),
                       dailyboy::TextPositionModeLayout::Anchor::BottomRight);
  expect_font(lines[4].font(),
              "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 20);
  //     - text: "SLATE PIXEL"
  //       position:
  //         mode: pixel
  //         x: 120
  //         y: 900
  //       font:
  //         path: /usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf
  //         size_px: 18
  EXPECT_EQ(lines[5].text(), "SLATE PIXEL");
  expect_pixel_position(lines[5].position(), 120, 900);
  expect_font(lines[5].font(),
              "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", 18);
  //     - text: "SLATE %"
  //       position:
  //         mode: percent
  //         x: 50
  //         y: 50
  //       font:
  //         path: /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf
  //         size_px: 18
  EXPECT_EQ(lines[6].text(), "SLATE %");
  expect_percent_position(lines[6].position(), 50, 50);
  expect_font(lines[6].font(),
              "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 18);
  // ############################################################
  // output:
  //  videos:
  //   - id: review_mov
  //     enabled: true
  //     colorspace: Output - sRGB
  //     path: "{review_mov_path}"
  //     codec: h264
  //     codec_options:
  //       preset: slow
  //       crf: 18
  //       gop: 24
  //       pix_fmt: yuv422p
  //  image_sequences:
  //   - id: archive_acescg
  //     enabled: true
  //     colorspace: ACES - ACEScg
  //     path_pattern: "{dailies_root}/archive.%04d.png"
  //   - id: share_srgb
  //     enabled: false
  //     colorspace: Output - sRGB
  //     path_pattern: "{dailies_root}/share.%04d.png"
  // ############################################################

  const std::vector<dailyboy::JobOutputVideo>& videos =
      job.output().videos().videos();
  ASSERT_EQ(videos.size(), 1u);
  const dailyboy::JobOutputVideo& video = videos[0];
  EXPECT_EQ(video.id(), "review_mov");
  // id set by user
  EXPECT_TRUE(video.enabled());
  // enabled: true
  EXPECT_EQ(video.display_view().display(), "passthrough");
  EXPECT_EQ(video.display_view().view(), "passthrough");
  // colorspace
  EXPECT_EQ(video.path(), "{review_mov_path}");
  // path with template variable
  EXPECT_EQ(video.fps(), 24);
  EXPECT_EQ(video.codec(),
            dailyboy::JobOutputVideo::JobOutputVideoCodecValue::H264);
  const dailyboy::JobOutputVideoH264& options =
      std::get<dailyboy::JobOutputVideoH264>(video.codec_options());
  EXPECT_EQ(options.preset(),
            dailyboy::JobOutputVideoH264::JobOutputVideoH264PresetValue::Slow);
  EXPECT_EQ(options.crf(), 18);
  EXPECT_EQ(options.gop(), 24);
  EXPECT_EQ(options.bitrate_kbps(), 0);
  EXPECT_EQ(
      options.pix_fmt(),
      dailyboy::JobOutputVideoH264::JobOutputVideoH264PixFmtValue::Yuv422p);
  EXPECT_EQ(
      options.tune(),
      dailyboy::JobOutputVideoH264::JobOutputVideoH264TuneValue::Unspecified);
  EXPECT_EQ(options.profile(), dailyboy::JobOutputVideoH264::
                                   JobOutputVideoH264ProfileValue::Unspecified);
  EXPECT_TRUE(options.level().empty());
  EXPECT_TRUE(options.faststart());
  EXPECT_EQ(video.signal().range(),
            dailyboy::JobOutputVideoSignal::RangeValue::Tv);
  // 2 image sequences as defined by user
  const std::vector<dailyboy::JobOutputImageSequence>& image_sequences =
      job.output().image_sequences().image_sequences();
  ASSERT_EQ(image_sequences.size(), 2u);
  // First image sequence: archive_acescg
  const dailyboy::JobOutputImageSequence& archive = image_sequences[0];
  EXPECT_EQ(archive.id(), "archive_acescg");
  EXPECT_TRUE(archive.enabled());
  EXPECT_EQ(archive.display_view().display(), "passthrough");
  EXPECT_EQ(archive.display_view().view(), "passthrough");
  EXPECT_EQ(archive.path_pattern(), "{dailies_root}/archive.%04d.png");
  // Second image sequence: share_srgb (disabled)
  const dailyboy::JobOutputImageSequence& share = image_sequences[1];
  EXPECT_EQ(share.id(), "share_srgb");
  EXPECT_FALSE(share.enabled());
  EXPECT_EQ(share.display_view().display(), "passthrough");
  EXPECT_EQ(share.display_view().view(), "passthrough");
  EXPECT_EQ(share.path_pattern(), "{dailies_root}/share.%04d.png");
  // ############################################################
  // plans:
  // - id: sh010_plate
  //   input_colorspace: ACES - ACEScg
  //   sequence:
  //     path: "{dailies_root}/plate.%04d.png"
  //     frame_start: 1001
  //     frame_end: 1048
  // ############################################################

  const std::vector<dailyboy::JobPlan>& plans = job.plans().plans();
  ASSERT_EQ(plans.size(), 1u);
  const dailyboy::JobPlan& plan = plans[0];
  EXPECT_EQ(plan.id(), "sh010_plate");
  EXPECT_EQ(plan.input_colorspace(), "ACES - ACEScg");
  EXPECT_EQ(plan.sequence().path(), "{dailies_root}/plate.%04d.png");
  EXPECT_EQ(plan.sequence().frame_start(), 1001);
  EXPECT_EQ(plan.sequence().frame_end(), 1048);
}

/*!
 * \brief Loads a job with no video codec and returns a user error.
 */
TEST(JobLoader, LoadJob_MissingCodec_ReturnsUserError) {
  // Prepare
  const std::filesystem::path yaml =
      std::filesystem::path(DAILYBOY_TEST_DATA_DIR) /
      "test__job_loader__load_job__omitted_codec.yaml";

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  EXPECT_FALSE(schema.ok()) << schema.message();
  ASSERT_FALSE(job.ok());
  EXPECT_EQ(job.status().code(), dailyboy::Status::Code::kUser);
  EXPECT_TRUE(job.status().message().find("codec") != std::string::npos)
      << job.status().message();
}

/*!
 * \brief Loads H.264 with omitted codec_options and applies engine defaults.
 */
TEST(JobLoader, LoadJob_OmittedCodecOptions_UsesH264Defaults) {
  // Prepare
  const std::filesystem::path yaml =
      std::filesystem::path(DAILYBOY_TEST_DATA_DIR) /
      "test__job_loader__load_job__omitted_codec_options.yaml";

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  const dailyboy::JobOutputVideo& video =
      job.value().output().videos().videos().front();
  EXPECT_EQ(video.fps(), 24);
  EXPECT_EQ(video.codec(),
            dailyboy::JobOutputVideo::JobOutputVideoCodecValue::H264);
  const dailyboy::JobOutputVideoH264& options =
      std::get<dailyboy::JobOutputVideoH264>(video.codec_options());
  EXPECT_EQ(
      options.preset(),
      dailyboy::JobOutputVideoH264::JobOutputVideoH264PresetValue::Medium);
  EXPECT_EQ(options.crf(), dailyboy::JobOutputVideoH264::kDefaultCrf);
  EXPECT_EQ(options.gop(), 0);
  EXPECT_EQ(options.bitrate_kbps(), 0);
  EXPECT_EQ(
      options.pix_fmt(),
      dailyboy::JobOutputVideoH264::JobOutputVideoH264PixFmtValue::Yuv420p);
  EXPECT_TRUE(options.faststart());
  EXPECT_EQ(video.signal().range(),
            dailyboy::JobOutputVideoSignal::RangeValue::Tv);
}

/*!
 * \brief Loads an unquoted YAML number level and canonicalizes it to \c 4.1.
 */
TEST(JobLoader, LoadJob_UnquotedLevel_CanonicalizesTo41) {
  // Prepare
  const std::filesystem::path yaml =
      std::filesystem::path(DAILYBOY_TEST_DATA_DIR) /
      "test__job_loader__load_job__level_unquoted.yaml";

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  EXPECT_EQ(std::get<dailyboy::JobOutputVideoH264>(
                job.value().output().videos().videos().front().codec_options())
                .level(),
            "4.1");
}

/*!
 * \brief Loads H.264 with both crf and bitrate set and returns a user error.
 */
TEST(JobLoader, LoadJob_CrfAndBitrate_ReturnsUserError) {
  // Prepare
  const std::filesystem::path yaml =
      std::filesystem::path(DAILYBOY_TEST_DATA_DIR) /
      "test__job_loader__load_job__crf_and_bitrate.yaml";

  // Test
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_FALSE(job.ok());
  EXPECT_EQ(job.status().code(), dailyboy::Status::Code::kUser);
  EXPECT_TRUE(job.status().message().find("mutually exclusive") !=
              std::string::npos)
      << job.status().message();
}

/*!
 * \brief Loads high422 with yuv420p and returns a user error.
 */
TEST(JobLoader, LoadJob_ProfilePixFmtMismatch_ReturnsUserError) {
  // Prepare
  const std::filesystem::path yaml =
      std::filesystem::path(DAILYBOY_TEST_DATA_DIR) /
      "test__job_loader__load_job__profile_pix_fmt_mismatch.yaml";

  // Test
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_FALSE(job.ok());
  EXPECT_EQ(job.status().code(), dailyboy::Status::Code::kUser);
  EXPECT_TRUE(job.status().message().find("high422") != std::string::npos)
      << job.status().message();
}

/*!
 * \brief Loads MJPEG with omitted codec_options and applies engine defaults.
 */
TEST(JobLoader, LoadJob_OmittedMjpegOptions_UsesMjpegDefaults) {
  // Prepare
  const std::filesystem::path yaml =
      std::filesystem::path(DAILYBOY_TEST_DATA_DIR) /
      "test__job_loader__load_job__omitted_mjpeg_options.yaml";

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  const dailyboy::JobOutputVideo& video =
      job.value().output().videos().videos().front();
  EXPECT_EQ(video.codec(),
            dailyboy::JobOutputVideo::JobOutputVideoCodecValue::Mjpeg);
  const dailyboy::JobOutputVideoMjpeg& options =
      std::get<dailyboy::JobOutputVideoMjpeg>(video.codec_options());
  EXPECT_EQ(options.qscale(), dailyboy::JobOutputVideoMjpeg::kDefaultQscale);
  EXPECT_EQ(
      options.pix_fmt(),
      dailyboy::JobOutputVideoMjpeg::JobOutputVideoMjpegPixFmtValue::Yuv422p);
  EXPECT_EQ(
      options.huffman(),
      dailyboy::JobOutputVideoMjpeg::JobOutputVideoMjpegHuffmanValue::Optimal);
  EXPECT_TRUE(options.faststart());
  EXPECT_EQ(video.signal().range(),
            dailyboy::JobOutputVideoSignal::RangeValue::Tv);
}

/*!
 * \brief Loads explicit MJPEG codec_options and stores qscale, pix_fmt, and
 *        huffman.
 */
TEST(JobLoader, LoadJob_MjpegOptions_ParsesAllFields) {
  // Prepare
  const std::filesystem::path yaml =
      std::filesystem::path(DAILYBOY_TEST_DATA_DIR) /
      "test__job_loader__load_job__mjpeg_options.yaml";

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  const dailyboy::JobOutputVideoMjpeg& options =
      std::get<dailyboy::JobOutputVideoMjpeg>(
          job.value().output().videos().videos().front().codec_options());
  EXPECT_EQ(options.qscale(), 5);
  EXPECT_EQ(
      options.pix_fmt(),
      dailyboy::JobOutputVideoMjpeg::JobOutputVideoMjpegPixFmtValue::Yuv420p);
  EXPECT_EQ(
      options.huffman(),
      dailyboy::JobOutputVideoMjpeg::JobOutputVideoMjpegHuffmanValue::Default);
}

/*!
 * \brief Validates MJPEG codec_options that use H.264 keys and fails schema.
 */
TEST(JobLoader, ValidateJobSchema_MjpegWithH264Keys_FailsValidation) {
  // Prepare
  const std::filesystem::path yaml =
      std::filesystem::path(DAILYBOY_TEST_DATA_DIR) /
      "test__job_loader__load_job__mjpeg_with_h264_keys.yaml";

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);

  // Assert
  EXPECT_FALSE(schema.ok()) << schema.message();
}

/*!
 * \brief Loads DNxHD with omitted codec_options and applies DNxHR HQ defaults.
 */
TEST(JobLoader, LoadJob_OmittedDnxhdOptions_UsesDnxhrHqDefaults) {
  // Prepare
  const std::filesystem::path yaml = write_dnxhd_job("omit.yaml", "");

  // Test
  const dailyboy::JobOutputVideoDnxhd options = load_dnxhd_options(yaml);

  // Assert
  EXPECT_EQ(
      options.profile(),
      dailyboy::JobOutputVideoDnxhd::JobOutputVideoDnxhdProfileValue::DnxhrHq);
  EXPECT_EQ(
      options.pix_fmt(),
      dailyboy::JobOutputVideoDnxhd::JobOutputVideoDnxhdPixFmtValue::Yuv422p);
  EXPECT_EQ(options.bitrate_kbps(), 0);
  EXPECT_FALSE(options.interlaced());
  EXPECT_FALSE(options.nitris_compat());
  EXPECT_TRUE(options.faststart());
  EXPECT_EQ(dailyboy::JobOutputVideoSignal::RangeValue::Tv,
            dailyboy::JobOutputVideoSignal::RangeValue::Tv);
}

/*!
 * \brief Loads profile dnxhr_lb and stores that enum.
 */
TEST(JobLoader, LoadJob_ProfileDnxhrLb_StoresLb) {
  // Prepare
  const std::filesystem::path yaml =
      write_dnxhd_job("profile_lb.yaml", "        profile: dnxhr_lb\n");

  // Test
  const dailyboy::JobOutputVideoDnxhd options = load_dnxhd_options(yaml);

  // Assert
  EXPECT_EQ(
      options.profile(),
      dailyboy::JobOutputVideoDnxhd::JobOutputVideoDnxhdProfileValue::DnxhrLb);
}

/*!
 * \brief Loads profile dnxhr_sq and stores that enum.
 */
TEST(JobLoader, LoadJob_ProfileDnxhrSq_StoresSq) {
  // Prepare
  const std::filesystem::path yaml =
      write_dnxhd_job("profile_sq.yaml", "        profile: dnxhr_sq\n");

  // Test
  const dailyboy::JobOutputVideoDnxhd options = load_dnxhd_options(yaml);

  // Assert
  EXPECT_EQ(
      options.profile(),
      dailyboy::JobOutputVideoDnxhd::JobOutputVideoDnxhdProfileValue::DnxhrSq);
}

/*!
 * \brief Loads profile dnxhr_hq and stores that enum.
 */
TEST(JobLoader, LoadJob_ProfileDnxhrHq_StoresHq) {
  // Prepare
  const std::filesystem::path yaml =
      write_dnxhd_job("profile_hq.yaml", "        profile: dnxhr_hq\n");

  // Test
  const dailyboy::JobOutputVideoDnxhd options = load_dnxhd_options(yaml);

  // Assert
  EXPECT_EQ(
      options.profile(),
      dailyboy::JobOutputVideoDnxhd::JobOutputVideoDnxhdProfileValue::DnxhrHq);
}

/*!
 * \brief Loads profile dnxhr_hqx and defaults pix_fmt to yuv422p10.
 */
TEST(JobLoader, LoadJob_ProfileDnxhrHqx_StoresHqxAndYuv422p10) {
  // Prepare
  const std::filesystem::path yaml =
      write_dnxhd_job("profile_hqx.yaml", "        profile: dnxhr_hqx\n");

  // Test
  const dailyboy::JobOutputVideoDnxhd options = load_dnxhd_options(yaml);

  // Assert
  EXPECT_EQ(
      options.profile(),
      dailyboy::JobOutputVideoDnxhd::JobOutputVideoDnxhdProfileValue::DnxhrHqx);
  EXPECT_EQ(
      options.pix_fmt(),
      dailyboy::JobOutputVideoDnxhd::JobOutputVideoDnxhdPixFmtValue::Yuv422p10);
}

/*!
 * \brief Loads profile dnxhr_444 and defaults pix_fmt to yuv444p10.
 */
TEST(JobLoader, LoadJob_ProfileDnxhr444_Stores444AndYuv444p10) {
  // Prepare
  const std::filesystem::path yaml =
      write_dnxhd_job("profile_444.yaml", "        profile: dnxhr_444\n");

  // Test
  const dailyboy::JobOutputVideoDnxhd options = load_dnxhd_options(yaml);

  // Assert
  EXPECT_EQ(
      options.profile(),
      dailyboy::JobOutputVideoDnxhd::JobOutputVideoDnxhdProfileValue::Dnxhr444);
  EXPECT_EQ(
      options.pix_fmt(),
      dailyboy::JobOutputVideoDnxhd::JobOutputVideoDnxhdPixFmtValue::Yuv444p10);
}

/*!
 * \brief Loads classic DNxHD with bitrate_kbps and stores both fields.
 */
TEST(JobLoader, LoadJob_ProfileDnxhdWithBitrate_StoresClassic) {
  // Prepare
  const std::filesystem::path yaml =
      write_dnxhd_job("profile_dnxhd.yaml",
                      "        profile: dnxhd\n        bitrate_kbps: 36000\n");

  // Test
  const dailyboy::JobOutputVideoDnxhd options = load_dnxhd_options(yaml);

  // Assert
  EXPECT_EQ(
      options.profile(),
      dailyboy::JobOutputVideoDnxhd::JobOutputVideoDnxhdProfileValue::Dnxhd);
  EXPECT_EQ(options.bitrate_kbps(), 36000);
}

/*!
 * \brief Loads explicit pix_fmt yuv422p with DNxHR HQ.
 */
TEST(JobLoader, LoadJob_PixFmtYuv422p_StoresYuv422p) {
  // Prepare
  const std::filesystem::path yaml =
      write_dnxhd_job("pix_fmt_422.yaml", "        pix_fmt: yuv422p\n");

  // Test
  const dailyboy::JobOutputVideoDnxhd options = load_dnxhd_options(yaml);

  // Assert
  EXPECT_EQ(
      options.pix_fmt(),
      dailyboy::JobOutputVideoDnxhd::JobOutputVideoDnxhdPixFmtValue::Yuv422p);
}

/*!
 * \brief Loads pix_fmt yuv422p10 with a compatible HQX profile.
 */
TEST(JobLoader, LoadJob_PixFmtYuv422p10_StoresYuv422p10) {
  // Prepare
  const std::filesystem::path yaml = write_dnxhd_job(
      "pix_fmt_422p10.yaml",
      "        profile: dnxhr_hqx\n        pix_fmt: yuv422p10\n");

  // Test
  const dailyboy::JobOutputVideoDnxhd options = load_dnxhd_options(yaml);

  // Assert
  EXPECT_EQ(
      options.pix_fmt(),
      dailyboy::JobOutputVideoDnxhd::JobOutputVideoDnxhdPixFmtValue::Yuv422p10);
}

/*!
 * \brief Loads pix_fmt yuv444p10 with a compatible 444 profile.
 */
TEST(JobLoader, LoadJob_PixFmtYuv444p10_StoresYuv444p10) {
  // Prepare
  const std::filesystem::path yaml = write_dnxhd_job(
      "pix_fmt_444p10.yaml",
      "        profile: dnxhr_444\n        pix_fmt: yuv444p10\n");

  // Test
  const dailyboy::JobOutputVideoDnxhd options = load_dnxhd_options(yaml);

  // Assert
  EXPECT_EQ(
      options.pix_fmt(),
      dailyboy::JobOutputVideoDnxhd::JobOutputVideoDnxhdPixFmtValue::Yuv444p10);
}

/*!
 * \brief Loads interlaced true with classic DNxHD and stores the flag.
 */
TEST(JobLoader, LoadJob_InterlacedTrue_StoresInterlaced) {
  // Prepare
  const std::filesystem::path yaml =
      write_dnxhd_job("interlaced.yaml",
                      "        profile: dnxhd\n        bitrate_kbps: 120000\n"
                      "        interlaced: true\n");

  // Test
  const dailyboy::JobOutputVideoDnxhd options = load_dnxhd_options(yaml);

  // Assert
  EXPECT_TRUE(options.interlaced());
}

/*!
 * \brief Loads nitris_compat true and stores the flag.
 */
TEST(JobLoader, LoadJob_NitrisCompatTrue_StoresTrue) {
  // Prepare
  const std::filesystem::path yaml =
      write_dnxhd_job("nitris.yaml", "        nitris_compat: true\n");

  // Test
  const dailyboy::JobOutputVideoDnxhd options = load_dnxhd_options(yaml);

  // Assert
  EXPECT_TRUE(options.nitris_compat());
}

/*!
 * \brief Loads faststart false and stores the flag.
 */
TEST(JobLoader, LoadJob_FaststartFalse_StoresFalse) {
  // Prepare
  const std::filesystem::path yaml =
      write_dnxhd_job("faststart.yaml", "        faststart: false\n");

  // Test
  const dailyboy::JobOutputVideoDnxhd options = load_dnxhd_options(yaml);

  // Assert
  EXPECT_FALSE(options.faststart());
}

/*!
 * \brief Loads classic DNxHD without bitrate_kbps and returns a user error.
 */
TEST(JobLoader, LoadJob_DnxhdWithoutBitrate_ReturnsUserError) {
  // Prepare
  const std::filesystem::path yaml =
      write_dnxhd_job("no_bitrate.yaml", "        profile: dnxhd\n");

  // Test
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_FALSE(job.ok());
  EXPECT_EQ(job.status().code(), dailyboy::Status::Code::kUser);
  EXPECT_TRUE(job.status().message().find("bitrate_kbps") != std::string::npos)
      << job.status().message();
}

/*!
 * \brief Loads a DNxHR profile with bitrate_kbps and returns a user error.
 */
TEST(JobLoader, LoadJob_HrWithBitrate_ReturnsUserError) {
  // Prepare
  const std::filesystem::path yaml = write_dnxhd_job(
      "hr_bitrate.yaml",
      "        profile: dnxhr_hq\n        bitrate_kbps: 36000\n");

  // Test
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_FALSE(job.ok());
  EXPECT_EQ(job.status().code(), dailyboy::Status::Code::kUser);
  EXPECT_TRUE(job.status().message().find("bitrate_kbps") != std::string::npos)
      << job.status().message();
}

/*!
 * \brief Loads a DNxHR profile with interlaced true and returns a user error.
 */
TEST(JobLoader, LoadJob_HrWithInterlaced_ReturnsUserError) {
  // Prepare
  const std::filesystem::path yaml =
      write_dnxhd_job("hr_interlaced.yaml",
                      "        profile: dnxhr_hq\n        interlaced: true\n");

  // Test
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_FALSE(job.ok());
  EXPECT_EQ(job.status().code(), dailyboy::Status::Code::kUser);
  EXPECT_TRUE(job.status().message().find("interlaced") != std::string::npos)
      << job.status().message();
}

/*!
 * \brief Loads HQ with pix_fmt yuv422p10 and returns a user error.
 */
TEST(JobLoader, LoadJob_PixFmtMismatch_ReturnsUserError) {
  // Prepare
  const std::filesystem::path yaml = write_dnxhd_job(
      "pix_mismatch.yaml",
      "        profile: dnxhr_hq\n        pix_fmt: yuv422p10\n");

  // Test
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_FALSE(job.ok());
  EXPECT_EQ(job.status().code(), dailyboy::Status::Code::kUser);
  EXPECT_TRUE(job.status().message().find("pix_fmt") != std::string::npos)
      << job.status().message();
}

/*!
 * \brief Validates DNxHD codec_options that use H.264 keys and fails schema.
 */
TEST(JobLoader, ValidateJobSchema_DnxhdWithH264Keys_FailsValidation) {
  // Prepare
  const std::filesystem::path yaml =
      write_dnxhd_job("h264_keys.yaml", "        crf: 18\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);

  // Assert
  EXPECT_FALSE(schema.ok()) << schema.message();
}

/*!
 * \brief Loads two videos with one disabled and keeps both entries.
 */
TEST(JobLoader, LoadJob_TwoVideosOneDisabled_LoadsBothFlags) {
  // Prepare
  const std::filesystem::path yaml =
      write_output_job("two_videos.yaml",
                       "  videos:\n"
                       "    - id: review_h264\n"
                       "      enabled: true\n"
                       "      display_view:\n"
                       "        display: \"passthrough\"\n"
                       "        view: \"passthrough\"\n"
                       "      signal:\n"
                       "        range: tv\n"
                       "        matrix: bt709\n"
                       "        primaries: bt709\n"
                       "        transfer: bt709\n"
                       "      path: /tmp/dailyboy_h264.mov\n"
                       "      codec: h264\n"
                       "    - id: review_mjpeg\n"
                       "      enabled: false\n"
                       "      display_view:\n"
                       "        display: \"passthrough\"\n"
                       "        view: \"passthrough\"\n"
                       "      signal:\n"
                       "        range: tv\n"
                       "        matrix: bt709\n"
                       "        primaries: bt709\n"
                       "        transfer: bt709\n"
                       "      path: /tmp/dailyboy_mjpeg.mov\n"
                       "      codec: mjpeg\n"
                       "  image_sequences: []\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  const std::vector<dailyboy::JobOutputVideo>& videos =
      job.value().output().videos().videos();
  ASSERT_EQ(videos.size(), 2u);
  EXPECT_EQ(videos[0].id(), "review_h264");
  EXPECT_TRUE(videos[0].enabled());
  EXPECT_EQ(videos[0].codec(),
            dailyboy::JobOutputVideo::JobOutputVideoCodecValue::H264);
  EXPECT_EQ(videos[1].id(), "review_mjpeg");
  EXPECT_FALSE(videos[1].enabled());
  EXPECT_EQ(videos[1].codec(),
            dailyboy::JobOutputVideo::JobOutputVideoCodecValue::Mjpeg);
}

/*!
 * \brief Loads a job that omits image_sequences when a video is present.
 */
TEST(JobLoader, LoadJob_VideosWithoutImageSequences_LoadsVideos) {
  // Prepare
  const std::filesystem::path yaml =
      write_output_job("videos_only.yaml",
                       "  videos:\n"
                       "    - id: review_mjpeg\n"
                       "      enabled: true\n"
                       "      display_view:\n"
                       "        display: \"passthrough\"\n"
                       "        view: \"passthrough\"\n"
                       "      signal:\n"
                       "        range: tv\n"
                       "        matrix: bt709\n"
                       "        primaries: bt709\n"
                       "        transfer: bt709\n"
                       "      path: /tmp/dailyboy_mjpeg.mov\n"
                       "      codec: mjpeg\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  EXPECT_EQ(job.value().output().videos().videos().size(), 1u);
  EXPECT_TRUE(job.value().output().image_sequences().image_sequences().empty());
}

/*!
 * \brief Loads a job that omits videos when an image sequence is present.
 */
TEST(JobLoader, LoadJob_ImageSequencesWithoutVideos_LoadsSequences) {
  // Prepare
  const std::filesystem::path yaml =
      write_output_job("sequences_only.yaml",
                       "  image_sequences:\n"
                       "    - id: archive\n"
                       "      enabled: true\n"
                       "      display_view:\n"
                       "        display: \"passthrough\"\n"
                       "        view: \"passthrough\"\n"
                       "      path_pattern: /tmp/dailyboy_archive.%04d.png\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  EXPECT_TRUE(job.value().output().videos().videos().empty());
  EXPECT_EQ(job.value().output().image_sequences().image_sequences().size(),
            1u);
}

/*!
 * \brief Validates a job whose videos are all disabled and fails schema.
 */
TEST(JobLoader, ValidateJobSchema_AllVideosDisabled_FailsValidation) {
  // Prepare
  const std::filesystem::path yaml =
      write_output_job("all_disabled.yaml",
                       "  videos:\n"
                       "    - id: preview\n"
                       "      enabled: false\n"
                       "      display_view:\n"
                       "        display: \"passthrough\"\n"
                       "        view: \"passthrough\"\n"
                       "      signal:\n"
                       "        range: tv\n"
                       "        matrix: bt709\n"
                       "        primaries: bt709\n"
                       "        transfer: bt709\n"
                       "      path: /tmp/dailyboy_disabled.mov\n"
                       "      codec: h264\n"
                       "  image_sequences: []\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);

  // Assert
  EXPECT_FALSE(schema.ok()) << schema.message();
}

/*!
 * \brief Validates a job with an empty videos list and fails schema.
 */
TEST(JobLoader, ValidateJobSchema_EmptyVideos_FailsValidation) {
  // Prepare
  const std::filesystem::path yaml =
      write_output_job("empty_videos.yaml",
                       "  videos: []\n"
                       "  image_sequences: []\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);

  // Assert
  EXPECT_FALSE(schema.ok()) << schema.message();
}

/*!
 * \brief Validates two videos that share an id and fails.
 */
TEST(JobLoader, ValidateJobSchema_DuplicateVideoId_FailsValidation) {
  // Prepare
  const std::filesystem::path yaml =
      write_output_job("dup_video_id.yaml",
                       "  videos:\n"
                       "    - id: preview\n"
                       "      enabled: true\n"
                       "      display_view:\n"
                       "        display: \"passthrough\"\n"
                       "        view: \"passthrough\"\n"
                       "      signal:\n"
                       "        range: tv\n"
                       "        matrix: bt709\n"
                       "        primaries: bt709\n"
                       "        transfer: bt709\n"
                       "      path: /tmp/dailyboy_a.mov\n"
                       "      codec: h264\n"
                       "    - id: preview\n"
                       "      enabled: true\n"
                       "      display_view:\n"
                       "        display: \"passthrough\"\n"
                       "        view: \"passthrough\"\n"
                       "      signal:\n"
                       "        range: tv\n"
                       "        matrix: bt709\n"
                       "        primaries: bt709\n"
                       "        transfer: bt709\n"
                       "      path: /tmp/dailyboy_b.mov\n"
                       "      codec: mjpeg\n"
                       "  image_sequences: []\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  EXPECT_FALSE(schema.ok()) << schema.message();
  EXPECT_TRUE(schema.message().find("duplicate id") != std::string::npos)
      << schema.message();
  ASSERT_FALSE(job.ok());
  EXPECT_EQ(job.status().code(), dailyboy::Status::Code::kUser);
  EXPECT_TRUE(job.status().message().find("duplicate id") != std::string::npos)
      << job.status().message();
}

/*!
 * \brief Validates a video and image sequence that share an id and fails.
 */
TEST(JobLoader, ValidateJobSchema_DuplicateVideoAndSequenceId_FailsValidation) {
  // Prepare
  const std::filesystem::path yaml =
      write_output_job("dup_cross_id.yaml",
                       "  videos:\n"
                       "    - id: preview\n"
                       "      enabled: true\n"
                       "      display_view:\n"
                       "        display: \"passthrough\"\n"
                       "        view: \"passthrough\"\n"
                       "      signal:\n"
                       "        range: tv\n"
                       "        matrix: bt709\n"
                       "        primaries: bt709\n"
                       "        transfer: bt709\n"
                       "      path: /tmp/dailyboy_preview.mov\n"
                       "      codec: h264\n"
                       "  image_sequences:\n"
                       "    - id: preview\n"
                       "      enabled: true\n"
                       "      display_view:\n"
                       "        display: \"passthrough\"\n"
                       "        view: \"passthrough\"\n"
                       "      path_pattern: /tmp/out.%04d.png\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);

  // Assert
  EXPECT_FALSE(schema.ok()) << schema.message();
  EXPECT_TRUE(schema.message().find("duplicate id") != std::string::npos)
      << schema.message();
}

/*!
 * \brief Loads a job with no videos and one enabled image sequence.
 */
TEST(JobLoader, LoadJob_SequenceOnly_LoadsImageSequence) {
  // Prepare
  const std::filesystem::path yaml =
      write_output_job("sequence_only.yaml",
                       "  videos: []\n"
                       "  image_sequences:\n"
                       "    - id: archive\n"
                       "      enabled: true\n"
                       "      display_view:\n"
                       "        display: \"passthrough\"\n"
                       "        view: \"passthrough\"\n"
                       "      path_pattern: /tmp/archive.%04d.png\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  EXPECT_TRUE(job.value().output().videos().videos().empty());
  ASSERT_EQ(job.value().output().image_sequences().image_sequences().size(),
            1u);
  EXPECT_TRUE(job.value()
                  .output()
                  .image_sequences()
                  .image_sequences()
                  .front()
                  .enabled());
}

/*!
 * \brief Loads three image sequences after the former maxItems cap of two.
 */
TEST(JobLoader, LoadJob_ThreeImageSequences_LoadsAll) {
  // Prepare
  const std::filesystem::path yaml =
      write_output_job("three_seqs.yaml",
                       "  videos:\n"
                       "    - id: preview\n"
                       "      enabled: true\n"
                       "      display_view:\n"
                       "        display: \"passthrough\"\n"
                       "        view: \"passthrough\"\n"
                       "      signal:\n"
                       "        range: tv\n"
                       "        matrix: bt709\n"
                       "        primaries: bt709\n"
                       "        transfer: bt709\n"
                       "      path: /tmp/preview.mov\n"
                       "      codec: h264\n"
                       "  image_sequences:\n"
                       "    - id: a\n"
                       "      enabled: true\n"
                       "      display_view:\n"
                       "        display: \"passthrough\"\n"
                       "        view: \"passthrough\"\n"
                       "      path_pattern: /tmp/a.%04d.png\n"
                       "    - id: b\n"
                       "      enabled: false\n"
                       "      display_view:\n"
                       "        display: \"passthrough\"\n"
                       "        view: \"passthrough\"\n"
                       "      path_pattern: /tmp/b.%04d.png\n"
                       "    - id: c\n"
                       "      enabled: true\n"
                       "      display_view:\n"
                       "        display: \"passthrough\"\n"
                       "        view: \"passthrough\"\n"
                       "      path_pattern: /tmp/c.%04d.png\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(schema.ok()) << schema.message();
  ASSERT_TRUE(job.ok()) << job.status().message();
  EXPECT_EQ(job.value().output().image_sequences().image_sequences().size(),
            3u);
}

/*!
 * \brief Validates a leftover format key on an image sequence and fails.
 */
TEST(JobLoader, ValidateJobSchema_ImageSequenceFormatKey_FailsValidation) {
  // Prepare
  const std::filesystem::path yaml =
      write_output_job("format_key.yaml",
                       "  videos:\n"
                       "    - id: preview\n"
                       "      enabled: true\n"
                       "      display_view:\n"
                       "        display: \"passthrough\"\n"
                       "        view: \"passthrough\"\n"
                       "      signal:\n"
                       "        range: tv\n"
                       "        matrix: bt709\n"
                       "        primaries: bt709\n"
                       "        transfer: bt709\n"
                       "      path: /tmp/preview.mov\n"
                       "      codec: h264\n"
                       "  image_sequences:\n"
                       "    - id: archive\n"
                       "      enabled: true\n"
                       "      display_view:\n"
                       "        display: \"passthrough\"\n"
                       "        view: \"passthrough\"\n"
                       "      signal:\n"
                       "        range: tv\n"
                       "        matrix: bt709\n"
                       "        primaries: bt709\n"
                       "        transfer: bt709\n"
                       "      format: png\n"
                       "      path_pattern: /tmp/archive.%04d.png\n");

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);

  // Assert
  EXPECT_FALSE(schema.ok()) << schema.message();
}

std::string layout_burn_in(const std::string& anchor,
                           const std::string& extra = "") {
  return std::string(
             "    - template: \"x\"\n"
             "      position:\n"
             "        mode: \"layout\"\n"
             "        anchor: \"") +
         anchor +
         "\"\n"
         "      font:\n"
         "        path: \"/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf\"\n"
         "        size_px: 16\n" +
         extra;
}

/*!
 * \brief Accepts all nine layout anchors and rejects the former \c center name.
 */
TEST(JobLoader, ParseAnchor_NinePointGrid_LoadsAndRejectsLegacyCenter) {
  // Prepare
  std::string burn_ins;
  const char* anchors[] = {"top_left",    "top_center",    "top_right",
                           "center_left", "center_center", "center_right",
                           "bottom_left", "bottom_center", "bottom_right"};
  const dailyboy::TextPositionModeLayout::Anchor expected[] = {
      dailyboy::TextPositionModeLayout::Anchor::TopLeft,
      dailyboy::TextPositionModeLayout::Anchor::TopCenter,
      dailyboy::TextPositionModeLayout::Anchor::TopRight,
      dailyboy::TextPositionModeLayout::Anchor::CenterLeft,
      dailyboy::TextPositionModeLayout::Anchor::CenterCenter,
      dailyboy::TextPositionModeLayout::Anchor::CenterRight,
      dailyboy::TextPositionModeLayout::Anchor::BottomLeft,
      dailyboy::TextPositionModeLayout::Anchor::BottomCenter,
      dailyboy::TextPositionModeLayout::Anchor::BottomRight};
  for (const char* anchor : anchors) {
    burn_ins += layout_burn_in(anchor);
  }
  const std::filesystem::path yaml =
      write_layout_job("nine_anchors.yaml", burn_ins);
  const std::filesystem::path legacy =
      write_layout_job("legacy_center.yaml", layout_burn_in("center"));

  // Test
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);
  dailyboy::Status legacy_schema = dailyboy::validate_job_schema(legacy);

  // Assert
  ASSERT_TRUE(job.ok()) << job.status().message();
  const auto& list = job.value().layout().burn_ins().burn_ins();
  ASSERT_EQ(list.size(), 9u);
  for (int i = 0; i < 9; ++i) {
    expect_layout_anchor(list[static_cast<std::size_t>(i)].position(),
                         expected[i]);
  }
  EXPECT_FALSE(legacy_schema.ok()) << legacy_schema.message();
}

/*!
 * \brief Defaults omitted font channels to white and honors a partial color.
 */
TEST(JobLoader, ParseTextFont_MissingAndPartialColor_DefaultsToWhite) {
  // Prepare
  const std::filesystem::path omitted =
      write_layout_job("font_no_color.yaml", layout_burn_in("top_left"));
  const std::filesystem::path partial = write_layout_job(
      "font_partial_color.yaml", layout_burn_in("top_left",
                                                "        color:\n"
                                                "          r: 0.25\n"));

  // Test
  dailyboy::StatusOr<dailyboy::Job> omitted_job = dailyboy::load_job(omitted);
  dailyboy::StatusOr<dailyboy::Job> partial_job = dailyboy::load_job(partial);

  // Assert
  ASSERT_TRUE(omitted_job.ok()) << omitted_job.status().message();
  expect_rgb(
      omitted_job.value().layout().burn_ins().burn_ins().front().font().color(),
      1.0, 1.0, 1.0);
  ASSERT_TRUE(partial_job.ok()) << partial_job.status().message();
  expect_rgb(
      partial_job.value().layout().burn_ins().burn_ins().front().font().color(),
      0.25, 1.0, 1.0);
}

/*!
 * \brief Leaves \c box unset when the YAML node is omitted.
 */
TEST(JobLoader, ParseBurnIn_OmittedBox_HasNoBox) {
  // Prepare
  const std::filesystem::path yaml =
      write_layout_job("no_box.yaml", layout_burn_in("top_left"));

  // Test
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);

  // Assert
  ASSERT_TRUE(job.ok()) << job.status().message();
  EXPECT_FALSE(
      job.value().layout().burn_ins().burn_ins().front().box().has_value());
}
