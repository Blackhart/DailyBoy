#pragma once

#include <OpenImageIO/imagebuf.h>

namespace dailyboy {

/*!
 * \brief One loaded source image: thin owner of an OpenImageIO \c ImageBuf.
 *
 * Pixels stay in the buffer as read (type, channels, metadata). Color
 * conversion and packing for encode happen at the writer, not here.
 */
class Frame {
 public:
  Frame() = default;
  explicit Frame(OIIO::ImageBuf buf) : buf_(std::move(buf)) {}

  OIIO::ImageBuf& buf() { return buf_; }
  const OIIO::ImageBuf& buf() const { return buf_; }

  int width() const { return buf_.spec().width; }
  int height() const { return buf_.spec().height; }

 private:
  OIIO::ImageBuf buf_;
};

}  // namespace dailyboy
