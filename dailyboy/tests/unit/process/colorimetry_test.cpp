#include "process/colorimetry.hpp"

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>
#include <gtest/gtest.h>

#include <map>
#include <string>
#include <utility>

#include "error/color.hpp"
#include "image/frame.hpp"
#include "job/job.hpp"
#include "job/output.hpp"
#include "job/plans.hpp"

namespace {

dailyboy::Frame solid_rgb() {
  OIIO::ImageSpec spec(2, 2, 3, OIIO::TypeDesc::FLOAT);
  OIIO::ImageBuf buf(spec);
  const float rgb[3] = {0.1f, 0.2f, 0.3f};
  EXPECT_TRUE(OIIO::ImageBufAlgo::fill(buf, OIIO::cspan<float>(rgb, 3)));
  return dailyboy::Frame(std::move(buf));
}

dailyboy::JobOutputDisplayView passthrough_display_view() {
  dailyboy::JobOutputDisplayView display_view;
  display_view.set_display("passthrough");
  display_view.set_view("passthrough");
  return display_view;
}

dailyboy::Job job_with_spaces(const std::string& input,
                              const std::string& working,
                              const std::string& ocio_path) {
  dailyboy::Job job;
  dailyboy::JobPlan plan;
  plan.set_input_colorspace(input);
  job.plans().plans().push_back(std::move(plan));
  job.colorimetry().set_ocio_config(ocio_path);
  job.colorimetry().set_working_colorspace(working);
  return job;
}

void add_passthrough_output(dailyboy::Job& job) {
  dailyboy::JobOutputImageSequence item;
  item.set_id("out");
  item.set_enabled(true);
  item.set_display_view(passthrough_display_view());
  item.set_path_pattern("/unused/out.%04d.exr");
  job.output().image_sequences().image_sequences().push_back(std::move(item));
}

dailyboy::ColorPipeline must_prepare(const dailyboy::Job& job) {
  dailyboy::StatusOr<dailyboy::ColorPipeline> prepared =
      dailyboy::ColorPipeline::prepare(job);
  EXPECT_TRUE(prepared.ok()) << prepared.status().message();
  return std::move(prepared).value();
}

}  // namespace

/*!
 * \brief Same input and working spaces leave pixels unchanged without OCIO.
 */
TEST(Colorimetry,
     ConvertColorFromInputToWorking_SameSpace_LeavesPixelsWithoutConfig) {
  // Prepare
  dailyboy::Frame frame = solid_rgb();
  dailyboy::Job job =
      job_with_spaces("ACES - ACEScg", "ACES - ACEScg", "/unused/config.ocio");
  dailyboy::ColorPipeline color_pipeline = must_prepare(job);

  // Test
  const dailyboy::Status status =
      dailyboy::convert_color_from_input_to_working(frame, color_pipeline);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  float px[3] = {0.0f, 0.0f, 0.0f};
  ASSERT_TRUE(
      frame.buf().get_pixels(OIIO::ROI(0, 1, 0, 1), OIIO::TypeDesc::FLOAT, px));
  EXPECT_FLOAT_EQ(px[0], 0.1f);
  EXPECT_FLOAT_EQ(px[1], 0.2f);
  EXPECT_FLOAT_EQ(px[2], 0.3f);
}

/*!
 * \brief Passthrough display_view leaves pixels unchanged without OCIO.
 */
TEST(Colorimetry,
     ConvertColorFromWorkingToDisplay_Passthrough_LeavesPixelsWithoutConfig) {
  // Prepare
  dailyboy::Frame frame = solid_rgb();
  dailyboy::Job job =
      job_with_spaces("ACES - ACEScg", "ACES - ACEScg", "/unused/config.ocio");
  add_passthrough_output(job);
  dailyboy::ColorPipeline color_pipeline = must_prepare(job);

  // Test
  const dailyboy::Status status =
      dailyboy::convert_color_from_working_to_display(
          frame, color_pipeline, passthrough_display_view());

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  float px[3] = {0.0f, 0.0f, 0.0f};
  ASSERT_TRUE(
      frame.buf().get_pixels(OIIO::ROI(0, 1, 0, 1), OIIO::TypeDesc::FLOAT, px));
  EXPECT_FLOAT_EQ(px[0], 0.1f);
  EXPECT_FLOAT_EQ(px[1], 0.2f);
  EXPECT_FLOAT_EQ(px[2], 0.3f);
}

