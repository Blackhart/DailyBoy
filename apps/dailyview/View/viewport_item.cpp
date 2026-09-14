#include "View/viewport_item.hpp"

#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>

#include "Model/sequence_model.hpp"

namespace dailyview {
namespace {

constexpr char kVertexShader[] = R"(
attribute vec2 position;
attribute vec2 texCoord;
varying vec2 vTexCoord;
void main() {
  vTexCoord = texCoord;
  gl_Position = vec4(position, 0.0, 1.0);
}
)";

constexpr char kFragmentShader[] = R"(
uniform sampler2D tex;
varying vec2 vTexCoord;
void main() {
  gl_FragColor = texture2D(tex, vTexCoord);
}
)";

class ViewportRenderer : public QQuickFramebufferObject::Renderer,
                         protected QOpenGLFunctions {
 public:
  ViewportRenderer() {
    initializeOpenGLFunctions();
    BuildProgram();
  }

  ~ViewportRenderer() override {
    DeleteTexture();
    delete program_;
  }

  void render() override {
    glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    if (texture_id_ == 0 || program_ == nullptr) {
      return;
    }
    DrawTexturedQuad();
  }

  QOpenGLFramebufferObject* createFramebufferObject(
      const QSize& size) override {
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    return new QOpenGLFramebufferObject(size, format);
  }

  void synchronize(QQuickFramebufferObject* item) override {
    auto* viewport = static_cast<ViewportItem*>(item);
    std::vector<uint8_t> pixels;
    viewport->CopyRgb8(&pixels);
    const int width = viewport->texture_width();
    const int height = viewport->texture_height();
    if (pixels.empty() || width <= 0 || height <= 0) {
      return;
    }
    UploadTexture(width, height, pixels.data());
  }

 private:
  void BuildProgram() {
    program_ = new QOpenGLShaderProgram();
    program_->addCacheableShaderFromSourceCode(QOpenGLShader::Vertex,
                                               kVertexShader);
    program_->addCacheableShaderFromSourceCode(QOpenGLShader::Fragment,
                                               kFragmentShader);
    program_->bindAttributeLocation("position", 0);
    program_->bindAttributeLocation("texCoord", 1);
    program_->link();
  }

  void UploadTexture(int width, int height, const uint8_t* pixels) {
    if (texture_id_ == 0) {
      glGenTextures(1, &texture_id_);
    }
    glBindTexture(GL_TEXTURE_2D, texture_id_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
                 GL_UNSIGNED_BYTE, pixels);
  }

  void DeleteTexture() {
    if (texture_id_ != 0) {
      glDeleteTextures(1, &texture_id_);
      texture_id_ = 0;
    }
  }

  void DrawTexturedQuad() {
    static const GLfloat kVertices[] = {
        -1.0f, -1.0f, 0.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f,  0.0f, 0.0f, 1.0f, 1.0f,  1.0f, 0.0f,
    };

    program_->bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_id_);
    program_->setUniformValue("tex", 0);
    program_->enableAttributeArray(0);
    program_->enableAttributeArray(1);
    program_->setAttributeArray(0, GL_FLOAT, kVertices, 2, 4 * sizeof(GLfloat));
    program_->setAttributeArray(1, GL_FLOAT, kVertices + 2, 2,
                                4 * sizeof(GLfloat));
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    program_->disableAttributeArray(0);
    program_->disableAttributeArray(1);
    program_->release();
  }

  QOpenGLShaderProgram* program_ = nullptr;
  GLuint texture_id_ = 0;
};

}  // namespace

ViewportItem::ViewportItem(QQuickItem* parent)
    : QQuickFramebufferObject(parent) {
  setMirrorVertically(true);
}

QQuickFramebufferObject::Renderer* ViewportItem::createRenderer() const {
  return new ViewportRenderer();
}

void ViewportItem::setModel(SequenceModel* model) {
  if (model_ == model) {
    return;
  }
  if (model_ != nullptr) {
    disconnect(model_, nullptr, this, nullptr);
  }
  model_ = model;
  if (model_ != nullptr) {
    connect(model_, &SequenceModel::frameReady, this,
            &ViewportItem::OnFrameReady);
    connect(model_, &SequenceModel::errorChanged, this,
            &ViewportItem::OnErrorChanged);
    const dailyboy::Status status = model_->LoadExamplePlate();
    if (status.ok()) {
      OnFrameReady();
    } else {
      OnErrorChanged();
    }
  }
  emit modelChanged();
}

int ViewportItem::texture_width() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return width_;
}

int ViewportItem::texture_height() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return height_;
}

void ViewportItem::CopyRgb8(std::vector<uint8_t>* out) const {
  std::lock_guard<std::mutex> lock(mutex_);
  *out = rgb8_;
}

void ViewportItem::OnFrameReady() {
  if (model_ == nullptr) {
    return;
  }
  {
    std::lock_guard<std::mutex> lock(mutex_);
    width_ = model_->frame_width();
    height_ = model_->frame_height();
    rgb8_ = model_->rgb8();
  }
  update();
}

void ViewportItem::OnErrorChanged() {}

}  // namespace dailyview
