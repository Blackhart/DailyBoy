#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace dailyboy {

/*!
 * \brief Input file sequence for a plan (ShotGrid / libfileseq path).
 */
class JobSequence {
 public:
  JobSequence() = default;

  const std::string& path() const { return path_; }
  void set_path(std::string path) { path_ = std::move(path); }

  int frame_start() const { return frame_start_; }
  void set_frame_start(int frame_start) { frame_start_ = frame_start; }

  int frame_end() const { return frame_end_; }
  void set_frame_end(int frame_end) { frame_end_ = frame_end; }

  int handle_head() const { return handle_head_; }
  void set_handle_head(int handle_head) { handle_head_ = handle_head; }

  int handle_tail() const { return handle_tail_; }
  void set_handle_tail(int handle_tail) { handle_tail_ = handle_tail; }

  /*!
   * \brief First frame to process: \c frame_start - \c handle_head.
   */
  int effective_frame_start() const { return frame_start_ - handle_head_; }

  /*!
   * \brief Last frame to process: \c frame_end + \c handle_tail.
   */
  int effective_frame_end() const { return frame_end_ + handle_tail_; }

  /*!
   * \brief Inclusive plate count including handles.
   */
  int plate_frame_count() const {
    return effective_frame_end() - effective_frame_start() + 1;
  }

 private:
  std::string path_;
  int frame_start_ = 0;
  int frame_end_ = 0;
  int handle_head_ = 0;
  int handle_tail_ = 0;
};

/*!
 * \brief Optional guide-track audio for a plan (\c plans[].audio).
 */
class JobPlanAudio {
 public:
  JobPlanAudio() = default;

  const std::string& path() const { return path_; }
  void set_path(std::string path) { path_ = std::move(path); }

 private:
  std::string path_;
};

/*!
 * \brief One composited source plan (input colorspace + sequence).
 */
class JobPlan {
 public:
  JobPlan() = default;

  const std::string& id() const { return id_; }
  void set_id(std::string id) { id_ = std::move(id); }

  const std::string& input_colorspace() const { return input_colorspace_; }
  void set_input_colorspace(std::string input_colorspace) {
    input_colorspace_ = std::move(input_colorspace);
  }

  const JobSequence& sequence() const { return sequence_; }
  JobSequence& sequence() { return sequence_; }
  void set_sequence(JobSequence sequence) { sequence_ = std::move(sequence); }

  const std::optional<JobPlanAudio>& audio() const { return audio_; }
  std::optional<JobPlanAudio>& audio() { return audio_; }
  void set_audio(std::optional<JobPlanAudio> audio) {
    audio_ = std::move(audio);
  }

 private:
  std::string id_;
  std::string input_colorspace_;
  JobSequence sequence_;
  std::optional<JobPlanAudio> audio_;
};

/*!
 * \brief Collection of source plans (\c plans in the job YAML).
 */
class JobPlans {
 public:
  JobPlans() = default;
  const std::vector<JobPlan>& plans() const { return plans_; }
  std::vector<JobPlan>& plans() { return plans_; }
  void set_plans(std::vector<JobPlan> plans) { plans_ = std::move(plans); }

 private:
  std::vector<JobPlan> plans_;
};

}  // namespace dailyboy
