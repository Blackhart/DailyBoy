#pragma once

#include <cstdint>
#include <vector>

#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

#include "image/frame.hpp"
#include "status.hpp"

namespace dailyview {

/*!
 * \brief Métier séquence : ouvre une plate et expose la première frame en RGB8.
 */
class SequenceModel : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("Provided as context property sequenceModel")
  Q_PROPERTY(int frameWidth READ frame_width NOTIFY frameReady)
  Q_PROPERTY(int frameHeight READ frame_height NOTIFY frameReady)
  Q_PROPERTY(QString errorString READ error_string NOTIFY errorChanged)

 public:
  explicit SequenceModel(QObject* parent = nullptr);

  /*!
   * \brief Opens the examples plate and loads \c frame_start (1001).
   * \return Engine \c Status; on failure also updates \c errorString.
   */
  dailyboy::Status LoadExamplePlate();

  int frame_width() const { return width_; }
  int frame_height() const { return height_; }
  const std::vector<uint8_t>& rgb8() const { return rgb8_; }
  QString error_string() const { return error_string_; }

 signals:
  void frameReady();
  void errorChanged();

 private:
  dailyboy::StatusOr<dailyboy::Frame> OpenAndLoadFirst(
      const std::string& pattern, int frame_start, int frame_end);
  dailyboy::Status PackRgb8(const dailyboy::Frame& frame);
  void SetChannelOrder(int channel_count, int order[3]);
  void SetError(const dailyboy::Status& status);

  int width_ = 0;
  int height_ = 0;
  std::vector<uint8_t> rgb8_;
  QString error_string_;
};

}  // namespace dailyview
