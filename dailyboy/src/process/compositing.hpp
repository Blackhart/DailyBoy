#pragma once

#include "image/frame.hpp"
#include "image/geom.hpp"
#include "job/job.hpp"
#include "job/layout.hpp"
#include "process/tokens.hpp"
#include "status.hpp"

namespace dailyboy {

/*!
 * \brief Plate resized for the layout image frame (display or working pixels).
 */
struct FittedPlate {
  Frame frame;
  Point origin;
  Rect image_frame;
};

/*!
 * \brief Allocates an RGB float canvas sized from \c job.layout().canvas().
 *
 * Does not fill the background; prefer \c make_filled_canvas for a ready canvas.
 */
StatusOr<Frame> make_canvas(const Job& job);

/*!
 * \brief Allocates a canvas and fills it with \c layout.background.
 */
StatusOr<Frame> make_filled_canvas(const Job& job);

/*!
 * \brief Usable rectangle after min margins (letterbox / pillarbox).
 */
StatusOr<Rect> usable_image_frame(const Job& job);

/*!
 * \brief Scales \a plate into the layout image frame (PAR, contain/cover).
 *
 * Resize only; color space is unchanged.
 */
StatusOr<FittedPlate> resize_plate(const Frame& plate, const Job& job);

/*!
 * \brief Pastes a fitted plate into \a canvas at its stored origin.
 */
Status paste_plate_into_canvas(Frame& canvas, const FittedPlate& plate);

/*!
 * \brief Draws \c layout.burn_ins onto \a frame (display-referred RGB).
 */
Status draw_hud(Frame& frame, const Job& job,
                const OverlayTokenContext& tokens);

/*!
 * \brief Pastes the plate then draws the HUD onto a filled canvas.
 */
StatusOr<Frame> compose(Frame canvas, const FittedPlate& plate, const Job& job,
                        const OverlayTokenContext& tokens);

/*!
 * \brief Scale that fits \a plate into \a image_frame (contain or cover).
 *
 * Display width is stored width times \a pixel_aspect.
 */
StatusOr<double> scale_to_fit(const Size& plate, const Rect& image_frame,
                              double pixel_aspect,
                              JobLayoutImage::JobLayoutImageFitValue fit);

}  // namespace dailyboy
