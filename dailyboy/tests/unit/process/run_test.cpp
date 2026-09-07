#include <gtest/gtest.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include <dailyboy/makeDaily.hpp>
#include <filesystem>
#include <fstream>
#include <string>

#include "job/loader.hpp"
#include "process/run.hpp"
#include "status.hpp"
#include "support/test_fixtures.hpp"

namespace {

int count_video_packets(const std::filesystem::path& path,
                        AVCodecID expected_codec) {
  AVFormatContext* format = nullptr;
  if (avformat_open_input(&format, path.c_str(), nullptr, nullptr) < 0) {
    return -1;
  }
  if (avformat_find_stream_info(format, nullptr) < 0) {
    avformat_close_input(&format);
    return -1;
  }
  int video =
      av_find_best_stream(format, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
  if (video < 0) {
    avformat_close_input(&format);
    return -1;
  }
  int count = 0;
  AVPacket* pkt = av_packet_alloc();
  while (av_read_frame(format, pkt) >= 0) {
    if (pkt->stream_index == video) {
      ++count;
    }
    av_packet_unref(pkt);
  }
  av_packet_free(&pkt);
  const AVCodecID codec_id = format->streams[video]->codecpar->codec_id;
  avformat_close_input(&format);
  EXPECT_EQ(codec_id, expected_codec);
  return count;
}

bool write_two_video_job(const std::filesystem::path& yaml_path,
                         const std::filesystem::path& plate_dir,
                         const std::filesystem::path& h264_mov,
                         const std::filesystem::path& mjpeg_mov,
                         bool mjpeg_enabled, std::string* error) {
  const std::string seq =
      dailyboy::test::plate_pattern_path(plate_dir).string();
  std::ofstream out(yaml_path);
  if (!out) {
    if (error != nullptr) {
      *error = "cannot write " + yaml_path.string();
    }
    return false;
  }
  out << "dailyboy_version: 1\n"
      << "color:\n"
      << "  ocio_config: \"/unused/config.ocio\"\n"
      << "layout:\n"
      << "  canvas:\n"
      << "    width: " << dailyboy::test::kPlateWidth << "\n"
      << "    height: " << dailyboy::test::kPlateHeight << "\n"
      << "  image:\n"
      << "    fit: \"contain\"\n"
      << "  slate:\n"
      << "    duration_frames: 0\n"
      << "    lines: []\n"
      << "output:\n"
      << "  videos:\n"
      << "    - id: review_h264\n"
      << "      enabled: true\n"
      << "      display_view:\n"
      << "        display: \"passthrough\"\n"
      << "        view: \"passthrough\"\n"
      << "      signal:\n"
      << "        range: tv\n"
      << "        matrix: bt709\n"
      << "        primaries: bt709\n"
      << "        transfer: bt709\n"
      << "      path: \"" << h264_mov.string() << "\"\n"
      << "      codec: h264\n"
      << "    - id: review_mjpeg\n"
      << "      enabled: " << (mjpeg_enabled ? "true" : "false") << "\n"
      << "      display_view:\n"
      << "        display: \"passthrough\"\n"
      << "        view: \"passthrough\"\n"
      << "      signal:\n"
      << "        range: pc\n"
      << "        matrix: bt709\n"
      << "        primaries: bt709\n"
      << "        transfer: bt709\n"
      << "      path: \"" << mjpeg_mov.string() << "\"\n"
      << "      codec: mjpeg\n"
      << "  image_sequences: []\n"
      << "plans:\n"
      << "  - id: plate\n"
      << "    input_colorspace: ACES - ACEScg\n"
      << "    sequence:\n"
      << "      path: \"" << seq << "\"\n"
      << "      frame_start: " << dailyboy::test::kPlateFrameStart << "\n"
      << "      frame_end: " << dailyboy::test::kPlateFrameEnd << "\n";
  if (!out) {
    if (error != nullptr) {
      *error = "write failed: " + yaml_path.string();
    }
    return false;
  }
  return true;
}

bool write_output_job_yaml(const std::filesystem::path& yaml_path,
                           const std::filesystem::path& plate_dir,
                           const std::string& output_yaml, std::string* error,
                           int slate_duration_frames = 0) {
  const std::string seq =
      dailyboy::test::plate_pattern_path(plate_dir).string();
  std::ofstream out(yaml_path);
  if (!out) {
    if (error != nullptr) {
      *error = "cannot write " + yaml_path.string();
    }
    return false;
  }
  out << "dailyboy_version: 1\n"
      << "color:\n"
      << "  ocio_config: \"/unused/config.ocio\"\n"
      << "layout:\n"
      << "  canvas:\n"
      << "    width: " << dailyboy::test::kPlateWidth << "\n"
      << "    height: " << dailyboy::test::kPlateHeight << "\n"
      << "  image:\n"
      << "    fit: \"contain\"\n"
      << "  slate:\n"
      << "    duration_frames: " << slate_duration_frames << "\n"
      << "    lines: []\n"
      << "output:\n"
      << output_yaml << "plans:\n"
      << "  - id: plate\n"
      << "    input_colorspace: ACES - ACEScg\n"
      << "    sequence:\n"
      << "      path: \"" << seq << "\"\n"
      << "      frame_start: " << dailyboy::test::kPlateFrameStart << "\n"
      << "      frame_end: " << dailyboy::test::kPlateFrameEnd << "\n";
  if (!out) {
    if (error != nullptr) {
      *error = "write failed: " + yaml_path.string();
    }
    return false;
  }
  return true;
}

}  // namespace

/*!
 * \brief Renders a generated H.264 plate job and writes a three-frame MOV.
 */
TEST(RunJob, RunJob_GeneratedH264Plate_WritesThreeFrameMov) {
  // Prepare
  const std::filesystem::path dir =
      std::filesystem::path(DAILYBOY_TEST_BINARY_DIR) / "run_job_fixtures";
  std::filesystem::remove_all(dir);
  std::filesystem::create_directories(dir);
  std::string error;
  ASSERT_TRUE(dailyboy::test::write_plate_png_sequence(dir, &error)) << error;
  const std::filesystem::path yaml = dir / "job.yaml";
  ASSERT_TRUE(dailyboy::test::write_plate_job_yaml(yaml, dir, &error)) << error;

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  ASSERT_TRUE(schema.ok()) << schema.message();
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);
  ASSERT_TRUE(job.ok()) << job.status().message();
  dailyboy::Status status = dailyboy::run_job(job.value());
  ASSERT_TRUE(status.ok()) << status.message();

