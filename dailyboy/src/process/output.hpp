#pragma once

#include <memory>

#include "image/frame.hpp"
#include "job/job.hpp"
#include "job/output.hpp"
#include "status.hpp"

namespace dailyboy {

/*!
 * \brief Open movie and image-sequence writers for one daily.
 *
 * Move-only. Open with \c open, write with \c write_outputs /
 * \c write_sequence / \c write_movie, then \c close. Frames must already be
 * display-referred.
 */
class Outputs {
 public:
  Outputs(Outputs&&) noexcept;
  Outputs& operator=(Outputs&&) noexcept;
  ~Outputs();

  Outputs(const Outputs&) = delete;
  Outputs& operator=(const Outputs&) = delete;

  /*!
   * \brief Collects enabled videos and image sequences and opens sequence
   *        writers.
   *
   * Movies open on the first write (need the frame size).
   */
  static StatusOr<Outputs> open(const Job& job);

  /*!
   * \brief Flushes movie writers. Safe to call more than once.
   */
  Status close();

 private:
  friend Status write_outputs(Outputs& out, const Frame& frame,
                              int frame_number,
                              const JobOutputDisplayView& display_view);
  friend Status write_outputs(Outputs& out, const Frame& frame,
                              int frame_number);
  friend Status write_sequence(Outputs& out, const Frame& frame,
                               int frame_number,
                               const JobOutputDisplayView& display_view);
  friend Status write_movie(Outputs& out, const Frame& frame,
                            const JobOutputDisplayView& display_view);

  Outputs();
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

/*!
 * \brief Writes \a frame to deliverables matching \a display_view.
 */
Status write_outputs(Outputs& out, const Frame& frame, int frame_number,
                     const JobOutputDisplayView& display_view);

/*!
 * \brief Writes \a frame to every enabled deliverable (e.g. slate).
 */
Status write_outputs(Outputs& out, const Frame& frame, int frame_number);

Status write_sequence(Outputs& out, const Frame& frame, int frame_number,
                      const JobOutputDisplayView& display_view);
Status write_movie(Outputs& out, const Frame& frame,
                   const JobOutputDisplayView& display_view);

}  // namespace dailyboy
