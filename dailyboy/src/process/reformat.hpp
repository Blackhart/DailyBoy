#pragma once

#include "image/frame.hpp"
#include "status.hpp"

namespace dailyboy {

/*!
 * \brief Makes a loaded frame safe for dailies.
 *
 * Drops extra channels, then (if needed) pastes once onto a black canvas
 * the size of the display window, with the region of interest at the
 * right offset. Does not pad to even size; encode size is \c layout.canvas.
 *
 * \param frame Loaded source frame; replaced in place when work is needed.
 */
Status reformat(Frame& frame);

}  // namespace dailyboy