  // Assert
  const std::filesystem::path mov = dailyboy::test::plate_preview_mov_path(dir);
  ASSERT_TRUE(std::filesystem::exists(mov));
  EXPECT_GT(std::filesystem::file_size(mov), 0u);
  EXPECT_EQ(count_video_packets(mov, AV_CODEC_ID_H264), 3);
}

/*!
 * \brief Runs makeDaily with a nested video path, creates parents, and writes
 *        the MOV there.
 */
TEST(MakeDaily, MakeDaily_NestedVideoPath_WritesMovAndCreatesParents) {
  // Prepare
  const std::filesystem::path dir =
      std::filesystem::path(DAILYBOY_TEST_BINARY_DIR) / "make_daily_fixtures";
  std::filesystem::remove_all(dir);
  std::filesystem::create_directories(dir);
  std::string error;
  ASSERT_TRUE(dailyboy::test::write_plate_png_sequence(dir, &error)) << error;
  const std::filesystem::path nested = dir / "nested" / "out";
  const std::filesystem::path mov = nested / "review.mov";
  const std::filesystem::path yaml = dir / "job.yaml";
  ASSERT_TRUE(dailyboy::test::write_plate_job_yaml(yaml, dir, &error, mov))
      << error;

  // Test
  const int code = dailyboy::makeDaily(yaml.string());

  // Assert
  EXPECT_EQ(code, 0);
  ASSERT_TRUE(std::filesystem::is_directory(nested));
  ASSERT_TRUE(std::filesystem::exists(mov));
  EXPECT_GT(std::filesystem::file_size(mov), 0u);
  EXPECT_EQ(count_video_packets(mov, AV_CODEC_ID_H264), 3);
  EXPECT_FALSE(std::filesystem::exists(dir / "dailyboy.mov"));
}

/*!
 * \brief Renders a generated MJPEG plate job and writes a three-frame MOV.
 */
