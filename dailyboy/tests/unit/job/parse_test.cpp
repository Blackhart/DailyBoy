#include <gtest/gtest.h>
#include <yaml-cpp/yaml.h>

#include <string>
#include <variant>

#include "error/job.hpp"
#include "job/parse_colorimetry.hpp"
#include "job/parse_layout.hpp"
#include "job/parse_metadata.hpp"
#include "job/parse_output.hpp"
#include "job/parse_plans.hpp"

/*!
 * \brief Parses a three-channel RGB map.
 */
TEST(Parse, ParseRgbColor_ThreeChannels_SetsRgb) {
  // Prepare
  const YAML::Node node = YAML::Load("{r: 0.1, g: 0.2, b: 0.3}");

  // Test
  dailyboy::StatusOr<dailyboy::RGBColor> color =
      dailyboy::parse_rgb_color(node, "layout.background");

  // Assert
  ASSERT_TRUE(color.ok()) << color.status().message();
  EXPECT_DOUBLE_EQ(color.value().r(), 0.1);
  EXPECT_DOUBLE_EQ(color.value().g(), 0.2);
  EXPECT_DOUBLE_EQ(color.value().b(), 0.3);
}

/*!
 * \brief Missing channel uses the default fill.
 */
TEST(Parse, ParseRgbColor_MissingBlue_UsesDefault) {
  // Prepare
  const YAML::Node node = YAML::Load("{r: 0.1, g: 0.2}");

  // Test
  dailyboy::StatusOr<dailyboy::RGBColor> color =
      dailyboy::parse_rgb_color(node, "layout.background", 0.5);

  // Assert
  ASSERT_TRUE(color.ok()) << color.status().message();
  EXPECT_DOUBLE_EQ(color.value().b(), 0.5);
}

/*!
 * \brief Parses a one-plan sequence block.
 */
TEST(Parse, ParsePlans_OnePlan_SetsIdAndRange) {
  // Prepare
  const YAML::Node node = YAML::Load(R"(
- id: plate
  input_colorspace: ACES - ACEScg
  sequence:
    path: "/tmp/plate.%04d.exr"
    frame_start: 1001
    frame_end: 1003
)");

  // Test
  dailyboy::StatusOr<dailyboy::JobPlans> plans = dailyboy::parse_plans(node);

  // Assert
  ASSERT_TRUE(plans.ok()) << plans.status().message();
  ASSERT_EQ(plans.value().plans().size(), 1u);
  EXPECT_EQ(plans.value().plans()[0].id(), "plate");
  EXPECT_EQ(plans.value().plans()[0].sequence().frame_start(), 1001);
  EXPECT_EQ(plans.value().plans()[0].sequence().frame_end(), 1003);
}

/*!
 * \brief Absent metadata yields empty substitutions.
 */
TEST(Parse, ParseMetadata_NullNode_ReturnsEmptySubstitutions) {
  // Prepare
  const YAML::Node node;

  // Test
  dailyboy::StatusOr<dailyboy::JobMetadata> metadata =
      dailyboy::parse_metadata(node);

  // Assert
  ASSERT_TRUE(metadata.ok()) << metadata.status().message();
  EXPECT_TRUE(metadata.value().substitutions().empty());
}

/*!
 * \brief Parses ocio_config from a color map.
 */
TEST(Parse, ParseColorimetry_OcioConfig_SetsPath) {
  // Prepare
  const YAML::Node node = YAML::Load(R"(
ocio_config: "/tmp/config.ocio"
working_colorspace: "ACES - ACEScg"
)");

  // Test
  dailyboy::StatusOr<dailyboy::JobColorimetry> color =
      dailyboy::parse_colorimetry(node);

  // Assert
  ASSERT_TRUE(color.ok()) << color.status().message();
  EXPECT_EQ(color.value().ocio_config().string(), "/tmp/config.ocio");
  EXPECT_EQ(color.value().working_colorspace(), "ACES - ACEScg");
}

/*!
 * \brief Parses an even canvas and contain fit.
 */
TEST(Parse, ParseLayout_EvenCanvas_SetsWidthHeightAndFit) {
  // Prepare
  const YAML::Node node = YAML::Load(R"(
canvas:
  width: 8
  height: 8
image:
  fit: "contain"
)");

  // Test
  dailyboy::StatusOr<dailyboy::JobLayout> layout = dailyboy::parse_layout(node);

  // Assert
  ASSERT_TRUE(layout.ok()) << layout.status().message();
  EXPECT_EQ(layout.value().canvas().width(), 8);
  EXPECT_EQ(layout.value().canvas().height(), 8);
}

/*!
 * \brief Empty videos and image_sequences is a contract error (nothing enabled).
 */
TEST(Parse, ParseOutput_EmptyLists_ReturnsJobUserError36) {
  // Prepare
  const YAML::Node node = YAML::Load(R"(
videos: []
image_sequences: []
)");

  // Test
  dailyboy::StatusOr<dailyboy::JobOutput> output = dailyboy::parse_output(node);

  // Assert
  ASSERT_FALSE(output.ok());
  EXPECT_NE(
      output.status().message().find(std::string(dailyboy::USER_ERROR_JOB_36)),
      std::string::npos);
}

