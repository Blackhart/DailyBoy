#include "Model/sequence_model.hpp"

#include <OpenImageIO/imagebufalgo.h>

#include "image/sequence.hpp"
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

dailyboy::Status SequenceModel::LoadExamplePlate() {
  dailyboy::StatusOr<dailyboy::Frame> frame =
      OpenAndLoadFirst(kExamplePattern, kExampleFrameStart, kExampleFrameEnd);
  if (!frame.ok()) {
    SetError(frame.status());
    return frame.status();
  }

  const dailyboy::Status pack_status = PackRgb8(*frame);
  if (!pack_status.ok()) {
    SetError(pack_status);
    return pack_status;
  }

  error_string_.clear();
  emit errorChanged();
  emit frameReady();
  return dailyboy::Status::Ok();
}

dailyboy::StatusOr<dailyboy::Frame> SequenceModel::OpenAndLoadFirst(
    const std::string& pattern, int frame_start, int frame_end) {
  dailyboy::JobSequence job_sequence;
  job_sequence.set_path(pattern);
  job_sequence.set_frame_start(frame_start);
  job_sequence.set_frame_end(frame_end);

  dailyboy::StatusOr<dailyboy::Sequence> sequence =
      dailyboy::Sequence::open(job_sequence);
  if (!sequence.ok()) {
    return sequence.status();
  }
  return (*sequence).load((*sequence).frame_start());
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
