#include "video/h264_writer.hpp"

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>
#include <gtest/gtest.h>

#include <filesystem>

#include "image/frame.hpp"
#include "job/output.hpp"
#include "status.hpp"
#include "support/mov_probe.hpp"
#include "support/test_fixtures.hpp"

namespace {

using dailyboy::test::file_contains;
using dailyboy::test::kPlateHeight;
using dailyboy::test::kPlateWidth;
using dailyboy::test::moov_before_mdat;
using dailyboy::test::MovInfo;
using dailyboy::test::probe_mov;
using dailyboy::test::unique_mov;

dailyboy::Frame solid_rgb_frame(float r, float g, float b) {
  OIIO::ImageSpec spec(kPlateWidth, kPlateHeight, 3, OIIO::TypeDesc::UINT8);
  OIIO::ImageBuf buf(spec);
  const float rgb[3] = {r, g, b};
  EXPECT_TRUE(OIIO::ImageBufAlgo::fill(buf, rgb));
  return dailyboy::Frame(std::move(buf));
}

dailyboy::Frame checker_frame() {
  OIIO::ImageSpec spec(kPlateWidth, kPlateHeight, 3, OIIO::TypeDesc::UINT8);
  OIIO::ImageBuf buf(spec);
  const float a[3] = {1.0f, 0.0f, 0.0f};
  const float b[3] = {0.0f, 0.0f, 1.0f};
  EXPECT_TRUE(OIIO::ImageBufAlgo::checker(buf, 2, 2, 1, a, b));
  return dailyboy::Frame(std::move(buf));
}

dailyboy::JobOutputVideoSignal signal_range(
    dailyboy::JobOutputVideoSignal::RangeValue range) {
  dailyboy::JobOutputVideoSignal signal;
  signal.set_range(range);
  return signal;
}

dailyboy::JobOutputVideo h264_video(
    dailyboy::JobOutputVideoH264 opt,
    dailyboy::JobOutputVideoSignal::RangeValue range =
        dailyboy::JobOutputVideoSignal::RangeValue::Tv) {
  dailyboy::JobOutputVideo video;
  video.set_codec(dailyboy::JobOutputVideo::JobOutputVideoCodecValue::H264);
  video.set_signal(signal_range(range));
  video.set_codec_options(std::move(opt));
  return video;
}

dailyboy::Status write_frames(
    const std::filesystem::path& mov, const dailyboy::JobOutputVideoH264& opt,
    int n, bool checker,
    dailyboy::JobOutputVideoSignal::RangeValue range =
        dailyboy::JobOutputVideoSignal::RangeValue::Tv) {
  dailyboy::H264Writer writer;
  dailyboy::Status open =
      writer.open(mov, kPlateWidth, kPlateHeight,
                  dailyboy::VideoWriter::kDefaultFps, h264_video(opt, range));
  if (!open.ok()) {
    return open;
  }
  for (int i = 0; i < n; ++i) {
    dailyboy::Frame frame =
        checker ? checker_frame() : solid_rgb_frame(0.5f, 0.5f, 0.5f);
    DAILYBOY_RETURN_IF_ERROR(writer.write(frame));
  }
  return writer.close();
}

dailyboy::Status write_three_rgb(
    const std::filesystem::path& mov, const dailyboy::JobOutputVideoH264& opt,
    dailyboy::JobOutputVideoSignal::RangeValue range =
        dailyboy::JobOutputVideoSignal::RangeValue::Tv) {
  dailyboy::H264Writer writer;
  dailyboy::Status open =
      writer.open(mov, kPlateWidth, kPlateHeight,
                  dailyboy::VideoWriter::kDefaultFps, h264_video(opt, range));
  if (!open.ok()) {
    return open;
  }
  DAILYBOY_RETURN_IF_ERROR(writer.write(solid_rgb_frame(1, 0, 0)));
  DAILYBOY_RETURN_IF_ERROR(writer.write(solid_rgb_frame(0, 1, 0)));
  DAILYBOY_RETURN_IF_ERROR(writer.write(solid_rgb_frame(0, 0, 1)));
  return writer.close();
}

}  // namespace

