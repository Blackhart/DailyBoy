#pragma once

#include <OpenImageIO/imagebuf.h>

#include "image/frame.hpp"
#include "image/geom.hpp"
#include "job/primitives.hpp"
#include "status.hpp"

namespace dailyboy {

/*!
 * \brief Fills \a canvas with \a bg (up to three channels).
 */
Status fill_background(OIIO::ImageBuf& canvas, const RGBColor& bg);

/*!
 * \brief Drops channels after RGB. No-op if the buffer already has at most 3.
 */
Status drop_extra_channels(Frame& frame);

/*!
 * \brief Pastes the data window onto a black canvas the size of the display
 *        window, origin at (0, 0).
 */
Status paste_to_display_window(Frame& frame);

/*!
 * \brief Pastes \a plate at \a origin, cropped to \a image_frame.
 */
Status paste_into_rect(OIIO::ImageBuf& canvas, const OIIO::ImageBuf& plate,
                       const Point& origin, const Rect& image_frame);

/*!
 * \brief Resamples \a src into \a scaled at \a fitted using \a filter.
 */
Status resize_buffer(OIIO::ImageBuf& scaled, const OIIO::ImageBuf& src,
                     const Size& fitted, const char* filter);

}  // namespace dailyboy