/*!
 * \brief Missing OCIO config fails at prepare when spaces differ.
 */
TEST(Colorimetry, Prepare_MissingConfig_ReturnsUserError) {
  // Prepare
  dailyboy::Job job = job_with_spaces("Input - Linear", "ACES - ACEScg",
                                      "/unused/missing.ocio");

  // Test
  const dailyboy::StatusOr<dailyboy::ColorPipeline> prepared =
      dailyboy::ColorPipeline::prepare(job);

  // Assert
  ASSERT_FALSE(prepared.ok());
  EXPECT_NE(prepared.status().message().find(dailyboy::USER_ERROR_COLOR_1),
            std::string::npos)
      << prepared.status().message();
}

/*!
 * \brief Missing OCIO config fails when a real DisplayView is required.
 */
TEST(Colorimetry, Prepare_RealDisplayViewMissingConfig_ReturnsUserError) {
  // Prepare
  dailyboy::Job job =
      job_with_spaces("ACES - ACEScg", "ACES - ACEScg", "/unused/missing.ocio");
  dailyboy::JobOutputImageSequence item;
  item.set_id("out");
  item.set_enabled(true);
  dailyboy::JobOutputDisplayView display_view;
  display_view.set_display("Rec.1886 Rec.709 - Display");
  display_view.set_view("ACES 2.0 - SDR 100 nits (Rec.709)");
  item.set_display_view(std::move(display_view));
  item.set_path_pattern("/unused/out.%04d.exr");
  job.output().image_sequences().image_sequences().push_back(std::move(item));

  // Test
  const dailyboy::StatusOr<dailyboy::ColorPipeline> prepared =
      dailyboy::ColorPipeline::prepare(job);

  // Assert
  ASSERT_FALSE(prepared.ok());
  EXPECT_NE(prepared.status().message().find(dailyboy::USER_ERROR_COLOR_1),
            std::string::npos)
      << prepared.status().message();
}

/*!
 * \brief Expands a working_colorspace substitution before comparing spaces.
 */
TEST(Colorimetry, Prepare_WorkingColorspaceToken_ExpandsBeforeIdentityCheck) {
  // Prepare
  dailyboy::Job job = job_with_spaces("ACES - ACEScg", "{working_colorspace}",
                                      "/unused/missing.ocio");
  job.metadata().substitutions()["working_colorspace"] =
      std::string("ACES - ACEScg");

  // Test
  const dailyboy::StatusOr<dailyboy::ColorPipeline> prepared =
      dailyboy::ColorPipeline::prepare(job);

  // Assert
  ASSERT_TRUE(prepared.ok()) << prepared.status().message();
}

/*!
 * \brief Rejects a frame-map substitution in working_colorspace.
 */
TEST(Colorimetry, Prepare_WorkingColorspaceFrameMap_ReturnsUserError) {
  // Prepare
  dailyboy::Job job = job_with_spaces("ACES - ACEScg", "{working_colorspace}",
                                      "/unused/missing.ocio");
  job.metadata().substitutions()["working_colorspace"] =
      std::map<std::string, std::string>{{"1001", "ACES - ACEScg"}};

  // Test
  const dailyboy::StatusOr<dailyboy::ColorPipeline> prepared =
      dailyboy::ColorPipeline::prepare(job);

  // Assert
  ASSERT_FALSE(prepared.ok());
  EXPECT_NE(prepared.status().message().find(dailyboy::USER_ERROR_COLOR_4),
            std::string::npos)
      << prepared.status().message();
}
