#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>
#include <OpenImageIO/imageio.h>
#include <gtest/gtest.h>

#include <array>
#include <utility>

#include "error/process.hpp"
#include "image/frame.hpp"
#include "image/geom.hpp"
#include "job/job.hpp"
#include "job/layout.hpp"
#include "process/colorimetry.hpp"
#include "process/compositing.hpp"
#include "process/reformat.hpp"
#include "process/tokens.hpp"
#include "status.hpp"

namespace {

dailyboy::JobOutputDisplayView passthrough_display_view() {
  dailyboy::JobOutputDisplayView display_view;
  display_view.set_display("passthrough");
  display_view.set_view("passthrough");
  return display_view;
}

OIIO::ImageBuf make_rgb(int width, int height, const float* fill) {
  OIIO::ImageSpec spec(width, height, 3, OIIO::TypeDesc::FLOAT);
  OIIO::ImageBuf buf(spec);
  EXPECT_TRUE(OIIO::ImageBufAlgo::fill(buf, OIIO::cspan<float>(fill, 3)));
  return buf;
}

std::array<float, 3> pixel_rgb(const OIIO::ImageBuf& buf, int x, int y) {
  float px[3] = {0.0f, 0.0f, 0.0f};
  EXPECT_TRUE(
      buf.get_pixels(OIIO::ROI(x, x + 1, y, y + 1), OIIO::TypeDesc::FLOAT, px));
  return {px[0], px[1], px[2]};
}

dailyboy::Job make_job(int canvas_w, int canvas_h,
                       dailyboy::JobLayoutImage::JobLayoutImageFitValue fit,
                       double par, dailyboy::Margin margin,
                       dailyboy::JobLayoutBackground background) {
  dailyboy::JobLayout layout;
  dailyboy::JobLayoutCanvas canvas;
  canvas.set_width(canvas_w);
  canvas.set_height(canvas_h);
  layout.set_canvas(std::move(canvas));
  dailyboy::JobLayoutPixelAspect pixel_aspect;
  pixel_aspect.set_aspect(par);
  layout.set_pixel_aspect(std::move(pixel_aspect));
  dailyboy::JobLayoutImage image;
  image.set_fit(fit);
  image.set_min_margin_px(std::move(margin));
  layout.set_image(std::move(image));
  layout.set_background(std::move(background));
  dailyboy::Job job;
  job.set_layout(std::move(layout));
  return job;
}

dailyboy::JobLayoutBackground rgb(double r, double g, double b) {
  dailyboy::JobLayoutBackground color;
  color.set_r(r);
  color.set_g(g);
  color.set_b(b);
  return color;
}

void expect_rgb(const OIIO::ImageBuf& buf, int x, int y, float r, float g,
                float b) {
  const std::array<float, 3> px = pixel_rgb(buf, x, y);
  EXPECT_FLOAT_EQ(px[0], r) << "at (" << x << "," << y << ")";
  EXPECT_FLOAT_EQ(px[1], g) << "at (" << x << "," << y << ")";
  EXPECT_FLOAT_EQ(px[2], b) << "at (" << x << "," << y << ")";
}

dailyboy::ColorPipeline must_prepare(const dailyboy::Job& job) {
  dailyboy::StatusOr<dailyboy::ColorPipeline> prepared =
      dailyboy::ColorPipeline::prepare(job);
  EXPECT_TRUE(prepared.ok()) << prepared.status().message();
  return std::move(prepared).value();
}

dailyboy::StatusOr<dailyboy::Frame> compose_plate(
    dailyboy::Frame plate, const dailyboy::Job& job,
    const dailyboy::ColorPipeline& color_pipeline) {
  dailyboy::Status status = dailyboy::reformat(plate);
  if (!status.ok()) {
    return status;
  }
  status = dailyboy::convert_color_from_input_to_working(plate, color_pipeline);
  if (!status.ok()) {
    return status;
  }
  dailyboy::StatusOr<dailyboy::FittedPlate> fitted =
      dailyboy::resize_plate(plate, job);
  if (!fitted.ok()) {
    return fitted.status();
  }
  status = dailyboy::convert_color_from_working_to_display(
      fitted.value().frame, color_pipeline, passthrough_display_view());
  if (!status.ok()) {
    return status;
  }
  dailyboy::StatusOr<dailyboy::Frame> canvas =
      dailyboy::make_filled_canvas(job);
  if (!canvas.ok()) {
    return canvas.status();
  }
  return dailyboy::compose(std::move(canvas).value(), fitted.value(), job,
                           dailyboy::OverlayTokenContext{});
}

}  // namespace

