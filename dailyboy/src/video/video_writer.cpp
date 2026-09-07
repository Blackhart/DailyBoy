/*!
 * \file video_writer.cpp
 * \brief Factory for codec-specific MOV writers.
 */

#include "video/video_writer.hpp"

#include "video/dnxhd_writer.hpp"
#include "video/h264_writer.hpp"
#include "video/mjpeg_writer.hpp"

namespace dailyboy {

std::unique_ptr<VideoWriter> make_video_writer(
    JobOutputVideo::JobOutputVideoCodecValue codec) {
  switch (codec) {
    case JobOutputVideo::JobOutputVideoCodecValue::Mjpeg:
      return std::make_unique<MjpegWriter>();
    case JobOutputVideo::JobOutputVideoCodecValue::Dnxhd:
      return std::make_unique<DnxhdWriter>();
    case JobOutputVideo::JobOutputVideoCodecValue::H264:
      return std::make_unique<H264Writer>();
  }
  return std::make_unique<H264Writer>();
}

}  // namespace dailyboy
