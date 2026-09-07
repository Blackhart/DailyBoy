#include "video/audio_timeline.hpp"

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>
#include <gtest/gtest.h>
#include <yaml-cpp/yaml.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

#include "image/frame.hpp"
#include "job/job.hpp"
#include "job/layout.hpp"
#include "job/output.hpp"
#include "job/parse_plans.hpp"
#include "job/plans.hpp"
#include "status.hpp"
#include "support/mov_probe.hpp"
#include "support/test_fixtures.hpp"
#include "video/h264_writer.hpp"

namespace {

using dailyboy::test::kPlateHeight;
using dailyboy::test::kPlateWidth;
using dailyboy::test::MovInfo;
using dailyboy::test::probe_mov;
using dailyboy::test::unique_mov;

void write_le16(std::ofstream& out, uint16_t value) {
  const char bytes[2] = {static_cast<char>(value & 0xff),
                         static_cast<char>((value >> 8) & 0xff)};
  out.write(bytes, 2);
}

void write_le32(std::ofstream& out, uint32_t value) {
  const char bytes[4] = {static_cast<char>(value & 0xff),
                         static_cast<char>((value >> 8) & 0xff),
                         static_cast<char>((value >> 16) & 0xff),
                         static_cast<char>((value >> 24) & 0xff)};
  out.write(bytes, 4);
}

std::filesystem::path write_stereo_wav(const std::filesystem::path& path,
                                       int sample_count, int16_t tone) {
  std::ofstream out(path, std::ios::binary);
  const int channels = 2;
  const int rate = 48000;
  const int data_bytes = sample_count * channels * 2;
  out.write("RIFF", 4);
  write_le32(out, 36 + data_bytes);
  out.write("WAVEfmt ", 8);
  write_le32(out, 16);
  write_le16(out, 1);
  write_le16(out, static_cast<uint16_t>(channels));
  write_le32(out, rate);
  write_le32(out, rate * channels * 2);
  write_le16(out, static_cast<uint16_t>(channels * 2));
  write_le16(out, 16);
  out.write("data", 4);
  write_le32(out, data_bytes);
  for (int i = 0; i < sample_count; ++i) {
    write_le16(out, static_cast<uint16_t>(tone));
    write_le16(out, static_cast<uint16_t>(tone));
  }
  return path;
}

dailyboy::JobPlan make_plan(const std::string& id, int start, int end,
                            const std::optional<std::string>& audio_path) {
  dailyboy::JobPlan plan;
  plan.set_id(id);
  plan.set_input_colorspace("ACEScg");
  dailyboy::JobSequence sequence;
  sequence.set_path("/tmp/unused.%04d.png");
  sequence.set_frame_start(start);
  sequence.set_frame_end(end);
  plan.set_sequence(std::move(sequence));
  if (audio_path.has_value()) {
    dailyboy::JobPlanAudio audio;
    audio.set_path(*audio_path);
    plan.set_audio(std::optional<dailyboy::JobPlanAudio>{std::move(audio)});
  }
  return plan;
}

dailyboy::Job make_job(std::vector<dailyboy::JobPlan> plans, int slate_frames) {
  dailyboy::Job job;
  dailyboy::JobPlans job_plans;
  job_plans.set_plans(std::move(plans));
  job.set_plans(std::move(job_plans));
  dailyboy::JobLayoutSlate slate;
  slate.set_duration_frames(slate_frames);
  dailyboy::JobLayout layout;
  layout.set_slate(std::move(slate));
  job.set_layout(std::move(layout));
  return job;
}

dailyboy::Frame solid_frame() {
  OIIO::ImageSpec spec(kPlateWidth, kPlateHeight, 3, OIIO::TypeDesc::UINT8);
  OIIO::ImageBuf buf(spec);
  const float rgb[3] = {0.4f, 0.5f, 0.6f};
  EXPECT_TRUE(OIIO::ImageBufAlgo::fill(buf, rgb));
  return dailyboy::Frame(std::move(buf));
}

dailyboy::JobOutputVideo h264_video() {
  dailyboy::JobOutputVideo video;
  video.set_codec(dailyboy::JobOutputVideo::JobOutputVideoCodecValue::H264);
  video.set_fps(24);
  dailyboy::JobOutputVideoH264 opt;
  video.set_codec_options(std::move(opt));
  return video;
}

bool head_is_near_silence(
    const std::shared_ptr<const dailyboy::AudioPcmTimeline>& timeline,
    int sample_count) {
  const auto& pcm = timeline->samples();
  const std::size_t n = static_cast<std::size_t>(sample_count) *
                        dailyboy::AudioPcmTimeline::kChannels;
  if (pcm.size() < n) {
    return false;
  }
  for (std::size_t i = 0; i < n; ++i) {
    if (std::abs(pcm[i]) > 32) {
      return false;
    }
  }
  return true;
}

}  // namespace

/*!
 * \brief Builds silence for the slate, then pads short plan audio to plates.
 */