/*!
 * \brief Leaves an 8x8 plate unchanged when the canvas matches (contain,
 *        square pixels, no margins).
 */
TEST(Compose, Compose_MatchingCanvas_LeavesPixels) {
  // Prepare
  const float plate[3] = {0.1f, 0.2f, 0.3f};
  dailyboy::Frame frame(make_rgb(8, 8, plate));
  dailyboy::Job job =
      make_job(8, 8, dailyboy::JobLayoutImage::JobLayoutImageFitValue::Contain,
               1.0, dailyboy::Margin{}, rgb(1.0, 0.0, 0.0));
  dailyboy::ColorPipeline color_pipeline = must_prepare(job);

  // Test
  dailyboy::StatusOr<dailyboy::Frame> result =
      compose_plate(std::move(frame), job, color_pipeline);

  // Assert
  ASSERT_TRUE(result.ok()) << result.status().message();
  EXPECT_EQ(result.value().width(), 8);
  EXPECT_EQ(result.value().height(), 8);
  expect_rgb(result.value().buf(), 0, 0, 0.1f, 0.2f, 0.3f);
  expect_rgb(result.value().buf(), 7, 7, 0.1f, 0.2f, 0.3f);
}

/*!
 * \brief Pillarboxes a 4x8 portrait plate in an 8x8 canvas with contain.
 */
TEST(Compose, Compose_PortraitContain_Pillarboxes) {
  // Prepare
  const float plate[3] = {1.0f, 0.0f, 0.0f};
  dailyboy::Frame frame(make_rgb(4, 8, plate));
  dailyboy::Job job =
      make_job(8, 8, dailyboy::JobLayoutImage::JobLayoutImageFitValue::Contain,
               1.0, dailyboy::Margin{}, rgb(0.0, 0.0, 1.0));
  dailyboy::ColorPipeline color_pipeline = must_prepare(job);

  // Test
  dailyboy::StatusOr<dailyboy::Frame> result =
      compose_plate(std::move(frame), job, color_pipeline);

  // Assert
  ASSERT_TRUE(result.ok()) << result.status().message();
  EXPECT_EQ(result.value().width(), 8);
  EXPECT_EQ(result.value().height(), 8);
  expect_rgb(result.value().buf(), 0, 0, 0.0f, 0.0f, 1.0f);
  expect_rgb(result.value().buf(), 1, 4, 0.0f, 0.0f, 1.0f);
  expect_rgb(result.value().buf(), 2, 0, 1.0f, 0.0f, 0.0f);
  expect_rgb(result.value().buf(), 5, 7, 1.0f, 0.0f, 0.0f);
  expect_rgb(result.value().buf(), 6, 4, 0.0f, 0.0f, 1.0f);
  expect_rgb(result.value().buf(), 7, 7, 0.0f, 0.0f, 1.0f);
}

/*!
 * \brief Fills an 8x8 canvas from an 8x4 landscape plate with cover (no
 *        background left).
 */
TEST(Compose, Compose_LandscapeCover_FillsCanvas) {
  // Prepare
  const float plate[3] = {1.0f, 0.0f, 0.0f};
  dailyboy::Frame frame(make_rgb(8, 4, plate));
  dailyboy::Job job =
      make_job(8, 8, dailyboy::JobLayoutImage::JobLayoutImageFitValue::Cover,
               1.0, dailyboy::Margin{}, rgb(0.0, 1.0, 0.0));
  dailyboy::ColorPipeline color_pipeline = must_prepare(job);

  // Test
  dailyboy::StatusOr<dailyboy::Frame> result =
      compose_plate(std::move(frame), job, color_pipeline);

  // Assert
  ASSERT_TRUE(result.ok()) << result.status().message();
  EXPECT_EQ(result.value().width(), 8);
  EXPECT_EQ(result.value().height(), 8);
  expect_rgb(result.value().buf(), 0, 0, 1.0f, 0.0f, 0.0f);
  expect_rgb(result.value().buf(), 7, 7, 1.0f, 0.0f, 0.0f);
  expect_rgb(result.value().buf(), 3, 4, 1.0f, 0.0f, 0.0f);
}

/*!
 * \brief Keeps left/right min_margin_px as background when containing 8x8
 *        into 8x8.
 */
