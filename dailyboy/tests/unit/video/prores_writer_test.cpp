#include "video/prores_writer.hpp"

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <string>

#include "image/frame.hpp"
#include "job/output.hpp"
#include "status.hpp"
#include "support/mov_probe.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/pixfmt.h>
}

namespace {

constexpr int kWidth = 256;
constexpr int kHeight = 120;

using dailyboy::test::moov_before_mdat;
using dailyboy::test::MovInfo;
using dailyboy::test::probe_mov;
using dailyboy::test::unique_mov;

dailyboy::Frame solid_rgb_frame(int width, int height, float r, float g,
                                float b) {
  OIIO::ImageSpec spec(width, height, 3, OIIO::TypeDesc::UINT8);
  OIIO::ImageBuf buf(spec);
  const float rgb[3] = {r, g, b};
  EXPECT_TRUE(OIIO::ImageBufAlgo::fill(buf, rgb));
  return dailyboy::Frame(std::move(buf));
}

dailyboy::JobOutputVideo prores_video(dailyboy::JobOutputVideoProres opt) {
  dailyboy::JobOutputVideo video;
  video.set_codec(dailyboy::JobOutputVideo::JobOutputVideoCodecValue::Prores);
  dailyboy::JobOutputVideoSignal signal;
  signal.set_range(dailyboy::JobOutputVideoSignal::RangeValue::Tv);
  video.set_signal(std::move(signal));
  video.set_codec_options(std::move(opt));
  return video;
}

dailyboy::Status write_three_rgb(const std::filesystem::path& mov,
                                 const dailyboy::JobOutputVideoProres& opt,
                                 int width = kWidth, int height = kHeight) {
  dailyboy::ProresWriter writer;
  dailyboy::Status open =
      writer.open(mov, width, height, dailyboy::VideoWriter::kDefaultFps,
                  prores_video(opt));
  if (!open.ok()) {
    return open;
  }
  DAILYBOY_RETURN_IF_ERROR(
      writer.write(solid_rgb_frame(width, height, 1, 0, 0)));
  DAILYBOY_RETURN_IF_ERROR(
      writer.write(solid_rgb_frame(width, height, 0, 1, 0)));
  DAILYBOY_RETURN_IF_ERROR(
      writer.write(solid_rgb_frame(width, height, 0, 0, 1)));
  return writer.close();
}

}  // namespace

/*!
 * \brief Encodes three RGB frames with default ProRes HQ options as yuv422p10.
 */
TEST(ProresWriter, Open_DefaultOptions_WritesHqYuv422p10Mov) {
  // Prepare
  const std::filesystem::path mov = unique_mov("prores_default");
  dailyboy::JobOutputVideoProres options;

  // Test
  dailyboy::Status status = write_three_rgb(mov, options);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  const MovInfo info = probe_mov(mov);
  EXPECT_EQ(info.codec_id, AV_CODEC_ID_PRORES);
  EXPECT_EQ(info.profile, AV_PROFILE_PRORES_HQ);
  EXPECT_EQ(info.pix_fmt, AV_PIX_FMT_YUV422P10);
  EXPECT_EQ(info.width, kWidth);
  EXPECT_EQ(info.height, kHeight);
  EXPECT_EQ(info.packets, 3);
}

/*!
 * \brief Encodes with profile proxy and stores that profile in the MOV.
 */
TEST(ProresWriter, Open_ProfileProxy_WritesProxyProfile) {
  // Prepare
  const std::filesystem::path mov = unique_mov("prores_profile_proxy");
  dailyboy::JobOutputVideoProres options;
  options.set_profile(
      dailyboy::JobOutputVideoProres::JobOutputVideoProresProfileValue::Proxy);

  // Test
  dailyboy::Status status = write_three_rgb(mov, options);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  EXPECT_EQ(probe_mov(mov).profile, AV_PROFILE_PRORES_PROXY);
}

/*!
 * \brief Encodes with profile lt and stores that profile in the MOV.
 */
TEST(ProresWriter, Open_ProfileLt_WritesLtProfile) {
  // Prepare
  const std::filesystem::path mov = unique_mov("prores_profile_lt");
  dailyboy::JobOutputVideoProres options;
  options.set_profile(
      dailyboy::JobOutputVideoProres::JobOutputVideoProresProfileValue::Lt);

  // Test
  dailyboy::Status status = write_three_rgb(mov, options);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  EXPECT_EQ(probe_mov(mov).profile, AV_PROFILE_PRORES_LT);
}

/*!
 * \brief Encodes with profile standard and stores that profile in the MOV.
 */
TEST(ProresWriter, Open_ProfileStandard_WritesStandardProfile) {
  // Prepare
  const std::filesystem::path mov = unique_mov("prores_profile_standard");
  dailyboy::JobOutputVideoProres options;
  options.set_profile(dailyboy::JobOutputVideoProres::
                          JobOutputVideoProresProfileValue::Standard);

  // Test
  dailyboy::Status status = write_three_rgb(mov, options);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  EXPECT_EQ(probe_mov(mov).profile, AV_PROFILE_PRORES_STANDARD);
}

/*!
 * \brief Encodes with profile 4444 and stores yuv444p10.
 */
