#include "Model/sequence_model.hpp"

#include <utility>

#pragma push_macro("emit")
#undef emit
#include "Model/job_runner.hpp"
#pragma pop_macro("emit")

#ifndef DAILYBOY_SOURCE_DIR
#error "DAILYBOY_SOURCE_DIR must be defined for DailyView example plates"
#endif

namespace dailyview {
namespace {

constexpr char kExamplePattern[] =
    DAILYBOY_SOURCE_DIR "/examples/frames/2048x1080/plate.%04d.png";
constexpr int kExampleFrameStart = 1001;
constexpr int kExampleFrameEnd = 1048;

}  // namespace

SequenceModel::SequenceModel(QObject* parent)
    : QObject(parent), job_runner_(std::make_unique<JobRunner>(this)) {}

SequenceModel::~SequenceModel() { job_runner_.reset(); }

dailyboy::Status SequenceModel::LoadExamplePlate() {
  sequence_open_ = false;
  Pause();
  job_runner_->EnqueueOpen(kExamplePattern, kExampleFrameStart,
                           kExampleFrameEnd);
  return dailyboy::Status::Ok();
}

void SequenceModel::Seek(int frame) {
  if (!sequence_open_) {
    return;
  }

  desired_frame_ = ClampFrame(frame);
  current_frame_ = desired_frame_;
  ++generation_;
  emit currentFrameChanged();

  if (load_inflight_) {
    return;
  }
  EnqueueDesiredLoad();
}

void SequenceModel::EnqueueDesiredLoad() {
  load_inflight_ = true;
  job_runner_->EnqueueLoad(generation_, desired_frame_);
}

void SequenceModel::OnSequenceOpened(OpenSequenceResult result) {
  if (!result.ok) {
    sequence_open_ = false;
    SetErrorMessage(result.error_message);
    return;
  }

  sequence_open_ = true;
  frame_start_ = result.frame_start;
  frame_end_ = result.frame_end;
  ClearError();
  emit sequenceChanged();
  Seek(frame_start_);
}

void SequenceModel::OnFrameLoaded(LoadFrameResult result) {
  load_inflight_ = false;

  if (result.generation != generation_) {
    EnqueueDesiredLoad();
    return;
  }

  if (!result.ok) {
    SetErrorMessage(result.error_message);
    return;
  }

  ApplyAcceptedFrame(std::move(result));
}

void SequenceModel::ApplyAcceptedFrame(LoadFrameResult result) {
  current_frame_ = result.frame;
  width_ = result.width;
  height_ = result.height;
  rgb8_ = std::move(result.rgb8);
  ClearError();
  emit currentFrameChanged();
  emit frameReady();
}

void SequenceModel::Play() {
  if (playing_ || !sequence_open_) {
    return;
  }
  playing_ = true;
  emit playingChanged();
}

void SequenceModel::Pause() {
  if (!playing_) {
    return;
  }
  playing_ = false;
  emit playingChanged();
}

void SequenceModel::TogglePlay() {
  if (playing_) {
    Pause();
  } else {
    Play();
  }
}

int SequenceModel::ClampFrame(int frame) const {
  if (!sequence_open_) {
    return frame;
  }
  if (frame < frame_start_) {
    return frame_start_;
  }
  if (frame > frame_end_) {
    return frame_end_;
  }
  return frame;
}

void SequenceModel::SetErrorMessage(const std::string& message) {
  error_string_ = QString::fromStdString(message);
  emit errorChanged();
}

void SequenceModel::ClearError() {
  if (error_string_.isEmpty()) {
    return;
  }
  error_string_.clear();
  emit errorChanged();
}

}  // namespace dailyview
