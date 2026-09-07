#include "image/sequence.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <map>
#include <string>

#include "error/image.hpp"
#include "job/plans.hpp"
#include "status.hpp"
#include "support/test_fixtures.hpp"

namespace {

std::filesystem::path test_dir() {
  return std::filesystem::path(DAILYBOY_TEST_BINARY_DIR) / "sequence_fixtures";
}

dailyboy::JobSequence plate_job_sequence(
    const std::filesystem::path& dir,
    int start = dailyboy::test::kPlateFrameStart,
    int end = dailyboy::test::kPlateFrameEnd) {
  dailyboy::JobSequence sequence;
  sequence.set_path(dailyboy::test::plate_pattern_path(dir).string());
  sequence.set_frame_start(start);
  sequence.set_frame_end(end);
  return sequence;
}

}  // namespace

/*!
 * \brief Opens a three-frame plate pattern and yields matching frame numbers
 *        and paths.
 */
TEST(Sequence, Open_ValidPlateRange_IteratesFrameNumbersAndPaths) {
  // Prepare
  const std::filesystem::path dir = test_dir() / "iterate";
  std::filesystem::remove_all(dir);
  std::filesystem::create_directories(dir);

  // Test
  dailyboy::StatusOr<dailyboy::Sequence> seq =
      dailyboy::Sequence::open(plate_job_sequence(dir));

  // Assert
  ASSERT_TRUE(seq.ok()) << seq.status().message();
  EXPECT_EQ(seq.value().frame_start(), 1001);
  EXPECT_EQ(seq.value().frame_end(), 1003);
  EXPECT_EQ(seq.value().size(), 3u);
  int expected = 1001;
  for (auto it = seq.value().begin(); it != seq.value().end(); ++it) {
    EXPECT_EQ(it.frame(), expected);
    EXPECT_EQ(it.path(), dailyboy::test::plate_frame_path(dir, expected));
    dailyboy::StatusOr<std::filesystem::path> direct =
        seq.value().path(expected);
    ASSERT_TRUE(direct.ok()) << direct.status().message();
    EXPECT_EQ(direct.value(), it.path());
    ++expected;
  }
  EXPECT_EQ(expected, 1004);
}

/*!
 * \brief Requests a frame outside the open range and returns a user error.
 */
TEST(Sequence, Path_FrameOutOfRange_ReturnsIoUserError4) {
  // Prepare
  dailyboy::JobSequence job;
  job.set_path("/tmp/plate.%04d.png");
  job.set_frame_start(1001);
  job.set_frame_end(1003);

  // Test
  dailyboy::StatusOr<dailyboy::Sequence> seq = dailyboy::Sequence::open(job);

  // Assert
  ASSERT_TRUE(seq.ok()) << seq.status().message();
  dailyboy::StatusOr<std::filesystem::path> path = seq.value().path(0);
  ASSERT_FALSE(path.ok());
  EXPECT_EQ(path.status().code(), dailyboy::Status::Code::kUser);
  EXPECT_EQ(path.status().message(), dailyboy::USER_ERROR_IO_4);
}

/*!
 * \brief Opens with frame_end before frame_start and returns a user error.
 */
TEST(Sequence, Open_InvertedFrameRange_ReturnsIoUserError1) {
  // Prepare
  dailyboy::JobSequence job;
  job.set_path("/tmp/plate.%04d.png");
  job.set_frame_start(1003);
  job.set_frame_end(1001);

  // Test
  dailyboy::StatusOr<dailyboy::Sequence> seq = dailyboy::Sequence::open(job);

  // Assert
  ASSERT_FALSE(seq.ok());
  EXPECT_EQ(seq.status().message(), dailyboy::USER_ERROR_IO_1);
}

/*!
 * \brief Opens a path with string substitutions and expands tokens.
 */
