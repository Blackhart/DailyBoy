#pragma once

#include <OpenImageIO/imagebuf.h>

#include <string>

#include "image/geom.hpp"
#include "job/text.hpp"
#include "status.hpp"

namespace dailyboy {

/*!
 * \brief Places the top-left of a box from a layout, pixel, or percent
 *        position.
 *
 * Layout mode insets the box 10 px from the matching edge or centers it.
 * Pixel mode uses \c x/\c y as-is. Percent mode maps \c x/\c y in 0–100
 * onto the canvas.
 */
Point compute_box_position(const TextPosition& position, const Size& box,
                           int canvas_width, int canvas_height);

/*!
 * \brief Measures glyph bounds with OpenImageIO \c text_size.
 */
StatusOr<Size> compute_text_size(const std::string& text, const TextFont& font);

/*!
 * \brief Paints glyphs at \a origin (top-left) with \a font color.
 */
Status draw_text(OIIO::ImageBuf& canvas, const Point& origin,
                 const std::string& text, const TextFont& font);

}  // namespace dailyboy