TEST(RunJob, RunJob_GeneratedMjpegPlate_WritesThreeFrameMov) {
  // Prepare
  const std::filesystem::path dir =
      std::filesystem::path(DAILYBOY_TEST_BINARY_DIR) /
      "run_job_mjpeg_fixtures";
  std::filesystem::remove_all(dir);
  std::filesystem::create_directories(dir);
  std::string error;
  ASSERT_TRUE(dailyboy::test::write_plate_png_sequence(dir, &error)) << error;
  const std::filesystem::path yaml = dir / "job.yaml";
  ASSERT_TRUE(
      dailyboy::test::write_plate_job_yaml(yaml, dir, &error, {}, "mjpeg"))
      << error;

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  ASSERT_TRUE(schema.ok()) << schema.message();
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);
  ASSERT_TRUE(job.ok()) << job.status().message();
  dailyboy::Status status = dailyboy::run_job(job.value());
  ASSERT_TRUE(status.ok()) << status.message();

  // Assert
  const std::filesystem::path mov = dailyboy::test::plate_preview_mov_path(dir);
  ASSERT_TRUE(std::filesystem::exists(mov));
  EXPECT_GT(std::filesystem::file_size(mov), 0u);
  EXPECT_EQ(count_video_packets(mov, AV_CODEC_ID_MJPEG), 3);
}

/*!
 * \brief Renders a generated DNxHR plate job at 256x120 and writes a MOV.
 */
TEST(RunJob, RunJob_GeneratedDnxhdPlate_WritesThreeFrameMov) {
  // Prepare
  const std::filesystem::path dir =
      std::filesystem::path(DAILYBOY_TEST_BINARY_DIR) /
      "run_job_dnxhd_fixtures";
  std::filesystem::remove_all(dir);
  std::filesystem::create_directories(dir);
  std::string error;
  ASSERT_TRUE(dailyboy::test::write_plate_png_sequence(dir, &error, 256, 120))
      << error;
  const std::filesystem::path yaml = dir / "job.yaml";
  ASSERT_TRUE(dailyboy::test::write_plate_job_yaml(yaml, dir, &error, {},
                                                   "dnxhd", 256, 120))
      << error;

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  ASSERT_TRUE(schema.ok()) << schema.message();
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);
  ASSERT_TRUE(job.ok()) << job.status().message();
  dailyboy::Status status = dailyboy::run_job(job.value());
  ASSERT_TRUE(status.ok()) << status.message();

  // Assert
  const std::filesystem::path mov = dailyboy::test::plate_preview_mov_path(dir);
  ASSERT_TRUE(std::filesystem::exists(mov));
  EXPECT_GT(std::filesystem::file_size(mov), 0u);
  EXPECT_EQ(count_video_packets(mov, AV_CODEC_ID_DNXHD), 3);
}

/*!
 * \brief Renders H.264 and MJPEG together and writes both MOVs.
 */
TEST(RunJob, RunJob_TwoEnabledCodecs_WritesBothMovs) {
  // Prepare
  const std::filesystem::path dir =
      std::filesystem::path(DAILYBOY_TEST_BINARY_DIR) /
      "run_job_multi_video_fixtures";
  std::filesystem::remove_all(dir);
  std::filesystem::create_directories(dir);
  std::string error;
  ASSERT_TRUE(dailyboy::test::write_plate_png_sequence(dir, &error)) << error;
  const std::filesystem::path h264_mov = dir / "review_h264.mov";
  const std::filesystem::path mjpeg_mov = dir / "review_mjpeg.mov";
  const std::filesystem::path yaml = dir / "job.yaml";
  ASSERT_TRUE(write_two_video_job(yaml, dir, h264_mov, mjpeg_mov, true, &error))
      << error;

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  ASSERT_TRUE(schema.ok()) << schema.message();
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);
  ASSERT_TRUE(job.ok()) << job.status().message();
  dailyboy::Status status = dailyboy::run_job(job.value());
  ASSERT_TRUE(status.ok()) << status.message();

  // Assert
  ASSERT_TRUE(std::filesystem::exists(h264_mov));
  ASSERT_TRUE(std::filesystem::exists(mjpeg_mov));
  EXPECT_GT(std::filesystem::file_size(h264_mov), 0u);
  EXPECT_GT(std::filesystem::file_size(mjpeg_mov), 0u);
  EXPECT_EQ(count_video_packets(h264_mov, AV_CODEC_ID_H264), 3);
  EXPECT_EQ(count_video_packets(mjpeg_mov, AV_CODEC_ID_MJPEG), 3);
}

/*!
 * \brief Renders two videos with MJPEG disabled and writes only the H.264 MOV.
 */