TEST(ProresWriter, Open_Profile4444_Writes4444Yuv444p10) {
  // Prepare
  const std::filesystem::path mov = unique_mov("prores_profile_4444");
  dailyboy::JobOutputVideoProres options;
  options.set_profile(dailyboy::JobOutputVideoProres::
                          JobOutputVideoProresProfileValue::FourFourFourFour);
  options.set_pix_fmt(dailyboy::JobOutputVideoProres::
                          JobOutputVideoProresPixFmtValue::Yuv444p10);

  // Test
  dailyboy::Status status = write_three_rgb(mov, options);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  const MovInfo info = probe_mov(mov);
  EXPECT_EQ(info.profile, AV_PROFILE_PRORES_4444);
  // MOV demuxer tags ProRes 4444 as 12-bit even when encoded as 10-bit.
  EXPECT_EQ(info.pix_fmt, AV_PIX_FMT_YUV444P12);
}

/*!
 * \brief Encodes with profile 4444xq and stores yuv444p10.
 */
TEST(ProresWriter, Open_Profile4444xq_WritesXqYuv444p10) {
  // Prepare
  const std::filesystem::path mov = unique_mov("prores_profile_4444xq");
  dailyboy::JobOutputVideoProres options;
  options.set_profile(dailyboy::JobOutputVideoProres::
                          JobOutputVideoProresProfileValue::FourFourFourFourXq);
  options.set_pix_fmt(dailyboy::JobOutputVideoProres::
                          JobOutputVideoProresPixFmtValue::Yuv444p10);

  // Test
  dailyboy::Status status = write_three_rgb(mov, options);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  const MovInfo info = probe_mov(mov);
  EXPECT_EQ(info.profile, AV_PROFILE_PRORES_XQ);
  // MOV demuxer tags ProRes 4444 XQ as 12-bit even when encoded as 10-bit.
  EXPECT_EQ(info.pix_fmt, AV_PIX_FMT_YUV444P12);
}

/*!
 * \brief Encodes 4444 with yuva444p10 and alpha_bits 16.
 */
TEST(ProresWriter, Open_PixFmtYuva444p10_WritesYuva444p10) {
  // Prepare
  const std::filesystem::path mov = unique_mov("prores_pix_fmt_yuva");
  dailyboy::JobOutputVideoProres options;
  options.set_profile(dailyboy::JobOutputVideoProres::
                          JobOutputVideoProresProfileValue::FourFourFourFour);
  options.set_pix_fmt(dailyboy::JobOutputVideoProres::
                          JobOutputVideoProresPixFmtValue::Yuva444p10);
  options.set_alpha_bits(16);

  // Test
  dailyboy::Status status = write_three_rgb(mov, options);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  // MOV demuxer tags ProRes 4444 alpha as 12-bit even when encoded as 10-bit.
  EXPECT_EQ(probe_mov(mov).pix_fmt, AV_PIX_FMT_YUVA444P12);
}

/*!
 * \brief Encodes with vendor Lavc and writes a valid ProRes MOV.
 */
TEST(ProresWriter, Open_VendorLavc_WritesValidMov) {
  // Prepare
  const std::filesystem::path mov = unique_mov("prores_vendor_lavc");
  dailyboy::JobOutputVideoProres options;
  options.set_vendor("Lavc");

  // Test
  dailyboy::Status status = write_three_rgb(mov, options);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  EXPECT_EQ(probe_mov(mov).codec_id, AV_CODEC_ID_PRORES);
}

/*!
 * \brief Encodes with TV signal tags and writes a valid ProRes MOV.
 */
TEST(ProresWriter, Open_SignalTv_WritesValidMov) {
  // Prepare
  const std::filesystem::path mov = unique_mov("prores_signal_tv");
  dailyboy::JobOutputVideoProres options;
  dailyboy::ProresWriter writer;
  dailyboy::JobOutputVideo video = prores_video(options);

  // Test
  dailyboy::Status status = writer.open(
      mov, kWidth, kHeight, dailyboy::VideoWriter::kDefaultFps, video);
  ASSERT_TRUE(status.ok()) << status.message();
  status = writer.write(solid_rgb_frame(kWidth, kHeight, 0.5f, 0.5f, 0.5f));
  ASSERT_TRUE(status.ok()) << status.message();
  status = writer.close();

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  EXPECT_EQ(probe_mov(mov).codec_id, AV_CODEC_ID_PRORES);
  EXPECT_EQ(probe_mov(mov).packets, 1);
}

/*!
 * \brief Encodes with faststart true and places moov before mdat.
 */
TEST(ProresWriter, Open_FaststartTrue_PlacesMoovBeforeMdat) {
  // Prepare
  const std::filesystem::path mov = unique_mov("prores_faststart_true");
  dailyboy::JobOutputVideoProres options;
  options.set_faststart(true);

  // Test
  dailyboy::Status status = write_three_rgb(mov, options);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  EXPECT_TRUE(moov_before_mdat(mov));
}

/*!
 * \brief Encodes with faststart false and leaves moov after mdat.
 */
TEST(ProresWriter, Open_FaststartFalse_LeavesMoovAfterMdat) {
  // Prepare
  const std::filesystem::path mov = unique_mov("prores_faststart_false");
  dailyboy::JobOutputVideoProres options;
  options.set_faststart(false);

  // Test
  dailyboy::Status status = write_three_rgb(mov, options);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  EXPECT_FALSE(moov_before_mdat(mov));
}
