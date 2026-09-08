#pragma once

#include <map>
#include <optional>
#include <string>

#include "image/frame.hpp"
#include "job/metadata.hpp"
#include "job/output.hpp"
#include "job/plans.hpp"
#include "status.hpp"

namespace dailyboy {

/*!
 * \brief Writes frames to an output file sequence (OpenImageIO).
 *
 * \c open resolves the path pattern (tokens + fileseq). \c write maps
 * \a frame_number onto that pattern with no plan range; the file extension
 * selects the writer. Optional \c format_options set TypeDesc and OIIO attrs.
 */
class SequenceWriter {
 public:
  SequenceWriter() = default;
  SequenceWriter(const SequenceWriter&) = delete;
  SequenceWriter& operator=(const SequenceWriter&) = delete;
  SequenceWriter(SequenceWriter&&) noexcept = default;
  SequenceWriter& operator=(SequenceWriter&&) noexcept = default;

  /*!
   * \brief Parses \a sequence.path as an output pattern.
   *
   * Ignores \c frame_start / \c frame_end: the writer accepts any frame
   * number passed to \c write.
   */
  static StatusOr<SequenceWriter> open(
      const JobSequence& sequence,
      const std::map<std::string, JobMetadataSubstitutionValue>& substitutions =
          {},
      std::optional<JobOutputImageSequenceFormatOptions> format_options =
          std::nullopt);

  /*!
   * \brief Writes \a frame to the file for \a frame_number (creates parents).
   */
  Status write(const Frame& frame, int frame_number);

 private:
  std::string pattern_;
  std::optional<JobOutputImageSequenceFormatOptions> format_options_;
};

}  // namespace dailyboy