TEST(RunJob, RunJob_SecondVideoDisabled_SkipsDisabledMov) {
  // Prepare
  const std::filesystem::path dir =
      std::filesystem::path(DAILYBOY_TEST_BINARY_DIR) /
      "run_job_skip_video_fixtures";
  std::filesystem::remove_all(dir);
  std::filesystem::create_directories(dir);
  std::string error;
  ASSERT_TRUE(dailyboy::test::write_plate_png_sequence(dir, &error)) << error;
  const std::filesystem::path h264_mov = dir / "review_h264.mov";
  const std::filesystem::path mjpeg_mov = dir / "review_mjpeg.mov";
  const std::filesystem::path yaml = dir / "job.yaml";
  ASSERT_TRUE(
      write_two_video_job(yaml, dir, h264_mov, mjpeg_mov, false, &error))
      << error;

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  ASSERT_TRUE(schema.ok()) << schema.message();
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);
  ASSERT_TRUE(job.ok()) << job.status().message();
  dailyboy::Status status = dailyboy::run_job(job.value());
  ASSERT_TRUE(status.ok()) << status.message();

  // Assert
  ASSERT_TRUE(std::filesystem::exists(h264_mov));
  EXPECT_FALSE(std::filesystem::exists(mjpeg_mov));
  EXPECT_EQ(count_video_packets(h264_mov, AV_CODEC_ID_H264), 3);
}

/*!
 * \brief Renders H.264 plus a PNG sequence and writes both deliverables.
 */
TEST(RunJob, RunJob_VideoAndImageSequence_WritesMovAndPngs) {
  // Prepare
  const std::filesystem::path dir =
      std::filesystem::path(DAILYBOY_TEST_BINARY_DIR) /
      "run_job_video_seq_fixtures";
  std::filesystem::remove_all(dir);
  std::filesystem::create_directories(dir);
  std::string error;
  ASSERT_TRUE(dailyboy::test::write_plate_png_sequence(dir, &error)) << error;
  const std::filesystem::path mov = dir / "review.mov";
  const std::filesystem::path seq_dir = dir / "out";
  const std::filesystem::path yaml = dir / "job.yaml";
  ASSERT_TRUE(write_output_job_yaml(yaml, dir,
                                    "  videos:\n"
                                    "    - id: review_h264\n"
                                    "      enabled: true\n"
                                    "      display_view:\n"
                                    "        display: \"passthrough\"\n"
                                    "        view: \"passthrough\"\n"
                                    "      signal:\n"
                                    "        range: tv\n"
                                    "        matrix: bt709\n"
                                    "        primaries: bt709\n"
                                    "        transfer: bt709\n"
                                    "      path: \"" +
                                        mov.string() +
                                        "\"\n"
                                        "      codec: h264\n"
                                        "  image_sequences:\n"
                                        "    - id: archive\n"
                                        "      enabled: true\n"
                                        "      display_view:\n"
                                        "        display: \"passthrough\"\n"
                                        "        view: \"passthrough\"\n"
                                        "      path_pattern: \"" +
                                        (seq_dir / "plate.%04d.png").string() +
                                        "\"\n",
                                    &error))
      << error;

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  ASSERT_TRUE(schema.ok()) << schema.message();
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);
  ASSERT_TRUE(job.ok()) << job.status().message();
  dailyboy::Status status = dailyboy::run_job(job.value());
  ASSERT_TRUE(status.ok()) << status.message();

  // Assert
  ASSERT_TRUE(std::filesystem::exists(mov));
  EXPECT_EQ(count_video_packets(mov, AV_CODEC_ID_H264), 3);
  for (int frame = dailyboy::test::kPlateFrameStart;
       frame <= dailyboy::test::kPlateFrameEnd; ++frame) {
    EXPECT_TRUE(std::filesystem::exists(
        dailyboy::test::plate_frame_path(seq_dir, frame)));
  }
}

/*!
 * \brief Skips a disabled image sequence and still writes the H.264 MOV.
 */
