#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

#include "Model/load_frame_result.hpp"
#include "status.hpp"

namespace dailyview {

class JobRunner;

/*!
 * \brief Métier séquence : ouvre une plate, seek, et expose la frame RGB8.
 */
class SequenceModel : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("Provided as context property sequenceModel")
  Q_PROPERTY(int frameWidth READ frame_width NOTIFY frameReady)
  Q_PROPERTY(int frameHeight READ frame_height NOTIFY frameReady)
  Q_PROPERTY(int currentFrame READ current_frame NOTIFY currentFrameChanged)
  Q_PROPERTY(int frameStart READ frame_start NOTIFY sequenceChanged)
  Q_PROPERTY(int frameEnd READ frame_end NOTIFY sequenceChanged)
  Q_PROPERTY(bool playing READ playing NOTIFY playingChanged)
  Q_PROPERTY(QString errorString READ error_string NOTIFY errorChanged)

 public:
  explicit SequenceModel(QObject* parent = nullptr);
  ~SequenceModel() override;

  /*!
   * \brief Enqueues open of the examples plate then seek to \c frame_start.
   * \return \c Ok after enqueue; failures arrive via \c errorString.
   */
  dailyboy::Status LoadExamplePlate();

  /*!
   * \brief Requests load of \a frame (clamped); non-blocking via TBB.
   */
  Q_INVOKABLE void Seek(int frame);

  Q_INVOKABLE void Play();
  Q_INVOKABLE void Pause();
  Q_INVOKABLE void TogglePlay();

  void OnSequenceOpened(OpenSequenceResult result);
  void OnFrameLoaded(LoadFrameResult result);

  int frame_width() const { return width_; }
  int frame_height() const { return height_; }
  int current_frame() const { return current_frame_; }
  int frame_start() const { return frame_start_; }
  int frame_end() const { return frame_end_; }
  bool playing() const { return playing_; }
  const std::vector<uint8_t>& rgb8() const { return rgb8_; }
  QString error_string() const { return error_string_; }

 signals:
  void frameReady();
  void errorChanged();
  void currentFrameChanged();
  void sequenceChanged();
  void playingChanged();

 private:
  void EnqueueDesiredLoad();
  void ApplyAcceptedFrame(LoadFrameResult result);
  void SetErrorMessage(const std::string& message);
  void ClearError();
  int ClampFrame(int frame) const;

  int frame_start_ = 0;
  int frame_end_ = 0;
  bool sequence_open_ = false;
  int current_frame_ = 0;
  int desired_frame_ = 0;
  uint64_t generation_ = 0;
  bool load_inflight_ = false;
  bool playing_ = false;
  int width_ = 0;
  int height_ = 0;
  std::vector<uint8_t> rgb8_;
  QString error_string_;
  // Destroyed first so arena.wait() finishes while this QObject is alive.
  std::unique_ptr<JobRunner> job_runner_;
};

}  // namespace dailyview
