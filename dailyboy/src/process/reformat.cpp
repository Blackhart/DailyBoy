/*!
 * \file reformat.cpp
 * \brief Display-window restore and alpha drop.
 */

#include "process/reformat.hpp"

#include <dailyboy/log.hpp>
#include <exception>
#include <string>

#include "error/image.hpp"
#include "image/buffer.hpp"
#include "status.hpp"

namespace dailyboy {

namespace {

void log_drop_extra_channels(int channels_before, int channels_after) {
  if (channels_before <= 3) {
    return;
  }
  log_debug("reformat: drop alpha/extra channels (" +
            std::to_string(channels_before) + " -> " +
            std::to_string(channels_after) + ")");
}

void log_display_size(const Frame& frame) {
  log_debug("reformat: display " + std::to_string(frame.width()) + "x" +
            std::to_string(frame.height()));
}

}  // namespace

Status reformat(Frame& frame) {
  try {
    const int channels_before = frame.buf().spec().nchannels;
    DAILYBOY_RETURN_IF_ERROR(drop_extra_channels(frame));
    log_drop_extra_channels(channels_before, frame.buf().spec().nchannels);
    DAILYBOY_RETURN_IF_ERROR(paste_to_display_window(frame));
    log_display_size(frame);
    return Status::Ok();
  } catch (const std::exception& ex) {
    return Status::User(std::string(USER_ERROR_IO_9) + " " + ex.what());
  }
}

}  // namespace dailyboy
