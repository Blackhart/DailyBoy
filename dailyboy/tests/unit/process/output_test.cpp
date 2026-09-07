#include "process/output.hpp"

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <string>
#include <utility>

#include "error/process.hpp"
#include "image/frame.hpp"
#include "job/job.hpp"
#include "process/colorimetry.hpp"
#include "status.hpp"
#include "support/test_fixtures.hpp"

namespace {

dailyboy::JobOutputDisplayView passthrough_display_view() {
  dailyboy::JobOutputDisplayView display_view;
  display_view.set_display("passthrough");
  display_view.set_view("passthrough");
  return display_view;
}

dailyboy::Job make_sequence_job(const std::filesystem::path& out_pattern) {
  dailyboy::Job job;
  dailyboy::JobLayoutCanvas canvas;
  canvas.set_width(dailyboy::test::kPlateWidth);
  canvas.set_height(dailyboy::test::kPlateHeight);
  job.layout().set_canvas(std::move(canvas));

  dailyboy::JobOutputImageSequence item;
  item.set_id("archive");
  item.set_enabled(true);
  item.set_display_view(passthrough_display_view());
  item.set_path_pattern(out_pattern.string());
  job.output().image_sequences().image_sequences().push_back(std::move(item));
  return job;
}

dailyboy::Frame solid_frame(float r, float g, float b) {
  OIIO::ImageSpec spec(dailyboy::test::kPlateWidth,
                       dailyboy::test::kPlateHeight, 3, OIIO::TypeDesc::FLOAT);
  OIIO::ImageBuf buf(spec);
  const float rgb[3] = {r, g, b};
  EXPECT_TRUE(OIIO::ImageBufAlgo::fill(buf, OIIO::cspan<float>(rgb, 3)));
  return dailyboy::Frame(std::move(buf));
}

dailyboy::ColorPipeline must_prepare(const dailyboy::Job& job) {
  dailyboy::StatusOr<dailyboy::ColorPipeline> prepared =
      dailyboy::ColorPipeline::prepare(job);
  EXPECT_TRUE(prepared.ok()) << prepared.status().message();
  return std::move(prepared).value();
}

}  // namespace

/*!
 * \brief Opens writers from the output section even when plans are empty.
 */
TEST(Outputs, Open_EmptyPlansWithSequence_Succeeds) {
  // Prepare
  const std::filesystem::path dir =
      std::filesystem::path(DAILYBOY_TEST_BINARY_DIR) / "outputs_no_plans";
  std::error_code ec;
  std::filesystem::create_directories(dir, ec);
  dailyboy::Job job = make_sequence_job(dir / "out.%04d.png");
  dailyboy::ColorPipeline color_pipeline = must_prepare(job);

  // Test
  dailyboy::StatusOr<dailyboy::Outputs> out = dailyboy::Outputs::open(job);

  // Assert
  ASSERT_TRUE(out.ok()) << out.status().message();
  EXPECT_TRUE(out.value().close().ok());
}

/*!
 * \brief Rejects a job with no enabled deliverable.
 */
TEST(Outputs, Open_NoEnabledDeliverable_ReturnsRenderUserError3) {
  // Prepare
  dailyboy::Job job;
  dailyboy::ColorPipeline color_pipeline = must_prepare(job);

  // Test
  dailyboy::StatusOr<dailyboy::Outputs> out = dailyboy::Outputs::open(job);

  // Assert
  ASSERT_FALSE(out.ok());
  EXPECT_EQ(out.status().message(), std::string(dailyboy::USER_ERROR_RENDER_3));
}

/*!
 * \brief Writes one PNG through output then close.
 */
