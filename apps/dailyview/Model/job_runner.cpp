#include "Model/job_runner.hpp"

#include <utility>

#include <OpenImageIO/imagebufalgo.h>
#include <QMetaObject>

#include "Model/load_frame_result.hpp"
#include "Model/sequence_model.hpp"
#include "job/plans.hpp"

namespace dailyview {
namespace {

void SetChannelOrder(int channel_count, int order[3]) {
  order[0] = 0;
  order[1] = 1;
  order[2] = 2;
  if (channel_count < 2) {
    order[1] = 0;
    order[2] = 0;
  } else if (channel_count < 3) {
    order[2] = -1;
  }
}

dailyboy::Status ExtractRgb8(const dailyboy::Frame& frame, int width,
                             int height, std::vector<uint8_t>* rgb8) {
  int order[3] = {};
  float fill[3] = {0.0f, 0.0f, 0.0f};
  SetChannelOrder(frame.buf().spec().nchannels, order);

  OIIO::ImageBuf rgb_buf;
  if (!OIIO::ImageBufAlgo::channels(rgb_buf, frame.buf(), 3, order, fill)) {
    return dailyboy::Status::User(rgb_buf.geterror());
  }

  rgb8->resize(static_cast<std::size_t>(width) *
               static_cast<std::size_t>(height) * 3);
  if (!rgb_buf.get_pixels(OIIO::ROI(0, width, 0, height), OIIO::TypeDesc::UINT8,
                          rgb8->data())) {
    return dailyboy::Status::User(rgb_buf.geterror());
  }
  return dailyboy::Status::Ok();
}

dailyboy::Status PackRgb8(const dailyboy::Frame& frame,
                          LoadFrameResult* result) {
  const int width = frame.width();
  const int height = frame.height();
  if (width <= 0 || height <= 0) {
    return dailyboy::Status::User("invalid frame size");
  }

  const dailyboy::Status status =
      ExtractRgb8(frame, width, height, &result->rgb8);
  if (!status.ok()) {
    return status;
  }
  result->width = width;
  result->height = height;
  return dailyboy::Status::Ok();
}

LoadFrameResult MakeFailedLoad(uint64_t generation, int frame,
                               std::string message) {
  LoadFrameResult result;
  result.generation = generation;
  result.frame = frame;
  result.ok = false;
  result.error_message = std::move(message);
  return result;
}

}  // namespace

JobRunner::JobRunner(SequenceModel* model) : model_(model) {}

JobRunner::~JobRunner() { WaitIdle(); }

void JobRunner::BeginJob() {
  std::lock_guard<std::mutex> lock(mutex_);
  ++outstanding_;
}

void JobRunner::EndJob() {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    --outstanding_;
  }
  idle_.notify_all();
}

void JobRunner::WaitIdle() {
  std::unique_lock<std::mutex> lock(mutex_);
  idle_.wait(lock, [this] { return outstanding_ == 0; });
}

void JobRunner::EnqueueOpen(std::string pattern, int frame_start,
                            int frame_end) {
  BeginJob();
  arena_.enqueue([this, pattern = std::move(pattern), frame_start, frame_end] {
    OpenSequence(std::move(pattern), frame_start, frame_end);
    EndJob();
  });
}

void JobRunner::EnqueueLoad(uint64_t generation, int frame) {
  BeginJob();
  arena_.enqueue([this, generation, frame] {
    LoadFrame(generation, frame);
    EndJob();
  });
}

void JobRunner::OpenSequence(std::string pattern, int frame_start,
                             int frame_end) {
  dailyboy::JobSequence job_sequence;
  job_sequence.set_path(std::move(pattern));
  job_sequence.set_frame_start(frame_start);
  job_sequence.set_frame_end(frame_end);

  OpenSequenceResult result;
  dailyboy::StatusOr<dailyboy::Sequence> opened =
      dailyboy::Sequence::open(job_sequence);
  if (!opened.ok()) {
    result.ok = false;
    result.error_message = opened.status().message();
    DeliverOpen(std::move(result));
    return;
  }

  sequence_ = std::move(*opened);
  result.ok = true;
  result.frame_start = sequence_->frame_start();
  result.frame_end = sequence_->frame_end();
  DeliverOpen(std::move(result));
}

void JobRunner::LoadFrame(uint64_t generation, int frame) {
  if (!sequence_) {
    DeliverLoad(MakeFailedLoad(generation, frame, "no sequence open"));
    return;
  }

  dailyboy::StatusOr<dailyboy::Frame> loaded = sequence_->load(frame);
  if (!loaded.ok()) {
    DeliverLoad(
        MakeFailedLoad(generation, frame, loaded.status().message()));
    return;
  }

  LoadFrameResult result;
  result.generation = generation;
  result.frame = frame;
  const dailyboy::Status pack_status = PackRgb8(*loaded, &result);
  if (!pack_status.ok()) {
    DeliverLoad(MakeFailedLoad(generation, frame, pack_status.message()));
    return;
  }

  result.ok = true;
  DeliverLoad(std::move(result));
}

void JobRunner::DeliverOpen(OpenSequenceResult result) {
  QMetaObject::invokeMethod(
      model_,
      [model = model_, result = std::move(result)]() mutable {
        model->OnSequenceOpened(std::move(result));
      },
      Qt::QueuedConnection);
}

void JobRunner::DeliverLoad(LoadFrameResult result) {
  QMetaObject::invokeMethod(
      model_,
      [model = model_, result = std::move(result)]() mutable {
        model->OnFrameLoaded(std::move(result));
      },
      Qt::QueuedConnection);
}

}  // namespace dailyview