TEST(Compose, Compose_SideMargins_KeepBackground) {
  // Prepare
  const float plate[3] = {1.0f, 0.0f, 0.0f};
  dailyboy::Frame frame(make_rgb(8, 8, plate));
  dailyboy::Margin margin;
  margin.set_left(2);
  margin.set_right(2);
  dailyboy::Job job =
      make_job(8, 8, dailyboy::JobLayoutImage::JobLayoutImageFitValue::Contain,
               1.0, margin, rgb(0.0, 0.0, 1.0));
  dailyboy::ColorPipeline color_pipeline = must_prepare(job);

  // Test
  dailyboy::StatusOr<dailyboy::Frame> result =
      compose_plate(std::move(frame), job, color_pipeline);

  // Assert
  ASSERT_TRUE(result.ok()) << result.status().message();
  EXPECT_EQ(result.value().width(), 8);
  EXPECT_EQ(result.value().height(), 8);
  expect_rgb(result.value().buf(), 0, 4, 0.0f, 0.0f, 1.0f);
  expect_rgb(result.value().buf(), 1, 4, 0.0f, 0.0f, 1.0f);
  expect_rgb(result.value().buf(), 6, 4, 0.0f, 0.0f, 1.0f);
  expect_rgb(result.value().buf(), 7, 4, 0.0f, 0.0f, 1.0f);
  expect_rgb(result.value().buf(), 3, 3, 1.0f, 0.0f, 0.0f);
  expect_rgb(result.value().buf(), 4, 4, 1.0f, 0.0f, 0.0f);
}

/*!
 * \brief Pillarboxes an 8x8 plate on an 8x8 canvas when pixel_aspect is 2.
 */
TEST(Compose, Compose_PixelAspectTwo_Pillarboxes) {
  // Prepare
  const float plate[3] = {1.0f, 0.0f, 0.0f};
  dailyboy::Frame frame(make_rgb(8, 8, plate));
  dailyboy::Job job =
      make_job(8, 8, dailyboy::JobLayoutImage::JobLayoutImageFitValue::Contain,
               2.0, dailyboy::Margin{}, rgb(0.0, 0.0, 1.0));
  dailyboy::ColorPipeline color_pipeline = must_prepare(job);

  // Test
  dailyboy::StatusOr<dailyboy::Frame> result =
      compose_plate(std::move(frame), job, color_pipeline);

  // Assert
  ASSERT_TRUE(result.ok()) << result.status().message();
  EXPECT_EQ(result.value().width(), 8);
  EXPECT_EQ(result.value().height(), 8);
  expect_rgb(result.value().buf(), 0, 0, 0.0f, 0.0f, 1.0f);
  expect_rgb(result.value().buf(), 1, 4, 0.0f, 0.0f, 1.0f);
  expect_rgb(result.value().buf(), 2, 0, 1.0f, 0.0f, 0.0f);
  expect_rgb(result.value().buf(), 5, 7, 1.0f, 0.0f, 0.0f);
  expect_rgb(result.value().buf(), 6, 4, 0.0f, 0.0f, 1.0f);
  expect_rgb(result.value().buf(), 7, 7, 0.0f, 0.0f, 1.0f);
}

/*!
 * \brief Rejects min_margin_px that consumes the whole canvas.
 */
TEST(Compose, Compose_MarginsLargerThanCanvas_ReturnsOverlayUserError1) {
  // Prepare
  const float plate[3] = {1.0f, 0.0f, 0.0f};
  dailyboy::Frame frame(make_rgb(8, 8, plate));
  dailyboy::Margin margin;
  margin.set_left(8);
  dailyboy::Job job =
      make_job(8, 8, dailyboy::JobLayoutImage::JobLayoutImageFitValue::Contain,
               1.0, margin, rgb(0.0, 0.0, 0.0));
  dailyboy::ColorPipeline color_pipeline = must_prepare(job);

  // Test
  dailyboy::StatusOr<dailyboy::Frame> result =
      compose_plate(std::move(frame), job, color_pipeline);

  // Assert
  ASSERT_FALSE(result.ok());
  EXPECT_EQ(result.status().message(), dailyboy::USER_ERROR_OVERLAY_1);
}

/*!
 * \brief Contain uses the smaller axis scale (square pixels).
 */
TEST(Compose, ScaleToFit_ContainSquarePixels_UsesMinAxis) {
  // Prepare
  const dailyboy::Size plate{4, 8};
  const dailyboy::Rect image_frame{0, 0, 8, 8};

  // Test
  dailyboy::StatusOr<double> scale = dailyboy::scale_to_fit(
      plate, image_frame, 1.0,
      dailyboy::JobLayoutImage::JobLayoutImageFitValue::Contain);

  // Assert
  ASSERT_TRUE(scale.ok()) << scale.status().message();
  EXPECT_DOUBLE_EQ(scale.value(), 1.0);
}
