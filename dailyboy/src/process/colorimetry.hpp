#pragma once

#include <memory>
#include <string>

#include "image/frame.hpp"
#include "job/job.hpp"
#include "job/output.hpp"
#include "status.hpp"

namespace dailyboy {

/*!
 * \brief Prepared OCIO config and software processors for one job run.
 *
 * Built once via \c prepare; converts reuse the stored handles (no per-frame
 * config load). Identity / passthrough transforms leave the matching handle
 * empty.
 */
class ColorPipeline {
 public:
  ColorPipeline(ColorPipeline&&) noexcept;
  ColorPipeline& operator=(ColorPipeline&&) noexcept;
  ~ColorPipeline();

  ColorPipeline(const ColorPipeline&) = delete;
  ColorPipeline& operator=(const ColorPipeline&) = delete;

  /*!
   * \brief Loads the OCIO config (if needed) and builds I→W plus DisplayView
   *        processors for enabled deliverables.
   *
   * Skips loading the config when I→W is identity and every display_view is
   * the test passthrough pair (\c passthrough / \c passthrough).
   */
  static StatusOr<ColorPipeline> prepare(const Job& job);

 private:
  friend Status convert_color_from_input_to_working(Frame& frame,
                                                    const ColorPipeline&);
  friend Status convert_color_from_working_to_display(
      Frame& frame, const ColorPipeline&, const JobOutputDisplayView&);

  ColorPipeline();
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

/*!
 * \brief Converts plate pixels from plan input space to working space.
 *
 * No-op when \c prepare left the I→W processor empty (same spaces).
 */
Status convert_color_from_input_to_working(Frame& frame,
                                           const ColorPipeline& color_pipeline);

/*!
 * \brief Converts pixels from working space through an OCIO DisplayView.
 *
 * \a display_view must have been collected by \c prepare. No-op when the
 * matching processor is empty (passthrough / identity).
 */
Status convert_color_from_working_to_display(
    Frame& frame, const ColorPipeline& color_pipeline,
    const JobOutputDisplayView& display_view);

/*!
 * \brief True when \a display_view is the unit-test passthrough pair.
 */
bool is_passthrough_display_view(const JobOutputDisplayView& display_view);

/*!
 * \brief Map key for a display/view pair.
 */
std::string display_view_key(const JobOutputDisplayView& display_view);

}  // namespace dailyboy
