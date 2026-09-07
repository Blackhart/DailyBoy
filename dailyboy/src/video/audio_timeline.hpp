#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "job/job.hpp"
#include "status.hpp"

namespace dailyboy {

/*!
 * \brief Interleaved stereo PCM guide track at 48 kHz (s16).
 *
 * Built once per job: slate silence, then each plan's audio trimmed or padded
 * to that plan's plate duration.
 */
class AudioPcmTimeline {
 public:
  static constexpr int kSampleRate = 48000;
  static constexpr int kChannels = 2;

  AudioPcmTimeline() = default;

  const std::vector<int16_t>& samples() const { return samples_; }
  std::vector<int16_t>& samples() { return samples_; }

  bool empty() const { return samples_.empty(); }
  int sample_count() const {
    return static_cast<int>(samples_.size() / kChannels);
  }

 private:
  std::vector<int16_t> samples_;
};

/*!
 * \brief Builds the MOV guide timeline for \a job at \a fps (video fps).
 *
 * Returns an empty shared timeline when no plan declares \c audio.
 */
StatusOr<std::shared_ptr<const AudioPcmTimeline>> build_job_audio_timeline(
    const Job& job, int fps);

}  // namespace dailyboy