TEST(Sequence, Open_StringSubstitutionTokens_ExpandsPath) {
  // Prepare
  dailyboy::JobSequence job;
  job.set_path("{root}/plate.%04d.png");
  job.set_frame_start(1001);
  job.set_frame_end(1001);
  std::map<std::string, dailyboy::JobMetadataSubstitutionValue> subs;
  subs["root"] = std::string("/tmp/seq");

  // Test
  dailyboy::StatusOr<dailyboy::Sequence> seq =
      dailyboy::Sequence::open(job, subs);

  // Assert
  ASSERT_TRUE(seq.ok()) << seq.status().message();
  dailyboy::StatusOr<std::filesystem::path> path = seq.value().path(1001);
  ASSERT_TRUE(path.ok()) << path.status().message();
  EXPECT_EQ(path.value().filename(), "plate.1001.png");
  EXPECT_EQ(path.value().parent_path(), "/tmp/seq");
}

/*!
 * \brief Opens a path that uses a frame-map substitution and returns a user
 *        error.
 */
TEST(Sequence, Open_FrameMapTokenInPath_ReturnsIoUserError6) {
  // Prepare
  dailyboy::JobSequence job;
  job.set_path("{note}/plate.%04d.png");
  job.set_frame_start(1001);
  job.set_frame_end(1001);
  std::map<std::string, dailyboy::JobMetadataSubstitutionValue> subs;
  subs["note"] = std::map<std::string, std::string>{{"1001", "hi"}};

  // Test
  dailyboy::StatusOr<dailyboy::Sequence> seq =
      dailyboy::Sequence::open(job, subs);

  // Assert
  ASSERT_FALSE(seq.ok());
  EXPECT_EQ(seq.status().message(), dailyboy::USER_ERROR_IO_6);
}

/*!
 * \brief Loads existing PNG frames and reads width, height, and channel count.
 */
TEST(Sequence, Load_ExistingPngFrames_ReadsImageSpec) {
  // Prepare
  const std::filesystem::path dir = test_dir() / "load";
  std::filesystem::remove_all(dir);
  std::string error;
  ASSERT_TRUE(dailyboy::test::write_plate_png_sequence(dir, &error)) << error;

  // Test
  dailyboy::StatusOr<dailyboy::Sequence> seq =
      dailyboy::Sequence::open(plate_job_sequence(dir));

  // Assert
  ASSERT_TRUE(seq.ok()) << seq.status().message();
  int expected = 1001;
  for (auto it = seq.value().begin(); it != seq.value().end(); ++it) {
    dailyboy::StatusOr<dailyboy::Frame> frame = it.load();
    ASSERT_TRUE(frame.ok()) << frame.status().message();
    EXPECT_EQ(frame.value().width(), dailyboy::test::kPlateWidth);
    EXPECT_EQ(frame.value().height(), dailyboy::test::kPlateHeight);
    EXPECT_EQ(frame.value().buf().spec().nchannels, 3);
    EXPECT_EQ(it.frame(), expected);
    ++expected;
  }
  dailyboy::StatusOr<dailyboy::Frame> mid = seq.value().load(1002);
  ASSERT_TRUE(mid.ok()) << mid.status().message();
  EXPECT_EQ(mid.value().width(), 8);
}

/*!
 * \brief Loads a missing frame file and returns a user error.
 */
TEST(Sequence, Load_MissingFrameFile_ReturnsIoUserError5) {
  // Prepare
  const std::filesystem::path dir = test_dir() / "missing";
  std::filesystem::remove_all(dir);
  std::string error;
  ASSERT_TRUE(dailyboy::test::write_plate_png_sequence(dir, &error)) << error;
  std::filesystem::remove(dailyboy::test::plate_frame_path(dir, 1002));

  // Test
  dailyboy::StatusOr<dailyboy::Sequence> seq =
      dailyboy::Sequence::open(plate_job_sequence(dir));

  // Assert
  ASSERT_TRUE(seq.ok()) << seq.status().message();
  dailyboy::StatusOr<dailyboy::Frame> frame = seq.value().load(1002);
  ASSERT_FALSE(frame.ok());
  EXPECT_EQ(frame.status().code(), dailyboy::Status::Code::kUser);
  EXPECT_EQ(frame.status().message().compare(
                0, dailyboy::USER_ERROR_IO_5.size(), dailyboy::USER_ERROR_IO_5),
            0)
      << frame.status().message();
}
