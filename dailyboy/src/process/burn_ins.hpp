#pragma once

#include "image/frame.hpp"
#include "image/sequence.hpp"
#include "job/job.hpp"
#include "job/output.hpp"
#include "process/colorimetry.hpp"
#include "process/compositing.hpp"
#include "process/output.hpp"
#include "status.hpp"

namespace dailyboy {

/*!
 * \brief Loads, prepares, composes, and writes each plate frame.
 */
Status write_burnins(const Job& job, Outputs& out,
                     const ColorPipeline& color_pipeline);

/*!
 * \brief Loads one plate frame in its input color space.
 */
StatusOr<Frame> load_plate(const Sequence::Iterator& it);

/*!
 * \brief Load → reformat → I→W → resize → DisplayView for one display_view.
 */
StatusOr<FittedPlate> prepare_plate(const Sequence::Iterator& it,
                                    const Job& job,
                                    const ColorPipeline& color_pipeline,
                                    const JobOutputDisplayView& display_view);

}  // namespace dailyboy
