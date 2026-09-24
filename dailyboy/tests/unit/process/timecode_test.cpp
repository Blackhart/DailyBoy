#include "process/timecode.hpp"

#include <gtest/gtest.h>

#include <string>

#include "error/job.hpp"
#include "job/job.hpp"
#include "job/output.hpp"
#include "job/plans.hpp"

namespace {

dailyboy::Job make_job_with_timecode(dailyboy::JobPlanTimecode::Start start,
                                     bool drop_frame, int video_fps) {
  dailyboy::Job job;
  dailyboy::JobPlan plan;
  plan.set_id("shot");
  plan.set_input_colorspace("ACES - ACEScg");
  dailyboy::JobSequence sequence;
  sequence.set_path("plate.%04d.png");
  sequence.set_frame_start(1001);
  sequence.set_frame_end(1010);
  plan.set_sequence(std::move(sequence));
  dailyboy::JobPlanTimecode timecode;
  timecode.set_start(std::move(start));
  timecode.set_drop_frame(drop_frame);
  plan.set_timecode(std::move(timecode));
  dailyboy::JobPlans plans;
  plans.set_plans({std::move(plan)});
  job.set_plans(std::move(plans));

  dailyboy::JobOutputVideo video;
  video.set_id("main");
  video.set_enabled(true);
  video.set_path("out.mov");
  video.set_fps(video_fps);
  video.set_codec(dailyboy::JobOutputVideo::JobOutputVideoCodecValue::H264);
  dailyboy::JobOutputVideos videos;
  videos.set_videos({std::move(video)});
  dailyboy::JobOutput output;
  output.set_videos(std::move(videos));
  job.set_output(std::move(output));
  return job;
}

}  // namespace

/*!
 * \brief Converts 86400 frames at 24 fps ND to one hour SMPTE.
 */
TEST(Timecode, FramesToSmpte_NonDrop24_OneHour) {
  // Prepare
  // Test
  dailyboy::StatusOr<std::string> text =
      dailyboy::frames_to_smpte(86400, 24, false);

  // Assert
  ASSERT_TRUE(text.ok()) << text.status().message();
  EXPECT_EQ(*text, "01:00:00:00");
}

/*!
 * \brief Parses one-hour SMPTE back to 86400 frames at 24 fps.
 */
TEST(Timecode, SmpteToFrames_NonDrop24_OneHour) {
  // Prepare
  // Test
  dailyboy::StatusOr<std::int64_t> frames =
      dailyboy::smpte_to_frames("01:00:00:00", 24, false);

  // Assert
  ASSERT_TRUE(frames.ok()) << frames.status().message();
  EXPECT_EQ(*frames, 86400);
}

/*!
 * \brief Integer start 101 at 24 fps formats as 00:00:04:05 on the hero frame.
 */
TEST(Timecode, FormatOverlay_IntegerStart24_HeroFrame) {
  // Prepare
  dailyboy::Job job = make_job_with_timecode(101, false, 24);

  // Test
  dailyboy::StatusOr<std::string> text =
      dailyboy::format_overlay_timecode(job, 1001);

  // Assert
  ASSERT_TRUE(text.ok()) << text.status().message();
  EXPECT_EQ(*text, "00:00:04:05");
}

/*!
 * \brief Head handle eight frames before hero subtracts from start TC.
 */
TEST(Timecode, FormatOverlay_HeadHandle_OffsetBeforeStart) {
  // Prepare
  dailyboy::Job job =
      make_job_with_timecode(std::string("01:00:00:00"), false, 24);
  job.plans().plans().front().sequence().set_handle_head(8);

  // Test
  dailyboy::StatusOr<std::string> text =
      dailyboy::format_overlay_timecode(job, 993);

  // Assert
  ASSERT_TRUE(text.ok()) << text.status().message();
  EXPECT_EQ(*text, "00:59:59:16");
}

/*!
 * \brief Rejects mismatched enabled video fps when timecode is set.
 */
TEST(Timecode, ResolveFps_MismatchedEnabledVideos_UserError) {
  // Prepare
  dailyboy::Job job =
      make_job_with_timecode(std::string("01:00:00:00"), false, 24);
  dailyboy::JobOutputVideo second;
  second.set_id("other");
  second.set_enabled(true);
  second.set_path("other.mov");
  second.set_fps(25);
  second.set_codec(dailyboy::JobOutputVideo::JobOutputVideoCodecValue::H264);
  job.output().videos().videos().push_back(std::move(second));

  // Test
  dailyboy::StatusOr<int> fps = dailyboy::resolve_timecode_fps(job);

  // Assert
  EXPECT_FALSE(fps.ok());
  EXPECT_EQ(fps.status().message(), std::string(dailyboy::USER_ERROR_JOB_104));
}

/*!
 * \brief Drop-frame true with 24 fps is rejected.
 */
TEST(Timecode, ResolveFps_DropFrameAt24_UserError) {
  // Prepare
  dailyboy::Job job =
      make_job_with_timecode(std::string("01:00:00;00"), true, 24);

  // Test
  dailyboy::StatusOr<int> fps = dailyboy::resolve_timecode_fps(job);

  // Assert
  EXPECT_FALSE(fps.ok());
  EXPECT_EQ(fps.status().message(), std::string(dailyboy::USER_ERROR_JOB_103));
}

/*!
 * \brief MOV tag uses first media frame including slate duration.
 */
TEST(Timecode, FormatMov_WithSlate_OffsetsByDuration) {
  // Prepare
  dailyboy::Job job =
      make_job_with_timecode(std::string("01:00:00:00"), false, 24);
  job.layout().slate().set_duration_frames(2);

  // Test
  dailyboy::StatusOr<std::string> text = dailyboy::format_mov_timecode(job);

  // Assert
  ASSERT_TRUE(text.ok()) << text.status().message();
  EXPECT_EQ(*text, "00:59:59:22");
}