TEST(AudioPcmTimeline, Build_ShortWavWithSlate_PadsAndLeadsWithSilence) {
  // Prepare
  const std::filesystem::path dir =
      std::filesystem::path(DAILYBOY_TEST_BINARY_DIR) / "audio_pad";
  std::filesystem::remove_all(dir);
  std::filesystem::create_directories(dir);
  const auto wav = write_stereo_wav(dir / "short.wav", 100, 8000);
  const int fps = 24;
  const int slate = 2;
  const int plates = 4;
  dailyboy::Job job =
      make_job({make_plan("a", 1001, 1001 + plates - 1, wav.string())}, slate);

  // Test
  dailyboy::StatusOr<std::shared_ptr<const dailyboy::AudioPcmTimeline>> built =
      dailyboy::build_job_audio_timeline(job, fps);

  // Assert
  ASSERT_TRUE(built.ok()) << built.status().message();
  ASSERT_TRUE(built.value());
  const int want =
      (slate + plates) * dailyboy::AudioPcmTimeline::kSampleRate / fps;
  EXPECT_EQ(built.value()->sample_count(), want);
  EXPECT_TRUE(head_is_near_silence(
      built.value(), slate * dailyboy::AudioPcmTimeline::kSampleRate / fps));
}

/*!
 * \brief Trims guide audio longer than the plate duration.
 */
TEST(AudioPcmTimeline, Build_LongWav_TrimsToPlateDuration) {
  // Prepare
  const std::filesystem::path dir =
      std::filesystem::path(DAILYBOY_TEST_BINARY_DIR) / "audio_trim";
  std::filesystem::remove_all(dir);
  std::filesystem::create_directories(dir);
  const int fps = 24;
  const int plates = 2;
  const int want_plate = plates * dailyboy::AudioPcmTimeline::kSampleRate / fps;
  const auto wav = write_stereo_wav(dir / "long.wav", want_plate * 3, 9000);
  dailyboy::Job job =
      make_job({make_plan("a", 1001, 1001 + plates - 1, wav.string())}, 0);

  // Test
  dailyboy::StatusOr<std::shared_ptr<const dailyboy::AudioPcmTimeline>> built =
      dailyboy::build_job_audio_timeline(job, fps);

  // Assert
  ASSERT_TRUE(built.ok()) << built.status().message();
  ASSERT_TRUE(built.value());
  EXPECT_EQ(built.value()->sample_count(), want_plate);
}

/*!
 * \brief Omits an audio stream when no plan declares audio.
 */
TEST(H264Writer, Open_NoPlanAudio_WritesVideoOnlyMov) {
  // Prepare
  const std::filesystem::path mov = unique_mov("h264_no_audio");
  dailyboy::H264Writer writer;

  // Test
  ASSERT_TRUE(
      writer.open(mov, kPlateWidth, kPlateHeight, 24, h264_video(), nullptr)
          .ok());
  ASSERT_TRUE(writer.write(solid_frame()).ok());
  ASSERT_TRUE(writer.close().ok());

  // Assert
  const MovInfo info = probe_mov(mov);
  EXPECT_EQ(info.codec_id, AV_CODEC_ID_H264);
  EXPECT_FALSE(info.has_audio);
}

/*!
 * \brief Muxes AAC after slate silence when a plan supplies a WAV guide.
 */
TEST(H264Writer, Open_WithSlate_StartsWithSilenceThenAac) {
  // Prepare
  const std::filesystem::path dir =
      std::filesystem::path(DAILYBOY_TEST_BINARY_DIR) / "h264_audio_slate";
  std::filesystem::remove_all(dir);
  std::filesystem::create_directories(dir);
  const int fps = 24;
  const int slate = 3;
  const int plates = 6;
  const auto wav = write_stereo_wav(
      dir / "guide.wav", plates * dailyboy::AudioPcmTimeline::kSampleRate / fps,
      12000);
  dailyboy::Job job =
      make_job({make_plan("a", 1001, 1001 + plates - 1, wav.string())}, slate);
  dailyboy::StatusOr<std::shared_ptr<const dailyboy::AudioPcmTimeline>> built =
      dailyboy::build_job_audio_timeline(job, fps);
  ASSERT_TRUE(built.ok()) << built.status().message();
  const std::filesystem::path mov = dir / "out.mov";
  dailyboy::H264Writer writer;

  // Test
  ASSERT_TRUE(writer
                  .open(mov, kPlateWidth, kPlateHeight, fps, h264_video(),
                        built.value())
                  .ok());
  for (int i = 0; i < slate + plates; ++i) {
    const dailyboy::Status write = writer.write(solid_frame());
    ASSERT_TRUE(write.ok()) << i << " " << write.message();
  }
  ASSERT_TRUE(writer.close().ok());

  // Assert
  const MovInfo info = probe_mov(mov);
  EXPECT_EQ(info.packets, slate + plates);
  EXPECT_TRUE(info.has_audio);
  EXPECT_EQ(info.audio_codec_id, AV_CODEC_ID_AAC);
  EXPECT_EQ(info.audio_sample_rate, 48000);
  EXPECT_EQ(info.audio_channels, 2);
  EXPECT_TRUE(head_is_near_silence(
      built.value(), slate * dailyboy::AudioPcmTimeline::kSampleRate / fps));
}

/*!
 * \brief Parses optional plans[].audio.path.
 */
TEST(Parse, ParsePlans_WithAudioPath_SetsAudio) {
  // Prepare
  const YAML::Node node = YAML::Load(R"(
- id: plate
  input_colorspace: ACES - ACEScg
  sequence:
    path: "/tmp/plate.%04d.exr"
    frame_start: 1001
    frame_end: 1003
  audio:
    path: "/tmp/guide.wav"
)");

  // Test
  dailyboy::StatusOr<dailyboy::JobPlans> plans = dailyboy::parse_plans(node);

  // Assert
  ASSERT_TRUE(plans.ok()) << plans.status().message();
  ASSERT_TRUE(plans.value().plans()[0].audio().has_value());
  EXPECT_EQ(plans.value().plans()[0].audio()->path(), "/tmp/guide.wav");
}
