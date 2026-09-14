#pragma once

#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>

#pragma push_macro("emit")
#undef emit
#include <tbb/task_arena.h>
#pragma pop_macro("emit")

#include "Model/load_frame_result.hpp"
#include "image/sequence.hpp"

namespace dailyview {

class SequenceModel;

/*!
 * \brief Serial TBB job runner that owns \c Sequence and packs RGB8 off-GUI.
 */
class JobRunner {
 public:
  explicit JobRunner(SequenceModel* model);
  ~JobRunner();

  JobRunner(const JobRunner&) = delete;
  JobRunner& operator=(const JobRunner&) = delete;

  void EnqueueOpen(std::string pattern, int frame_start, int frame_end);
  void EnqueueLoad(uint64_t generation, int frame);

 private:
  void OpenSequence(std::string pattern, int frame_start, int frame_end);
  void LoadFrame(uint64_t generation, int frame);
  void DeliverOpen(OpenSequenceResult result);
  void DeliverLoad(LoadFrameResult result);
  void BeginJob();
  void EndJob();
  void WaitIdle();

  SequenceModel* model_ = nullptr;
  tbb::task_arena arena_{1};
  std::optional<dailyboy::Sequence> sequence_;
  std::mutex mutex_;
  std::condition_variable idle_;
  int outstanding_ = 0;
};

}  // namespace dailyview
