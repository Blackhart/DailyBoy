#include "image/buffer.hpp"

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>
#include <gtest/gtest.h>

#include <array>

#include "image/frame.hpp"
#include "job/primitives.hpp"

namespace {

dailyboy::Frame make_rgba(int width, int height) {
  OIIO::ImageSpec spec(width, height, 4, OIIO::TypeDesc::FLOAT);
  spec.channelnames = {"R", "G", "B", "A"};
  OIIO::ImageBuf buf(spec);
  const float rgba[4] = {0.2f, 0.4f, 0.6f, 0.8f};
  EXPECT_TRUE(OIIO::ImageBufAlgo::fill(buf, OIIO::cspan<float>(rgba, 4)));
  return dailyboy::Frame(std::move(buf));
}

std::array<float, 3> pixel_rgb(const OIIO::ImageBuf& buf, int x, int y) {
  float px[3] = {0.0f, 0.0f, 0.0f};
  EXPECT_TRUE(
      buf.get_pixels(OIIO::ROI(x, x + 1, y, y + 1), OIIO::TypeDesc::FLOAT, px));
  return {px[0], px[1], px[2]};
}

}  // namespace

/*!
 * \brief Fills a 4x2 canvas with the given RGB.
 */
TEST(Buffer, FillBackground_Rgb_PaintsAllPixels) {
  // Prepare
  OIIO::ImageSpec spec(4, 2, 3, OIIO::TypeDesc::FLOAT);
  OIIO::ImageBuf canvas(spec);
  dailyboy::RGBColor bg(0.1, 0.2, 0.3);

  // Test
  const dailyboy::Status status = dailyboy::fill_background(canvas, bg);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  const std::array<float, 3> px = pixel_rgb(canvas, 0, 0);
  EXPECT_NEAR(px[0], 0.1f, 1e-5);
  EXPECT_NEAR(px[1], 0.2f, 1e-5);
  EXPECT_NEAR(px[2], 0.3f, 1e-5);
}

/*!
 * \brief Drops the alpha channel of an RGBA buffer.
 */
TEST(Buffer, DropExtraChannels_Rgba_KeepsRgb) {
  // Prepare
  dailyboy::Frame frame = make_rgba(2, 2);

  // Test
  const dailyboy::Status status = dailyboy::drop_extra_channels(frame);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  EXPECT_EQ(frame.buf().spec().nchannels, 3);
  const std::array<float, 3> px = pixel_rgb(frame.buf(), 0, 0);
  EXPECT_NEAR(px[0], 0.2f, 1e-5);
  EXPECT_NEAR(px[1], 0.4f, 1e-5);
  EXPECT_NEAR(px[2], 0.6f, 1e-5);
}

/*!
 * \brief Pastes a cropped data window onto the display window at origin 0.
 */
TEST(Buffer, PasteToDisplayWindow_OffsetData_RestoresFullSize) {
  // Prepare
  OIIO::ImageSpec spec(2, 2, 3, OIIO::TypeDesc::FLOAT);
  spec.x = 2;
  spec.y = 0;
  spec.full_x = 0;
  spec.full_y = 0;
  spec.full_width = 4;
  spec.full_height = 2;
  OIIO::ImageBuf buf(spec);
  const float rgb[3] = {1.0f, 0.0f, 0.0f};
  EXPECT_TRUE(OIIO::ImageBufAlgo::fill(buf, OIIO::cspan<float>(rgb, 3)));
  dailyboy::Frame frame(std::move(buf));

  // Test
  const dailyboy::Status status = dailyboy::paste_to_display_window(frame);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  EXPECT_EQ(frame.width(), 4);
  EXPECT_EQ(frame.height(), 2);
  EXPECT_EQ(frame.buf().spec().x, 0);
  const std::array<float, 3> red = pixel_rgb(frame.buf(), 2, 0);
  EXPECT_NEAR(red[0], 1.0f, 1e-5);
  const std::array<float, 3> black = pixel_rgb(frame.buf(), 0, 0);
  EXPECT_NEAR(black[0], 0.0f, 1e-5);
}

/*!
 * \brief Pastes a plate into a destination rect, clipped to that rect.
 */
TEST(Buffer, PasteIntoRect_PlateAtOrigin_CopiesPixels) {
  // Prepare
  OIIO::ImageSpec canvas_spec(4, 4, 3, OIIO::TypeDesc::FLOAT);
  OIIO::ImageBuf canvas(canvas_spec);
  EXPECT_TRUE(OIIO::ImageBufAlgo::zero(canvas));
  OIIO::ImageSpec plate_spec(2, 2, 3, OIIO::TypeDesc::FLOAT);
  OIIO::ImageBuf plate(plate_spec);
  const float rgb[3] = {0.0f, 1.0f, 0.0f};
  EXPECT_TRUE(OIIO::ImageBufAlgo::fill(plate, OIIO::cspan<float>(rgb, 3)));

  // Test
  const dailyboy::Status status =
      dailyboy::paste_into_rect(canvas, plate, {1, 1}, {0, 0, 4, 4});

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  const std::array<float, 3> px = pixel_rgb(canvas, 1, 1);
  EXPECT_NEAR(px[1], 1.0f, 1e-5);
}

/*!
 * \brief Resamples a 2x2 plate to 4x4.
 */
TEST(Buffer, ResizeBuffer_DoubleSize_ProducesFittedSpec) {
  // Prepare
  OIIO::ImageSpec src_spec(2, 2, 3, OIIO::TypeDesc::FLOAT);
  OIIO::ImageBuf src(src_spec);
  const float rgb[3] = {1.0f, 1.0f, 1.0f};
  EXPECT_TRUE(OIIO::ImageBufAlgo::fill(src, OIIO::cspan<float>(rgb, 3)));
  OIIO::ImageBuf scaled;

  // Test
  const dailyboy::Status status =
      dailyboy::resize_buffer(scaled, src, {4, 4}, "triangle");

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  EXPECT_EQ(scaled.spec().width, 4);
  EXPECT_EQ(scaled.spec().height, 4);
}
