#include "Model/sequence_model.hpp"

#include <OpenImageIO/imagebufalgo.h>

#include "job/plans.hpp"

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

SequenceModel::SequenceModel(QObject* parent) : QObject(parent) {}

int SequenceModel::frame_start() const {
  return sequence_ ? sequence_->frame_start() : 0;
}

int SequenceModel::frame_end() const {
  return sequence_ ? sequence_->frame_end() : 0;
}

dailyboy::Status SequenceModel::LoadExamplePlate() {
  const dailyboy::Status open_status =
      OpenSequence(kExamplePattern, kExampleFrameStart, kExampleFrameEnd);
  if (!open_status.ok()) {
    return open_status;
  }
  return SeekTo(sequence_->frame_start());
}

dailyboy::Status SequenceModel::OpenSequence(const std::string& pattern,
                                             int frame_start, int frame_end) {
  dailyboy::JobSequence job_sequence;
  job_sequence.set_path(pattern);
  job_sequence.set_frame_start(frame_start);
  job_sequence.set_frame_end(frame_end);

  dailyboy::StatusOr<dailyboy::Sequence> opened =
      dailyboy::Sequence::open(job_sequence);
  if (!opened.ok()) {
    SetError(opened.status());
    return opened.status();
  }

  sequence_ = std::move(*opened);
  Pause();
  emit sequenceChanged();
  return dailyboy::Status::Ok();
}

dailyboy::Status SequenceModel::SeekTo(int frame) {
  if (!sequence_) {
    return dailyboy::Status::User("no sequence open");
  }

  const int clamped = ClampFrame(frame);
  const dailyboy::Status load_status = LoadFrame(clamped);
  if (!load_status.ok()) {
    SetError(load_status);
    return load_status;
  }

  current_frame_ = clamped;
  error_string_.clear();
  emit errorChanged();
  emit currentFrameChanged();
  emit frameReady();
  return dailyboy::Status::Ok();
}

void SequenceModel::Seek(int frame) { SeekTo(frame); }

dailyboy::Status SequenceModel::LoadFrame(int frame) {
  dailyboy::StatusOr<dailyboy::Frame> loaded = sequence_->load(frame);
  if (!loaded.ok()) {
    return loaded.status();
  }
  return PackRgb8(*loaded);
}

void SequenceModel::Play() {
  if (playing_ || !sequence_) {
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
  if (!sequence_) {
    return frame;
  }
  if (frame < sequence_->frame_start()) {
    return sequence_->frame_start();
  }
  if (frame > sequence_->frame_end()) {
    return sequence_->frame_end();
  }
  return frame;
}

void SequenceModel::SetChannelOrder(int channel_count, int order[3]) {
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

dailyboy::Status SequenceModel::PackRgb8(const dailyboy::Frame& frame) {
  const int width = frame.width();
  const int height = frame.height();
  if (width <= 0 || height <= 0) {
    return dailyboy::Status::User("invalid frame size");
  }

  int order[3] = {};
  float fill[3] = {0.0f, 0.0f, 0.0f};
  SetChannelOrder(frame.buf().spec().nchannels, order);

  OIIO::ImageBuf rgb_buf;
  if (!OIIO::ImageBufAlgo::channels(rgb_buf, frame.buf(), 3, order, fill)) {
    return dailyboy::Status::User(rgb_buf.geterror());
  }

  rgb8_.resize(static_cast<std::size_t>(width) *
               static_cast<std::size_t>(height) * 3);
  if (!rgb_buf.get_pixels(OIIO::ROI(0, width, 0, height), OIIO::TypeDesc::UINT8,
                          rgb8_.data())) {
    return dailyboy::Status::User(rgb_buf.geterror());
  }

  width_ = width;
  height_ = height;
  return dailyboy::Status::Ok();
}

void SequenceModel::SetError(const dailyboy::Status& status) {
  error_string_ = QString::fromStdString(status.message());
  emit errorChanged();
}

}  // namespace dailyview