TEST(Outputs, Output_SequenceOnly_WritesPng) {
  // Prepare
  const std::filesystem::path dir =
      std::filesystem::path(DAILYBOY_TEST_BINARY_DIR) / "outputs_test";
  std::error_code ec;
  std::filesystem::create_directories(dir, ec);
  const std::filesystem::path pattern = dir / "out.%04d.png";
  dailyboy::Job job = make_sequence_job(pattern);
  dailyboy::ColorPipeline color_pipeline = must_prepare(job);
  dailyboy::Frame frame = solid_frame(0.2f, 0.4f, 0.6f);

  // Test
  dailyboy::StatusOr<dailyboy::Outputs> opened = dailyboy::Outputs::open(job);
  ASSERT_TRUE(opened.ok()) << opened.status().message();
  dailyboy::Outputs out = std::move(opened.value());
  const dailyboy::Status written =
      dailyboy::write_outputs(out, frame, 1001, passthrough_display_view());
  const dailyboy::Status closed = out.close();

  // Assert
  ASSERT_TRUE(written.ok()) << written.message();
  ASSERT_TRUE(closed.ok()) << closed.message();
  EXPECT_TRUE(std::filesystem::is_regular_file(dir / "out.1001.png"));
}

/*!
 * \brief close is a no-op the second time.
 */
TEST(Outputs, Close_AlreadyClosed_IsOk) {
  // Prepare
  const std::filesystem::path dir =
      std::filesystem::path(DAILYBOY_TEST_BINARY_DIR) / "outputs_close_test";
  std::error_code ec;
  std::filesystem::create_directories(dir, ec);
  dailyboy::Job job = make_sequence_job(dir / "out.%04d.png");
  dailyboy::ColorPipeline color_pipeline = must_prepare(job);
  dailyboy::StatusOr<dailyboy::Outputs> opened = dailyboy::Outputs::open(job);
  ASSERT_TRUE(opened.ok()) << opened.status().message();
  dailyboy::Outputs out = std::move(opened.value());
  ASSERT_TRUE(out.close().ok());

  // Test
  const dailyboy::Status status = out.close();

  // Assert
  EXPECT_TRUE(status.ok()) << status.message();
}

/*!
 * \brief write_movie is a no-op when no video is enabled.
 */
TEST(Outputs, WriteMovie_SequenceOnly_IsNoOp) {
  // Prepare
  const std::filesystem::path dir =
      std::filesystem::path(DAILYBOY_TEST_BINARY_DIR) / "outputs_movie_noop";
  std::error_code ec;
  std::filesystem::create_directories(dir, ec);
  dailyboy::Job job = make_sequence_job(dir / "out.%04d.png");
  dailyboy::ColorPipeline color_pipeline = must_prepare(job);
  dailyboy::Frame frame = solid_frame(0.1f, 0.1f, 0.1f);
  dailyboy::StatusOr<dailyboy::Outputs> opened = dailyboy::Outputs::open(job);
  ASSERT_TRUE(opened.ok()) << opened.status().message();
  dailyboy::Outputs out = std::move(opened.value());

  // Test
  const dailyboy::Status status =
      dailyboy::write_movie(out, frame, passthrough_display_view());
  ASSERT_TRUE(out.close().ok());

  // Assert
  EXPECT_TRUE(status.ok()) << status.message();
}

/*!
 * \brief write_sequence writes the PNG for the given frame number.
 */
TEST(Outputs, WriteSequence_SequenceOnly_WritesPng) {
  // Prepare
  const std::filesystem::path dir =
      std::filesystem::path(DAILYBOY_TEST_BINARY_DIR) / "outputs_write_seq";
  std::error_code ec;
  std::filesystem::create_directories(dir, ec);
  dailyboy::Job job = make_sequence_job(dir / "out.%04d.png");
  dailyboy::ColorPipeline color_pipeline = must_prepare(job);
  dailyboy::Frame frame = solid_frame(0.5f, 0.5f, 0.5f);
  dailyboy::StatusOr<dailyboy::Outputs> opened = dailyboy::Outputs::open(job);
  ASSERT_TRUE(opened.ok()) << opened.status().message();
  dailyboy::Outputs out = std::move(opened.value());

  // Test
  const dailyboy::Status status =
      dailyboy::write_sequence(out, frame, 1002, passthrough_display_view());
  ASSERT_TRUE(out.close().ok());

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  EXPECT_TRUE(std::filesystem::is_regular_file(dir / "out.1002.png"));
}