/*!
 * \brief Parses one enabled image sequence.
 */
TEST(Parse, ParseOutput_OneSequence_SetsIdAndPattern) {
  // Prepare
  const YAML::Node node = YAML::Load(R"(
videos: []
image_sequences:
  - id: archive
    enabled: true
    display_view:
      display: "passthrough"
      view: "passthrough"
    path_pattern: /tmp/out.%04d.png
)");

  // Test
  dailyboy::StatusOr<dailyboy::JobOutput> output = dailyboy::parse_output(node);

  // Assert
  ASSERT_TRUE(output.ok()) << output.status().message();
  ASSERT_EQ(output.value().image_sequences().image_sequences().size(), 1u);
  EXPECT_EQ(output.value().image_sequences().image_sequences()[0].id(),
            "archive");
  EXPECT_EQ(
      output.value().image_sequences().image_sequences()[0].path_pattern(),
      "/tmp/out.%04d.png");
  EXPECT_FALSE(output.value()
                   .image_sequences()
                   .image_sequences()[0]
                   .format_options()
                   .has_value());
}

/*!
 * \brief Parses PNG format_options from a .png path_pattern.
 */
TEST(Parse, ParseOutput_PngFormatOptions_SetsBitDepthAndCompression) {
  // Prepare
  const YAML::Node node = YAML::Load(R"(
videos: []
image_sequences:
  - id: archive
    enabled: true
    display_view:
      display: "passthrough"
      view: "passthrough"
    path_pattern: /tmp/out.%04d.png
    format_options:
      bit_depth: 16
      compression: filtered
      compression_level: 3
      filter: 2
)");

  // Test
  dailyboy::StatusOr<dailyboy::JobOutput> output = dailyboy::parse_output(node);

  // Assert
  ASSERT_TRUE(output.ok()) << output.status().message();
  const auto& opts =
      output.value().image_sequences().image_sequences()[0].format_options();
  ASSERT_TRUE(opts.has_value());
  const auto& png = std::get<dailyboy::JobOutputImageSequencePng>(*opts);
  EXPECT_EQ(png.bit_depth(), 16);
  EXPECT_EQ(png.compression(), "filtered");
  EXPECT_EQ(png.compression_level(), 3);
  EXPECT_EQ(png.filter(), 2);
}

/*!
 * \brief Rejects format_options when path_pattern has an unknown extension.
 */
TEST(Parse, ParseOutput_UnknownExtensionWithOptions_ReturnsJobUserError93) {
  // Prepare
  const YAML::Node node = YAML::Load(R"(
videos: []
image_sequences:
  - id: archive
    enabled: true
    display_view:
      display: "passthrough"
      view: "passthrough"
    path_pattern: /tmp/out.%04d.dpx
    format_options:
      bit_depth: 8
)");

  // Test
  dailyboy::StatusOr<dailyboy::JobOutput> output = dailyboy::parse_output(node);

  // Assert
  ASSERT_FALSE(output.ok());
  EXPECT_NE(
      output.status().message().find(std::string(dailyboy::USER_ERROR_JOB_93)),
      std::string::npos);
}

/*!
 * \brief Rejects EXR compression_level when compression forbids it.
 */
TEST(Parse, ParseOutput_ExrLevelWithPiz_ReturnsJobUserError94) {
  // Prepare
  const YAML::Node node = YAML::Load(R"(
videos: []
image_sequences:
  - id: archive
    enabled: true
    display_view:
      display: "passthrough"
      view: "passthrough"
    path_pattern: /tmp/out.%04d.exr
    format_options:
      bit_depth: h16
      compression: piz
      compression_level: 4
)");

  // Test
  dailyboy::StatusOr<dailyboy::JobOutput> output = dailyboy::parse_output(node);

  // Assert
  ASSERT_FALSE(output.ok());
  EXPECT_NE(
      output.status().message().find(std::string(dailyboy::USER_ERROR_JOB_94)),
      std::string::npos);
}

/*!
 * \brief Parses JPEG format_options including quality as compression_level.
 */
TEST(Parse, ParseOutput_JpegFormatOptions_SetsQualityAndSubsampling) {
  // Prepare
  const YAML::Node node = YAML::Load(R"(
videos: []
image_sequences:
  - id: review
    enabled: true
    display_view:
      display: "passthrough"
      view: "passthrough"
    path_pattern: /tmp/out.%04d.jpg
    format_options:
      bit_depth: 8
      compression_level: 85
      subsampling: "4:2:0"
      progressive: true
)");

  // Test
  dailyboy::StatusOr<dailyboy::JobOutput> output = dailyboy::parse_output(node);

  // Assert
  ASSERT_TRUE(output.ok()) << output.status().message();
  const auto& opts =
      output.value().image_sequences().image_sequences()[0].format_options();
  ASSERT_TRUE(opts.has_value());
  const auto& jpeg = std::get<dailyboy::JobOutputImageSequenceJpeg>(*opts);
  EXPECT_EQ(jpeg.bit_depth(), 8);
  EXPECT_EQ(jpeg.compression_level(), 85);
  EXPECT_EQ(jpeg.subsampling(), "4:2:0");
  EXPECT_TRUE(jpeg.progressive());
}