/*!
 * \brief Encodes three RGB frames with default H.264 options as yuv420p.
 */
TEST(H264Writer, Open_DefaultOptions_WritesYuv420pMov) {
  // Prepare
  const std::filesystem::path mov = unique_mov("h264_420_fixtures");
  dailyboy::JobOutputVideoH264 options;

  // Test
  dailyboy::Status status = write_three_rgb(mov, options);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  ASSERT_TRUE(std::filesystem::exists(mov));
  EXPECT_GT(std::filesystem::file_size(mov), 0u);
  const MovInfo info = probe_mov(mov);
  EXPECT_EQ(info.codec_id, AV_CODEC_ID_H264);
  EXPECT_EQ(info.pix_fmt, AV_PIX_FMT_YUV420P);
  EXPECT_EQ(info.width, kPlateWidth);
  EXPECT_EQ(info.height, kPlateHeight);
  EXPECT_EQ(info.packets, 3);
}

/*!
 * \brief Encodes with \c pix_fmt yuv422p and stores that format in the MOV.
 */
TEST(H264Writer, Open_PixFmtYuv422p_WritesYuv422pMov) {
  // Prepare
  const std::filesystem::path mov = unique_mov("h264_pix_fmt_422");
  dailyboy::JobOutputVideoH264 options;
  options.set_pix_fmt(
      dailyboy::JobOutputVideoH264::JobOutputVideoH264PixFmtValue::Yuv422p);

  // Test
  dailyboy::Status status = write_three_rgb(mov, options);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  const MovInfo info = probe_mov(mov);
  EXPECT_EQ(info.codec_id, AV_CODEC_ID_H264);
  EXPECT_EQ(info.pix_fmt, AV_PIX_FMT_YUV422P);
  EXPECT_EQ(info.packets, 3);
}

/*!
 * \brief Encodes with profile baseline and writes baseline or constrained
 *        baseline in the bitstream.
 */
TEST(H264Writer, Open_ProfileBaseline_WritesBaselineOrConstrainedBaseline) {
  // Prepare
  const std::filesystem::path mov = unique_mov("h264_profile_baseline");
  dailyboy::JobOutputVideoH264 options;
  options.set_profile(
      dailyboy::JobOutputVideoH264::JobOutputVideoH264ProfileValue::Baseline);

  // Test
  dailyboy::Status status = write_three_rgb(mov, options);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  const MovInfo info = probe_mov(mov);
  EXPECT_TRUE(info.profile == AV_PROFILE_H264_BASELINE ||
              info.profile == AV_PROFILE_H264_CONSTRAINED_BASELINE)
      << info.profile;
}

/*!
 * \brief Encodes with profile main and stores Main in the bitstream.
 */
TEST(H264Writer, Open_ProfileMain_WritesMainProfile) {
  // Prepare
  const std::filesystem::path mov = unique_mov("h264_profile_main");
  dailyboy::JobOutputVideoH264 options;
  options.set_profile(
      dailyboy::JobOutputVideoH264::JobOutputVideoH264ProfileValue::Main);

  // Test
  dailyboy::Status status = write_three_rgb(mov, options);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  const MovInfo info = probe_mov(mov);
  EXPECT_EQ(info.profile, AV_PROFILE_H264_MAIN);
}

/*!
 * \brief Encodes with profile high and stores High in the bitstream.
 */
TEST(H264Writer, Open_ProfileHigh_WritesHighProfile) {
  // Prepare
  const std::filesystem::path mov = unique_mov("h264_profile_high");
  dailyboy::JobOutputVideoH264 options;
  options.set_profile(
      dailyboy::JobOutputVideoH264::JobOutputVideoH264ProfileValue::High);

  // Test
  dailyboy::Status status = write_three_rgb(mov, options);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  const MovInfo info = probe_mov(mov);
  EXPECT_EQ(info.profile, AV_PROFILE_H264_HIGH);
}

/*!
 * \brief Encodes high422 with yuv422p and stores both in the bitstream.
 */
