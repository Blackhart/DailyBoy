#include "video/dnxhd_writer.hpp"

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
#include <libavformat/avformat.h>
#include <libavutil/pixfmt.h>
}

namespace {

constexpr int kHrWidth = 256;
constexpr int kHrHeight = 120;
constexpr int kCidWidth = 1920;
constexpr int kCidHeight = 1080;
constexpr int kCidBitrateKbps = 36000;
constexpr int kInterlacedBitrateKbps = 120000;
constexpr int kHd175BitrateKbps = 175000;

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

dailyboy::Frame checker_frame(int width, int height) {
  OIIO::ImageSpec spec(width, height, 3, OIIO::TypeDesc::UINT8);
  OIIO::ImageBuf buf(spec);
  const float a[3] = {1.0f, 0.0f, 0.0f};
  const float b[3] = {0.0f, 0.0f, 1.0f};
  EXPECT_TRUE(OIIO::ImageBufAlgo::checker(buf, 8, 8, 1, a, b));
  return dailyboy::Frame(std::move(buf));
}

dailyboy::JobOutputVideo dnxhd_video(dailyboy::JobOutputVideoDnxhd opt) {
  dailyboy::JobOutputVideo video;
  video.set_codec(dailyboy::JobOutputVideo::JobOutputVideoCodecValue::Dnxhd);
  dailyboy::JobOutputVideoSignal signal;
  signal.set_range(dailyboy::JobOutputVideoSignal::RangeValue::Tv);
  video.set_signal(std::move(signal));
  video.set_codec_options(std::move(opt));
  return video;
}

dailyboy::Status write_frames(const std::filesystem::path& mov,
                              const dailyboy::JobOutputVideoDnxhd& opt,
                              int width, int height, int n, bool checker) {
  dailyboy::DnxhdWriter writer;
  dailyboy::Status open = writer.open(
      mov, width, height, dailyboy::VideoWriter::kDefaultFps, dnxhd_video(opt));
  if (!open.ok()) {
    return open;
  }
  for (int i = 0; i < n; ++i) {
    dailyboy::Frame frame =
        checker ? checker_frame(width, height)
                : solid_rgb_frame(width, height, 0.5f, 0.5f, 0.5f);
    DAILYBOY_RETURN_IF_ERROR(writer.write(frame));
  }
  return writer.close();
}

dailyboy::Status write_three_rgb(const std::filesystem::path& mov,
                                 const dailyboy::JobOutputVideoDnxhd& opt,
                                 int width, int height) {
  dailyboy::DnxhdWriter writer;
  dailyboy::Status open = writer.open(
      mov, width, height, dailyboy::VideoWriter::kDefaultFps, dnxhd_video(opt));
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

dailyboy::Status write_hr(const std::filesystem::path& mov,
                          const dailyboy::JobOutputVideoDnxhd& opt) {
  return write_three_rgb(mov, opt, kHrWidth, kHrHeight);
}

bool dnxhd_bitstream_interlaced(const std::filesystem::path& path) {
  AVFormatContext* format = nullptr;
  if (avformat_open_input(&format, path.c_str(), nullptr, nullptr) < 0) {
    return false;
  }
  if (avformat_find_stream_info(format, nullptr) < 0) {
    avformat_close_input(&format);
    return false;
  }
  const int video =
      av_find_best_stream(format, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
  if (video < 0) {
    avformat_close_input(&format);
    return false;
  }
  AVPacket* pkt = av_packet_alloc();
  bool interlaced = false;
  bool found = false;
  while (av_read_frame(format, pkt) >= 0) {
    if (pkt->stream_index == video && pkt->size > 0x2d) {
      interlaced = (pkt->data[0x2c] & 0x80) == 0;
      found = true;
      av_packet_unref(pkt);
      break;
    }
    av_packet_unref(pkt);
  }
  av_packet_free(&pkt);
  avformat_close_input(&format);
  return found && interlaced;
}

}  // namespace

/*!
 * \brief Encodes three RGB frames with default DNxHR HQ options as yuv422p.
 */
TEST(DnxhdWriter, Open_DefaultOptions_WritesDnxhrHqYuv422pMov) {
  // Prepare
  const std::filesystem::path mov = unique_mov("dnxhd_default");
  dailyboy::JobOutputVideoDnxhd options;

  // Test
  dailyboy::Status status = write_hr(mov, options);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  const MovInfo info = probe_mov(mov);
  EXPECT_EQ(info.codec_id, AV_CODEC_ID_DNXHD);
  EXPECT_EQ(info.profile, AV_PROFILE_DNXHR_HQ);
  EXPECT_EQ(info.pix_fmt, AV_PIX_FMT_YUV422P);
  EXPECT_EQ(info.width, kHrWidth);
  EXPECT_EQ(info.height, kHrHeight);
  EXPECT_EQ(info.packets, 3);
}

/*!
 * \brief Encodes with profile dnxhr_lb and stores that profile in the MOV.
 */
TEST(DnxhdWriter, Open_ProfileDnxhrLb_WritesLbProfile) {
  // Prepare
  const std::filesystem::path mov = unique_mov("dnxhd_profile_lb");
  dailyboy::JobOutputVideoDnxhd options;
  options.set_profile(
      dailyboy::JobOutputVideoDnxhd::JobOutputVideoDnxhdProfileValue::DnxhrLb);

  // Test
  dailyboy::Status status = write_hr(mov, options);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  EXPECT_EQ(probe_mov(mov).profile, AV_PROFILE_DNXHR_LB);
}

/*!
 * \brief Encodes with profile dnxhr_sq and stores that profile in the MOV.
 */
TEST(DnxhdWriter, Open_ProfileDnxhrSq_WritesSqProfile) {
  // Prepare
  const std::filesystem::path mov = unique_mov("dnxhd_profile_sq");
  dailyboy::JobOutputVideoDnxhd options;
  options.set_profile(
      dailyboy::JobOutputVideoDnxhd::JobOutputVideoDnxhdProfileValue::DnxhrSq);

  // Test
  dailyboy::Status status = write_hr(mov, options);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  EXPECT_EQ(probe_mov(mov).profile, AV_PROFILE_DNXHR_SQ);
}

/*!
 * \brief Encodes with an explicit dnxhr_hq profile and stores HQ in the MOV.
 */
TEST(DnxhdWriter, Open_ProfileDnxhrHq_WritesHqProfile) {
  // Prepare
  const std::filesystem::path mov = unique_mov("dnxhd_profile_hq");
  dailyboy::JobOutputVideoDnxhd options;
  options.set_profile(
      dailyboy::JobOutputVideoDnxhd::JobOutputVideoDnxhdProfileValue::DnxhrHq);

  // Test
  dailyboy::Status status = write_hr(mov, options);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  EXPECT_EQ(probe_mov(mov).profile, AV_PROFILE_DNXHR_HQ);
}

/*!
 * \brief Encodes with profile dnxhr_hqx and stores yuv422p10.
 */
TEST(DnxhdWriter, Open_ProfileDnxhrHqx_WritesHqxYuv422p10) {
  // Prepare
  const std::filesystem::path mov = unique_mov("dnxhd_profile_hqx");
  dailyboy::JobOutputVideoDnxhd options;
  options.set_profile(
      dailyboy::JobOutputVideoDnxhd::JobOutputVideoDnxhdProfileValue::DnxhrHqx);
  options.set_pix_fmt(
      dailyboy::JobOutputVideoDnxhd::JobOutputVideoDnxhdPixFmtValue::Yuv422p10);

  // Test
  dailyboy::Status status = write_hr(mov, options);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  const MovInfo info = probe_mov(mov);
  EXPECT_EQ(info.profile, AV_PROFILE_DNXHR_HQX);
  EXPECT_EQ(info.pix_fmt, AV_PIX_FMT_YUV422P10);
}

/*!
 * \brief Encodes with profile dnxhr_444 and stores yuv444p10.
 */
TEST(DnxhdWriter, Open_ProfileDnxhr444_Writes444Yuv444p10) {
  // Prepare
  const std::filesystem::path mov = unique_mov("dnxhd_profile_444");
  dailyboy::JobOutputVideoDnxhd options;
  options.set_profile(
      dailyboy::JobOutputVideoDnxhd::JobOutputVideoDnxhdProfileValue::Dnxhr444);
  options.set_pix_fmt(
      dailyboy::JobOutputVideoDnxhd::JobOutputVideoDnxhdPixFmtValue::Yuv444p10);

  // Test
  dailyboy::Status status = write_hr(mov, options);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  const MovInfo info = probe_mov(mov);
  EXPECT_EQ(info.profile, AV_PROFILE_DNXHR_444);
  EXPECT_EQ(info.pix_fmt, AV_PIX_FMT_YUV444P10);
}

/*!
 * \brief Encodes classic DNxHD CID 1920x1080 at 36 Mbps as yuv422p.
 */
TEST(DnxhdWriter, Open_ProfileDnxhd_WritesClassicCidMov) {
  // Prepare
  const std::filesystem::path mov = unique_mov("dnxhd_profile_cid");
  dailyboy::JobOutputVideoDnxhd options;
  options.set_profile(
      dailyboy::JobOutputVideoDnxhd::JobOutputVideoDnxhdProfileValue::Dnxhd);
  options.set_bitrate_kbps(kCidBitrateKbps);

  // Test
  dailyboy::Status status =
      write_three_rgb(mov, options, kCidWidth, kCidHeight);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  const MovInfo info = probe_mov(mov);
  EXPECT_EQ(info.codec_id, AV_CODEC_ID_DNXHD);
  EXPECT_EQ(info.profile, AV_PROFILE_DNXHD);
  EXPECT_EQ(info.pix_fmt, AV_PIX_FMT_YUV422P);
  EXPECT_EQ(info.width, kCidWidth);
  EXPECT_EQ(info.height, kCidHeight);
  EXPECT_EQ(info.packets, 3);
}

/*!
 * \brief Encodes DNxHR HQ with explicit pix_fmt yuv422p.
 */
TEST(DnxhdWriter, Open_PixFmtYuv422p_WritesYuv422p) {
  // Prepare
  const std::filesystem::path mov = unique_mov("dnxhd_pix_fmt_422");
  dailyboy::JobOutputVideoDnxhd options;
  options.set_pix_fmt(
      dailyboy::JobOutputVideoDnxhd::JobOutputVideoDnxhdPixFmtValue::Yuv422p);

  // Test
  dailyboy::Status status = write_hr(mov, options);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  EXPECT_EQ(probe_mov(mov).pix_fmt, AV_PIX_FMT_YUV422P);
}

/*!
 * \brief Encodes DNxHR HQX with pix_fmt yuv422p10.
 */
TEST(DnxhdWriter, Open_PixFmtYuv422p10_WritesYuv422p10) {
  // Prepare
  const std::filesystem::path mov = unique_mov("dnxhd_pix_fmt_422p10");
  dailyboy::JobOutputVideoDnxhd options;
  options.set_profile(
      dailyboy::JobOutputVideoDnxhd::JobOutputVideoDnxhdProfileValue::DnxhrHqx);
  options.set_pix_fmt(
      dailyboy::JobOutputVideoDnxhd::JobOutputVideoDnxhdPixFmtValue::Yuv422p10);

  // Test
  dailyboy::Status status = write_hr(mov, options);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  EXPECT_EQ(probe_mov(mov).pix_fmt, AV_PIX_FMT_YUV422P10);
}

/*!
 * \brief Encodes DNxHR 444 with pix_fmt yuv444p10.
 */
TEST(DnxhdWriter, Open_PixFmtYuv444p10_WritesYuv444p10) {
  // Prepare
  const std::filesystem::path mov = unique_mov("dnxhd_pix_fmt_444p10");
  dailyboy::JobOutputVideoDnxhd options;
  options.set_profile(
      dailyboy::JobOutputVideoDnxhd::JobOutputVideoDnxhdProfileValue::Dnxhr444);
  options.set_pix_fmt(
      dailyboy::JobOutputVideoDnxhd::JobOutputVideoDnxhdPixFmtValue::Yuv444p10);

  // Test
  dailyboy::Status status = write_hr(mov, options);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  EXPECT_EQ(probe_mov(mov).pix_fmt, AV_PIX_FMT_YUV444P10);
}

/*!
 * \brief Encodes DNxHR progressive and stores a non-interlaced DNxHD header.
 */
TEST(DnxhdWriter, Open_InterlacedFalse_WritesProgressive) {
  // Prepare
  const std::filesystem::path mov = unique_mov("dnxhd_interlaced_false");
  dailyboy::JobOutputVideoDnxhd options;
  options.set_interlaced(false);

  // Test
  dailyboy::Status status = write_hr(mov, options);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  EXPECT_FALSE(dnxhd_bitstream_interlaced(mov));
}

/*!
 * \brief Encodes interlaced DNxHD CID 1920x1080i at 120 Mbps.
 */
TEST(DnxhdWriter, Open_InterlacedTrue_WritesInterlacedCid) {
  // Prepare
  const std::filesystem::path mov = unique_mov("dnxhd_interlaced_true");
  dailyboy::JobOutputVideoDnxhd options;
  options.set_profile(
      dailyboy::JobOutputVideoDnxhd::JobOutputVideoDnxhdProfileValue::Dnxhd);
  options.set_bitrate_kbps(kInterlacedBitrateKbps);
  options.set_interlaced(true);

  // Test
  dailyboy::Status status =
      write_three_rgb(mov, options, kCidWidth, kCidHeight);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  const MovInfo info = probe_mov(mov);
  EXPECT_EQ(info.profile, AV_PROFILE_DNXHD);
  EXPECT_TRUE(dnxhd_bitstream_interlaced(mov));
}

/*!
 * \brief Encodes with nitris_compat false (default) as a baseline file size.
 */
TEST(DnxhdWriter, Open_NitrisCompatFalse_WritesUnpaddedFrames) {
  // Prepare
  const std::filesystem::path mov = unique_mov("dnxhd_nitris_false");
  dailyboy::JobOutputVideoDnxhd options;
  options.set_nitris_compat(false);

  // Test
  dailyboy::Status status =
      write_frames(mov, options, kHrWidth, kHrHeight, 3, true);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  EXPECT_GT(std::filesystem::file_size(mov), 0u);
}

/*!
 * \brief Encodes with nitris_compat true and writes a valid DNxHR HQ MOV.
 */
TEST(DnxhdWriter, Open_NitrisCompatTrue_WritesLargerCodingUnits) {
  // Prepare
  const std::filesystem::path mov = unique_mov("dnxhd_nitris_on");
  dailyboy::JobOutputVideoDnxhd options;
  options.set_nitris_compat(true);

  // Test
  dailyboy::Status status =
      write_frames(mov, options, kHrWidth, kHrHeight, 3, true);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  const MovInfo info = probe_mov(mov);
  EXPECT_EQ(info.codec_id, AV_CODEC_ID_DNXHD);
  EXPECT_EQ(info.profile, AV_PROFILE_DNXHR_HQ);
  EXPECT_EQ(info.packets, 3);
}

/*!
 * \brief Encodes with faststart true and places moov before mdat.
 */
TEST(DnxhdWriter, Open_FaststartTrue_PlacesMoovBeforeMdat) {
  // Prepare
  const std::filesystem::path mov = unique_mov("dnxhd_faststart_true");
  dailyboy::JobOutputVideoDnxhd options;
  options.set_faststart(true);

  // Test
  dailyboy::Status status = write_hr(mov, options);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  EXPECT_TRUE(moov_before_mdat(mov));
}

/*!
 * \brief Encodes with faststart false and places mdat before moov.
 */
TEST(DnxhdWriter, Open_FaststartFalse_PlacesMdatBeforeMoov) {
  // Prepare
  const std::filesystem::path mov = unique_mov("dnxhd_faststart_false");
  dailyboy::JobOutputVideoDnxhd options;
  options.set_faststart(false);

  // Test
  dailyboy::Status status = write_hr(mov, options);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  EXPECT_FALSE(moov_before_mdat(mov));
}

/*!
 * \brief Encodes with \c signal.range tv and tags MPEG/limited range.
 */
TEST(DnxhdWriter, Open_SignalRangeTv_WritesMpegRange) {
  // Prepare
  const std::filesystem::path mov = unique_mov("dnxhd_color_range_tv");
  dailyboy::JobOutputVideoDnxhd options;

  // Test
  dailyboy::Status status = write_hr(mov, options);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  EXPECT_EQ(probe_mov(mov).color_range, AVCOL_RANGE_MPEG);
}

/*!
 * \brief Rejects an 8x8 frame; the Status includes FFmpeg's 256x120 message.
 */
TEST(DnxhdWriter, Open_FrameSmallerThan256x120_ReturnsUserError) {
  // Prepare
  const std::filesystem::path mov = unique_mov("dnxhd_too_small");
  dailyboy::DnxhdWriter writer;
  dailyboy::JobOutputVideoDnxhd options;

  // Test
  dailyboy::Status status = writer.open(
      mov, 8, 8, dailyboy::VideoWriter::kDefaultFps, dnxhd_video(options));

  // Assert
  ASSERT_FALSE(status.ok());
  EXPECT_EQ(status.code(), dailyboy::Status::Code::kUser);
  const std::string& msg = status.message();
  EXPECT_TRUE(msg.find("256") != std::string::npos ||
              msg.find("too small") != std::string::npos)
      << msg;
}

/*!
 * \brief Rejects classic DNxHD when size does not match the CID bitrate.
 */
TEST(DnxhdWriter, Open_ClassicDnxhdMismatchedSize_ReturnsUserError) {
  // Prepare
  const std::filesystem::path mov = unique_mov("dnxhd_cid_mismatch");
  dailyboy::DnxhdWriter writer;
  dailyboy::JobOutputVideoDnxhd options;
  options.set_profile(
      dailyboy::JobOutputVideoDnxhd::JobOutputVideoDnxhdProfileValue::Dnxhd);
  options.set_bitrate_kbps(kHd175BitrateKbps);

  // Test
  dailyboy::Status status =
      writer.open(mov, kHrWidth, kHrHeight, dailyboy::VideoWriter::kDefaultFps,
                  dnxhd_video(options));

  // Assert
  ASSERT_FALSE(status.ok());
  EXPECT_EQ(status.code(), dailyboy::Status::Code::kUser);
  const std::string& msg = status.message();
  EXPECT_TRUE(msg.find("incompatible") != std::string::npos ||
              msg.find("DNxHD") != std::string::npos)
      << msg;
}