TEST(RunJob, RunJob_ImageSequenceDisabled_SkipsPngs) {
  // Prepare
  const std::filesystem::path dir =
      std::filesystem::path(DAILYBOY_TEST_BINARY_DIR) /
      "run_job_skip_seq_fixtures";
  std::filesystem::remove_all(dir);
  std::filesystem::create_directories(dir);
  std::string error;
  ASSERT_TRUE(dailyboy::test::write_plate_png_sequence(dir, &error)) << error;
  const std::filesystem::path mov = dir / "review.mov";
  const std::filesystem::path seq_dir = dir / "out";
  const std::filesystem::path yaml = dir / "job.yaml";
  ASSERT_TRUE(write_output_job_yaml(yaml, dir,
                                    "  videos:\n"
                                    "    - id: review_h264\n"
                                    "      enabled: true\n"
                                    "      display_view:\n"
                                    "        display: \"passthrough\"\n"
                                    "        view: \"passthrough\"\n"
                                    "      signal:\n"
                                    "        range: tv\n"
                                    "        matrix: bt709\n"
                                    "        primaries: bt709\n"
                                    "        transfer: bt709\n"
                                    "      path: \"" +
                                        mov.string() +
                                        "\"\n"
                                        "      codec: h264\n"
                                        "  image_sequences:\n"
                                        "    - id: archive\n"
                                        "      enabled: false\n"
                                        "      display_view:\n"
                                        "        display: \"passthrough\"\n"
                                        "        view: \"passthrough\"\n"
                                        "      path_pattern: \"" +
                                        (seq_dir / "plate.%04d.png").string() +
                                        "\"\n",
                                    &error))
      << error;

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  ASSERT_TRUE(schema.ok()) << schema.message();
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);
  ASSERT_TRUE(job.ok()) << job.status().message();
  dailyboy::Status status = dailyboy::run_job(job.value());
  ASSERT_TRUE(status.ok()) << status.message();

  // Assert
  ASSERT_TRUE(std::filesystem::exists(mov));
  EXPECT_FALSE(std::filesystem::exists(seq_dir / "plate.1001.png"));
}

/*!
 * \brief Renders an image-sequence-only job and writes PNGs without a MOV.
 */
TEST(RunJob, RunJob_SequenceOnly_WritesPngsWithoutMov) {
  // Prepare
  const std::filesystem::path dir =
      std::filesystem::path(DAILYBOY_TEST_BINARY_DIR) /
      "run_job_seq_only_fixtures";
  std::filesystem::remove_all(dir);
  std::filesystem::create_directories(dir);
  std::string error;
  ASSERT_TRUE(dailyboy::test::write_plate_png_sequence(dir, &error)) << error;
  const std::filesystem::path seq_dir = dir / "out";
  const std::filesystem::path yaml = dir / "job.yaml";
  ASSERT_TRUE(write_output_job_yaml(yaml, dir,
                                    "  videos: []\n"
                                    "  image_sequences:\n"
                                    "    - id: archive\n"
                                    "      enabled: true\n"
                                    "      display_view:\n"
                                    "        display: \"passthrough\"\n"
                                    "        view: \"passthrough\"\n"
                                    "      path_pattern: \"" +
                                        (seq_dir / "plate.%04d.png").string() +
                                        "\"\n",
                                    &error))
      << error;

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  ASSERT_TRUE(schema.ok()) << schema.message();
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);
  ASSERT_TRUE(job.ok()) << job.status().message();
  dailyboy::Status status = dailyboy::run_job(job.value());
  ASSERT_TRUE(status.ok()) << status.message();

  // Assert
  EXPECT_FALSE(std::filesystem::exists(dir / "preview.mov"));
  for (int frame = dailyboy::test::kPlateFrameStart;
       frame <= dailyboy::test::kPlateFrameEnd; ++frame) {
    EXPECT_TRUE(std::filesystem::exists(
        dailyboy::test::plate_frame_path(seq_dir, frame)));
  }
}

/*!
 * \brief Prefixes movies and image sequences with slate frames before
 *        \c frame_start.
 */