TEST(H264Writer, Open_ProfileHigh422_WritesHigh422Yuv422p) {
  // Prepare
  const std::filesystem::path mov = unique_mov("h264_profile_high422");
  dailyboy::JobOutputVideoH264 options;
  options.set_pix_fmt(
      dailyboy::JobOutputVideoH264::JobOutputVideoH264PixFmtValue::Yuv422p);
  options.set_profile(
      dailyboy::JobOutputVideoH264::JobOutputVideoH264ProfileValue::High422);

  // Test
  dailyboy::Status status = write_three_rgb(mov, options);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  const MovInfo info = probe_mov(mov);
  EXPECT_EQ(info.pix_fmt, AV_PIX_FMT_YUV422P);
  EXPECT_EQ(info.profile, AV_PROFILE_H264_HIGH_422);
}

/*!
 * \brief Encodes with level 4.1 and stores level 41 in the bitstream.
 */
TEST(H264Writer, Open_Level41_WritesLevel41) {
  // Prepare
  const std::filesystem::path mov = unique_mov("h264_level_41");
  dailyboy::JobOutputVideoH264 options;
  options.set_level("4.1");

  // Test
  dailyboy::Status status = write_three_rgb(mov, options);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  const MovInfo info = probe_mov(mov);
  EXPECT_EQ(info.level, 41);
}

/*!
 * \brief Encodes with \c signal.range tv and stores MPEG/limited range.
 */
TEST(H264Writer, Open_SignalRangeTv_WritesMpegRange) {
  // Prepare
  const std::filesystem::path mov = unique_mov("h264_color_range_tv");
  dailyboy::JobOutputVideoH264 options;

  // Test
  dailyboy::Status status = write_three_rgb(
      mov, options, dailyboy::JobOutputVideoSignal::RangeValue::Tv);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  const MovInfo info = probe_mov(mov);
  EXPECT_EQ(info.color_range, AVCOL_RANGE_MPEG);
}

/*!
 * \brief Encodes with \c signal.range pc and stores JPEG/full range.
 */
TEST(H264Writer, Open_SignalRangePc_WritesJpegRange) {
  // Prepare
  const std::filesystem::path mov = unique_mov("h264_color_range_pc");
  dailyboy::JobOutputVideoH264 options;

  // Test
  dailyboy::Status status = write_three_rgb(
      mov, options, dailyboy::JobOutputVideoSignal::RangeValue::Pc);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  const MovInfo info = probe_mov(mov);
  EXPECT_EQ(info.color_range, AVCOL_RANGE_JPEG);
}

/*!
 * \brief Encodes with \c gop 1 so every packet is a keyframe (\c keyint=1).
 */
TEST(H264Writer, Open_Gop1_WritesAllKeyframes) {
  // Prepare
  const std::filesystem::path mov = unique_mov("h264_gop_1");
  dailyboy::JobOutputVideoH264 options;
  options.set_gop(1);
  dailyboy::Status status = write_frames(mov, options, 8, /*checker=*/false);

  // Test
  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  const MovInfo info = probe_mov(mov);
  EXPECT_EQ(info.packets, 8);
  EXPECT_EQ(info.key_packets, info.packets);
  EXPECT_TRUE(file_contains(mov, "keyint=1"));
}

/*!
 * \brief Encodes with \c gop 4 so keyframes are spaced (\c keyint=4).
 */
TEST(H264Writer, Open_Gop4_WritesSpacedKeyframes) {
  // Prepare
  const std::filesystem::path mov = unique_mov("h264_gop_4");
  dailyboy::JobOutputVideoH264 options;
  options.set_gop(4);
  dailyboy::Status status = write_frames(mov, options, 12, /*checker=*/false);

  // Test
  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  const MovInfo info = probe_mov(mov);
  EXPECT_EQ(info.packets, 12);
  EXPECT_GE(info.key_packets, 2);
  EXPECT_LT(info.key_packets, info.packets);
  EXPECT_TRUE(file_contains(mov, "keyint=4"));
}

/*!
 * \brief Encodes with faststart and places the \c moov atom before \c mdat.
 */
