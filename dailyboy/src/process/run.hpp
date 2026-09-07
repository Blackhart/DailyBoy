#pragma once

#include "job/job.hpp"
#include "status.hpp"

namespace dailyboy {

/*!
 * \brief Loads \c plans[0] frames and writes every enabled
 *        \c output.videos[] MOV and \c output.image_sequences[] file
 *        sequence.
 *
 * Skips \c enabled: false entries. Expands string substitutions in paths
 * and patterns and creates parent directories. If \c layout.slate is
 * present and \c duration_frames is greater than 0, writes that many
 * identical canvas frames (background plus slate lines, no plate, no
 * burn-ins) at the start of each enabled movie and image sequence. Sequence
 * slate files use numbers immediately before \c frame_start; plate files
 * keep source frame numbers. Then reformats each source frame when needed,
 * composes onto \c layout.canvas (fit, PAR, margins, background), draws
 * \c layout.burn_ins, and writes each enabled movie and image sequence.
 * Ignores job color. Output size is the canvas; later frames of a
 * different size fail.
 *
 * \param job Loaded job (sequence path tokens expanded at Sequence::open).
 */
Status run_job(const Job& job);

}  // namespace dailyboy
