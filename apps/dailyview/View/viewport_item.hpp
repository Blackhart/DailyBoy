#pragma once

#include <cstdint>
#include <mutex>
#include <vector>

#include <QQuickFramebufferObject>
#include <QtQml/qqmlregistration.h>

namespace dailyview {

class SequenceModel;

/*!
 * \brief Qt Quick viewport that displays RGB8 frames from \c SequenceModel.
 */
class ViewportItem : public QQuickFramebufferObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(dailyview::SequenceModel* model READ model WRITE setModel NOTIFY
                 modelChanged)

 public:
  explicit ViewportItem(QQuickItem* parent = nullptr);

  Renderer* createRenderer() const override;

  SequenceModel* model() const { return model_; }
  void setModel(SequenceModel* model);

  int texture_width() const;
  int texture_height() const;
  void CopyRgb8(std::vector<uint8_t>* out) const;

 signals:
  void modelChanged();

 private:
  void OnFrameReady();
  void OnErrorChanged();

  SequenceModel* model_ = nullptr;
  mutable std::mutex mutex_;
  int width_ = 0;
  int height_ = 0;
  std::vector<uint8_t> rgb8_;
};

}  // namespace dailyview
