#include "image/text.hpp"

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>
#include <gtest/gtest.h>

#include <string>

#include "error/process.hpp"
#include "job/primitives.hpp"
#include "job/text.hpp"
#include "support/test_fonts.hpp"

/*!
 * \brief Pixel position is used as the box origin.
 */
TEST(Text, ComputeBoxPosition_PixelMode_ReturnsOrigin) {
  // Prepare
  dailyboy::TextPosition position;
  position.set_mode(dailyboy::TextPosition::Mode::Pixel);
  dailyboy::TextPositionModePixel pixel;
  pixel.set_x(12);
  pixel.set_y(34);
  position.set_value(pixel);

  // Test
  const dailyboy::Point origin =
      dailyboy::compute_box_position(position, {10, 10}, 100, 100);

  // Assert
  EXPECT_EQ(origin.x, 12);
  EXPECT_EQ(origin.y, 34);
}

/*!
 * \brief Measures a non-empty string with a real font.
 */
TEST(Text, ComputeTextSize_DejaVuFont_ReturnsPositiveSize) {
  // Prepare
  dailyboy::TextFont font;
  font.set_path(dailyboy::test::kDejaVuSans);
  font.set_size_px(16);

  // Test
  dailyboy::StatusOr<dailyboy::Size> size =
      dailyboy::compute_text_size("OK", font);

  // Assert
  ASSERT_TRUE(size.ok()) << size.status().message();
  EXPECT_GT(size.value().width, 0);
  EXPECT_GT(size.value().height, 0);
}

/*!
 * \brief Missing font file returns OVERLAY_3.
 */
TEST(Text, ComputeTextSize_MissingFont_ReturnsOverlayUserError3) {
  // Prepare
  dailyboy::TextFont font;
  font.set_path("/no/such/font.ttf");
  font.set_size_px(16);

  // Test
  dailyboy::StatusOr<dailyboy::Size> size =
      dailyboy::compute_text_size("OK", font);

  // Assert
  ASSERT_FALSE(size.ok());
  EXPECT_NE(
      size.status().message().find(std::string(dailyboy::USER_ERROR_OVERLAY_3)),
      std::string::npos);
}

/*!
 * \brief Draws glyphs onto a black canvas.
 */
TEST(Text, DrawText_WhiteOnBlack_PaintsGlyphs) {
  // Prepare
  OIIO::ImageSpec spec(64, 32, 3, OIIO::TypeDesc::FLOAT);
  OIIO::ImageBuf canvas(spec);
  EXPECT_TRUE(OIIO::ImageBufAlgo::zero(canvas));
  dailyboy::TextFont font;
  font.set_path(dailyboy::test::kDejaVuSans);
  font.set_size_px(16);
  font.set_color(dailyboy::RGBColor(1.0, 1.0, 1.0));

  // Test
  const dailyboy::Status status =
      dailyboy::draw_text(canvas, {2, 2}, "A", font);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  float px[3] = {0.0f, 0.0f, 0.0f};
  bool lit = false;
  for (int y = 0; y < 32 && !lit; ++y) {
    for (int x = 0; x < 64 && !lit; ++x) {
      ASSERT_TRUE(canvas.get_pixels(OIIO::ROI(x, x + 1, y, y + 1),
                                    OIIO::TypeDesc::FLOAT, px));
      if (px[0] > 0.1f) {
        lit = true;
      }
    }
  }
  EXPECT_TRUE(lit);
}
