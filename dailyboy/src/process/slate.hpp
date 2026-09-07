#pragma once

#include "image/frame.hpp"
#include "job/job.hpp"
#include "job/layout.hpp"
#include "process/compositing.hpp"
#include "process/output.hpp"
#include "process/tokens.hpp"
#include "status.hpp"

namespace dailyboy {

/*!
 * \brief Writes head-slate frames (no-op if duration is 0).
 */
Status write_slates(const Job& job, Outputs& out);

/*!
 * \brief Draws \c layout.slate.lines onto \a canvas (display-referred RGB).
 */
Status draw_slate_texts(Frame& canvas, const Job& job,
                        const OverlayTokenContext& tokens);

}  // namespace dailyboy
