#include "video/mjpeg_writer.hpp"

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

dailyboy::JobOutputVideo mjpeg_video(
    dailyboy::JobOutputVideoMjpeg opt,
    dailyboy::JobOutputVideoSignal::RangeValue range =
        dailyboy::JobOutputVideoSignal::RangeValue::Pc) {
  dailyboy::JobOutputVideo video;
  video.set_codec(dailyboy::JobOutputVideo::JobOutputVideoCodecValue::Mjpeg);
  video.set_signal(signal_range(range));
  video.set_codec_options(std::move(opt));
  return video;
}

dailyboy::Status write_frames(
    const std::filesystem::path& mov, const dailyboy::JobOutputVideoMjpeg& opt,
    int n, bool checker,
    dailyboy::JobOutputVideoSignal::RangeValue range =
        dailyboy::JobOutputVideoSignal::RangeValue::Pc) {
  dailyboy::MjpegWriter writer;
  dailyboy::Status open =
      writer.open(mov, kPlateWidth, kPlateHeight,
                  dailyboy::VideoWriter::kDefaultFps, mjpeg_video(opt, range));
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
    const std::filesystem::path& mov, const dailyboy::JobOutputVideoMjpeg& opt,
    dailyboy::JobOutputVideoSignal::RangeValue range =
        dailyboy::JobOutputVideoSignal::RangeValue::Pc) {
  dailyboy::MjpegWriter writer;
  dailyboy::Status open =
      writer.open(mov, kPlateWidth, kPlateHeight,
                  dailyboy::VideoWriter::kDefaultFps, mjpeg_video(opt, range));
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
 * \brief Encodes three RGB frames with default MJPEG options as yuvj422p.
 */
TEST(MjpegWriter, Open_DefaultOptions_WritesYuvj422pMov) {
  // Prepare
  const std::filesystem::path mov = unique_mov("mjpeg_422_fixtures");
  dailyboy::JobOutputVideoMjpeg options;

  // Test
  dailyboy::Status status = write_three_rgb(mov, options);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  ASSERT_TRUE(std::filesystem::exists(mov));
  EXPECT_GT(std::filesystem::file_size(mov), 0u);
  const MovInfo info = probe_mov(mov);
  EXPECT_EQ(info.codec_id, AV_CODEC_ID_MJPEG);
  EXPECT_EQ(info.pix_fmt, AV_PIX_FMT_YUVJ422P);
  EXPECT_EQ(info.packets, 3);
}

/*!
 * \brief Encodes with \c pix_fmt yuv420p and stores yuvj420p (full range).
 */
TEST(MjpegWriter, Open_PixFmtYuv420p_WritesYuvj420pMov) {
  // Prepare
  const std::filesystem::path mov = unique_mov("mjpeg_pix_fmt_420");
  dailyboy::JobOutputVideoMjpeg options;
  options.set_pix_fmt(
      dailyboy::JobOutputVideoMjpeg::JobOutputVideoMjpegPixFmtValue::Yuv420p);

  // Test
  dailyboy::Status status = write_three_rgb(mov, options);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  const MovInfo info = probe_mov(mov);
  EXPECT_EQ(info.codec_id, AV_CODEC_ID_MJPEG);
  EXPECT_EQ(info.pix_fmt, AV_PIX_FMT_YUVJ420P);
  EXPECT_EQ(info.packets, 3);
}

/*!
 * \brief Encodes with \c pix_fmt yuv444p and stores yuvj444p (full range).
 */
TEST(MjpegWriter, Open_PixFmtYuv444p_WritesYuvj444pMov) {
  // Prepare
  const std::filesystem::path mov = unique_mov("mjpeg_pix_fmt_444");
  dailyboy::JobOutputVideoMjpeg options;
  options.set_pix_fmt(
      dailyboy::JobOutputVideoMjpeg::JobOutputVideoMjpegPixFmtValue::Yuv444p);

  // Test
  dailyboy::Status status = write_three_rgb(mov, options);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  const MovInfo info = probe_mov(mov);
  EXPECT_EQ(info.codec_id, AV_CODEC_ID_MJPEG);
  EXPECT_EQ(info.pix_fmt, AV_PIX_FMT_YUVJ444P);
  EXPECT_EQ(info.packets, 3);
}

/*!
 * \brief Encodes with \c signal.range pc and stores JPEG range yuvj422p.
 */
TEST(MjpegWriter, Open_SignalRangePc_WritesJpegRangeYuvj422p) {
  // Prepare
  const std::filesystem::path mov = unique_mov("mjpeg_color_range_pc");
  dailyboy::JobOutputVideoMjpeg options;

  // Test
  dailyboy::Status status = write_three_rgb(
      mov, options, dailyboy::JobOutputVideoSignal::RangeValue::Pc);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  const MovInfo info = probe_mov(mov);
  EXPECT_EQ(info.pix_fmt, AV_PIX_FMT_YUVJ422P);
  EXPECT_EQ(info.color_range, AVCOL_RANGE_JPEG);
}

/*!
 * \brief Encodes with \c signal.range tv and stores MPEG range yuv422p.
 */
TEST(MjpegWriter, Open_SignalRangeTv_WritesMpegRangeYuv422p) {
  // Prepare
  const std::filesystem::path mov = unique_mov("mjpeg_color_range_tv");
  dailyboy::JobOutputVideoMjpeg options;

  // Test
  dailyboy::Status status = write_three_rgb(
      mov, options, dailyboy::JobOutputVideoSignal::RangeValue::Tv);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  const MovInfo info = probe_mov(mov);
  EXPECT_EQ(info.pix_fmt, AV_PIX_FMT_YUV422P);
  EXPECT_EQ(info.color_range, AVCOL_RANGE_MPEG);
}

/*!
 * \brief Encodes with faststart and places the \c moov atom before \c mdat.
 */
TEST(MjpegWriter, Open_FaststartTrue_PlacesMoovBeforeMdat) {
  // Prepare
  const std::filesystem::path mov = unique_mov("mjpeg_faststart_true");
  dailyboy::JobOutputVideoMjpeg options;
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
TEST(MjpegWriter, Open_FaststartFalse_PlacesMdatBeforeMoov) {
  // Prepare
  const std::filesystem::path mov = unique_mov("mjpeg_faststart_false");
  dailyboy::JobOutputVideoMjpeg options;
  options.set_faststart(false);

  // Test
  dailyboy::Status status = write_three_rgb(mov, options);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  EXPECT_FALSE(moov_before_mdat(mov));
}

/*!
 * \brief Encodes qscale 2 vs 31 and produces a smaller file at qscale 31.
 */
TEST(MjpegWriter, Open_Qscale2Vs31_WritesSmallerFileAtQscale31) {
  // Prepare
  const std::filesystem::path high = unique_mov("mjpeg_qscale_2");
  const std::filesystem::path low = unique_mov("mjpeg_qscale_31");
  dailyboy::JobOutputVideoMjpeg opt_high;
  opt_high.set_qscale(2);
  dailyboy::JobOutputVideoMjpeg opt_low;
  opt_low.set_qscale(31);
  dailyboy::Status high_status =
      write_frames(high, opt_high, 24, /*checker=*/true);
  dailyboy::Status low_status =
      write_frames(low, opt_low, 24, /*checker=*/true);

  // Test
  // Assert
  ASSERT_TRUE(high_status.ok()) << high_status.message();
  ASSERT_TRUE(low_status.ok()) << low_status.message();
  EXPECT_GT(std::filesystem::file_size(high), std::filesystem::file_size(low));
}

/*!
 * \brief Encodes with optimal Huffman tables and writes a smaller file than
 *        default tables.
 */
TEST(MjpegWriter, Open_HuffmanOptimal_WritesSmallerFileThanDefault) {
  // Prepare
  const std::filesystem::path optimal = unique_mov("mjpeg_huffman_optimal");
  const std::filesystem::path def = unique_mov("mjpeg_huffman_default");
  dailyboy::JobOutputVideoMjpeg opt_opt;
  opt_opt.set_huffman(
      dailyboy::JobOutputVideoMjpeg::JobOutputVideoMjpegHuffmanValue::Optimal);
  dailyboy::JobOutputVideoMjpeg opt_def;
  opt_def.set_huffman(
      dailyboy::JobOutputVideoMjpeg::JobOutputVideoMjpegHuffmanValue::Default);
  dailyboy::Status opt_status =
      write_frames(optimal, opt_opt, 24, /*checker=*/true);
  dailyboy::Status def_status =
      write_frames(def, opt_def, 24, /*checker=*/true);

  // Test
  // Assert
  ASSERT_TRUE(opt_status.ok()) << opt_status.message();
  ASSERT_TRUE(def_status.ok()) << def_status.message();
  EXPECT_LT(std::filesystem::file_size(optimal),
            std::filesystem::file_size(def));
}