TEST(H264Writer, Open_FaststartTrue_PlacesMoovBeforeMdat) {
  // Prepare
  const std::filesystem::path mov = unique_mov("h264_faststart_true");
  dailyboy::JobOutputVideoH264 options;
  options.set_faststart(true);

  // Test
  dailyboy::Status status = write_three_rgb(mov, options);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  EXPECT_TRUE(moov_before_mdat(mov));
}

/*!
 * \brief Encodes without faststart and leaves \c mdat before \c moov.
 */
TEST(H264Writer, Open_FaststartFalse_PlacesMdatBeforeMoov) {
  // Prepare
  const std::filesystem::path mov = unique_mov("h264_faststart_false");
  dailyboy::JobOutputVideoH264 options;
  options.set_faststart(false);

  // Test
  dailyboy::Status status = write_three_rgb(mov, options);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  EXPECT_FALSE(moov_before_mdat(mov));
}

/*!
 * \brief Encodes with \c bitrate_kbps 2000 and writes ABR settings in x264 SEI.
 */
TEST(H264Writer, Open_Bitrate2000Kbps_WritesAbrSettings) {
  // Prepare
  const std::filesystem::path mov = unique_mov("h264_bitrate");
  dailyboy::JobOutputVideoH264 options;
  options.set_bitrate_kbps(2000);

  // Test
  dailyboy::Status status = write_three_rgb(mov, options);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  const MovInfo info = probe_mov(mov);
  EXPECT_EQ(info.codec_id, AV_CODEC_ID_H264);
  EXPECT_EQ(info.packets, 3);
  EXPECT_TRUE(file_contains(mov, "rc=abr"));
  EXPECT_TRUE(file_contains(mov, "bitrate=2000"));
}

/*!
 * \brief Encodes CRF 18 vs 51 and produces a smaller file at the higher CRF.
 */
TEST(H264Writer, Open_Crf18Vs51_WritesSmallerFileAtCrf51) {
  // Prepare
  const std::filesystem::path high = unique_mov("h264_crf_18");
  const std::filesystem::path low = unique_mov("h264_crf_51");
  dailyboy::JobOutputVideoH264 opt_high;
  opt_high.set_crf(18);
  dailyboy::JobOutputVideoH264 opt_low;
  opt_low.set_crf(51);
  dailyboy::Status high_status =
      write_frames(high, opt_high, 24, /*checker=*/true);
  dailyboy::Status low_status =
      write_frames(low, opt_low, 24, /*checker=*/true);

  // Test
  // Assert
  ASSERT_TRUE(high_status.ok()) << high_status.message();
  ASSERT_TRUE(low_status.ok()) << low_status.message();
  EXPECT_TRUE(file_contains(high, "crf=18.0"));
  EXPECT_TRUE(file_contains(low, "crf=51.0"));
  EXPECT_GT(std::filesystem::file_size(high), std::filesystem::file_size(low));
}

/*!
 * \brief Encodes with preset ultrafast and writes \c ref=1 in x264 SEI.
 */
TEST(H264Writer, Open_PresetUltrafast_WritesRef1InSei) {
  // Prepare
  const std::filesystem::path mov = unique_mov("h264_preset_ultrafast");
  dailyboy::JobOutputVideoH264 options;
  options.set_preset(
      dailyboy::JobOutputVideoH264::JobOutputVideoH264PresetValue::Ultrafast);

  // Test
  dailyboy::Status status = write_three_rgb(mov, options);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  EXPECT_TRUE(file_contains(mov, "x264"));
  EXPECT_TRUE(file_contains(mov, "ref=1"));
}

/*!
 * \brief Encodes with tune fastdecode and writes \c cabac=0 in x264 SEI.
 */
TEST(H264Writer, Open_TuneFastdecode_WritesCabac0InSei) {
  // Prepare
  const std::filesystem::path mov = unique_mov("h264_tune_fastdecode");
  dailyboy::JobOutputVideoH264 options;
  options.set_tune(
      dailyboy::JobOutputVideoH264::JobOutputVideoH264TuneValue::Fastdecode);

  // Test
  dailyboy::Status status = write_three_rgb(mov, options);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  EXPECT_TRUE(file_contains(mov, "cabac=0"));
}
