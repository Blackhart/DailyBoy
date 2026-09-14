#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

#include "image/frame.hpp"
#include "image/sequence.hpp"
#include "status.hpp"

namespace dailyview {

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

  /*!
   * \brief Opens the examples plate and seeks to \c frame_start.
   * \return Engine \c Status; on failure also updates \c errorString.
   */
  dailyboy::Status LoadExamplePlate();

  /*!
   * \brief Loads \a frame into the RGB8 buffer (clamped to the open range).
   *        Updates \c errorString on I/O failure.
   */
  Q_INVOKABLE void Seek(int frame);

  Q_INVOKABLE void Play();
  Q_INVOKABLE void Pause();
  Q_INVOKABLE void TogglePlay();

  int frame_width() const { return width_; }
  int frame_height() const { return height_; }
  int current_frame() const { return current_frame_; }
  int frame_start() const;
  int frame_end() const;
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
  dailyboy::Status OpenSequence(const std::string& pattern, int frame_start,
                                int frame_end);
  dailyboy::Status SeekTo(int frame);
  dailyboy::Status LoadFrame(int frame);
  dailyboy::Status PackRgb8(const dailyboy::Frame& frame);
  void SetChannelOrder(int channel_count, int order[3]);
  void SetError(const dailyboy::Status& status);
  int ClampFrame(int frame) const;

  std::optional<dailyboy::Sequence> sequence_;
  int current_frame_ = 0;
  bool playing_ = false;
  int width_ = 0;
  int height_ = 0;
  std::vector<uint8_t> rgb8_;
  QString error_string_;
};

}  // namespace dailyview