TEST(RunJob, RunJob_PresentSlate_PrefixesMovieAndSequence) {
  // Prepare
  const std::filesystem::path dir =
      std::filesystem::path(DAILYBOY_TEST_BINARY_DIR) /
      "run_job_slate_movie_fixtures";
  std::filesystem::remove_all(dir);
  std::filesystem::create_directories(dir);
  std::string error;
  ASSERT_TRUE(dailyboy::test::write_plate_png_sequence(dir, &error)) << error;
  const std::filesystem::path mov = dir / "review.mov";
  const std::filesystem::path seq_dir = dir / "out";
  const std::filesystem::path yaml = dir / "job.yaml";
  ASSERT_TRUE(write_output_job_yaml(yaml, dir,
                                    "  videos:\n"
                                    "    - id: review_h264\n"
                                    "      enabled: true\n"
                                    "      display_view:\n"
                                    "        display: \"passthrough\"\n"
                                    "        view: \"passthrough\"\n"
                                    "      signal:\n"
                                    "        range: tv\n"
                                    "        matrix: bt709\n"
                                    "        primaries: bt709\n"
                                    "        transfer: bt709\n"
                                    "      path: \"" +
                                        mov.string() +
                                        "\"\n"
                                        "      codec: h264\n"
                                        "  image_sequences:\n"
                                        "    - id: archive\n"
                                        "      enabled: true\n"
                                        "      display_view:\n"
                                        "        display: \"passthrough\"\n"
                                        "        view: \"passthrough\"\n"
                                        "      path_pattern: \"" +
                                        (seq_dir / "plate.%04d.png").string() +
                                        "\"\n",
                                    &error, 2))
      << error;

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  ASSERT_TRUE(schema.ok()) << schema.message();
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);
  ASSERT_TRUE(job.ok()) << job.status().message();
  dailyboy::Status status = dailyboy::run_job(job.value());
  ASSERT_TRUE(status.ok()) << status.message();

  // Assert
  ASSERT_TRUE(std::filesystem::exists(mov));
  EXPECT_EQ(count_video_packets(mov, AV_CODEC_ID_H264), 5);
  EXPECT_TRUE(
      std::filesystem::exists(dailyboy::test::plate_frame_path(seq_dir, 999)));
  EXPECT_TRUE(
      std::filesystem::exists(dailyboy::test::plate_frame_path(seq_dir, 1000)));
  for (int frame = dailyboy::test::kPlateFrameStart;
       frame <= dailyboy::test::kPlateFrameEnd; ++frame) {
    EXPECT_TRUE(std::filesystem::exists(
        dailyboy::test::plate_frame_path(seq_dir, frame)));
  }
  EXPECT_FALSE(
      std::filesystem::exists(dailyboy::test::plate_frame_path(seq_dir, 998)));
  EXPECT_FALSE(
      std::filesystem::exists(dailyboy::test::plate_frame_path(seq_dir, 1004)));
}

/*!
 * \brief Prefixes a sequence-only job with slate files before \c frame_start.
 */
TEST(RunJob, RunJob_SequenceOnlyWithSlate_WritesSlateThenPlates) {
  // Prepare
  const std::filesystem::path dir =
      std::filesystem::path(DAILYBOY_TEST_BINARY_DIR) /
      "run_job_slate_seq_only_fixtures";
  std::filesystem::remove_all(dir);
  std::filesystem::create_directories(dir);
  std::string error;
  ASSERT_TRUE(dailyboy::test::write_plate_png_sequence(dir, &error)) << error;
  const std::filesystem::path seq_dir = dir / "out";
  const std::filesystem::path yaml = dir / "job.yaml";
  ASSERT_TRUE(write_output_job_yaml(yaml, dir,
                                    "  videos: []\n"
                                    "  image_sequences:\n"
                                    "    - id: archive\n"
                                    "      enabled: true\n"
                                    "      display_view:\n"
                                    "        display: \"passthrough\"\n"
                                    "        view: \"passthrough\"\n"
                                    "      path_pattern: \"" +
                                        (seq_dir / "plate.%04d.png").string() +
                                        "\"\n",
                                    &error, 2))
      << error;

  // Test
  dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  ASSERT_TRUE(schema.ok()) << schema.message();
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);
  ASSERT_TRUE(job.ok()) << job.status().message();
  dailyboy::Status status = dailyboy::run_job(job.value());
  ASSERT_TRUE(status.ok()) << status.message();

  // Assert
  EXPECT_FALSE(std::filesystem::exists(dir / "preview.mov"));
  EXPECT_TRUE(
      std::filesystem::exists(dailyboy::test::plate_frame_path(seq_dir, 999)));
  EXPECT_TRUE(
      std::filesystem::exists(dailyboy::test::plate_frame_path(seq_dir, 1000)));
  for (int frame = dailyboy::test::kPlateFrameStart;
       frame <= dailyboy::test::kPlateFrameEnd; ++frame) {
    EXPECT_TRUE(std::filesystem::exists(
        dailyboy::test::plate_frame_path(seq_dir, frame)));
  }
}
