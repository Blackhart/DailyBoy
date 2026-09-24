#include <gtest/gtest.h>

#include "video/ffmpeg.hpp"

extern "C" {
#include <libavutil/pixfmt.h>
}

/*!
 * \brief Maps 8-bit YUV destinations to packed RGB24 encode depth.
 */
TEST(FfmpegHelpers, RgbEncodeDepth_EightBitPixFmt_IsBits8) {
  // Prepare / Test / Assert
  EXPECT_EQ(dailyboy::rgb_encode_depth_for_pix_fmt(AV_PIX_FMT_YUV420P),
            dailyboy::RgbEncodeDepth::Bits8);
  EXPECT_EQ(dailyboy::rgb_encode_depth_for_pix_fmt(AV_PIX_FMT_YUV422P),
            dailyboy::RgbEncodeDepth::Bits8);
  EXPECT_EQ(dailyboy::sws_rgb_pix_fmt(dailyboy::RgbEncodeDepth::Bits8),
            AV_PIX_FMT_RGB24);
  EXPECT_EQ(dailyboy::rgb_bytes_per_pixel(dailyboy::RgbEncodeDepth::Bits8), 3);
}

/*!
 * \brief Maps 10-bit YUV destinations to packed RGB48 encode depth.
 */
TEST(FfmpegHelpers, RgbEncodeDepth_TenBitPixFmt_IsBits16) {
  // Prepare / Test / Assert
  EXPECT_EQ(dailyboy::rgb_encode_depth_for_pix_fmt(AV_PIX_FMT_YUV422P10),
            dailyboy::RgbEncodeDepth::Bits16);
  EXPECT_EQ(dailyboy::rgb_encode_depth_for_pix_fmt(AV_PIX_FMT_YUV444P10),
            dailyboy::RgbEncodeDepth::Bits16);
  EXPECT_EQ(dailyboy::sws_rgb_pix_fmt(dailyboy::RgbEncodeDepth::Bits16),
            AV_PIX_FMT_RGB48);
  EXPECT_EQ(dailyboy::rgb_bytes_per_pixel(dailyboy::RgbEncodeDepth::Bits16), 6);
}
