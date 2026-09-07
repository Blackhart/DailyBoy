#pragma once

#include <cstddef>
#include <filesystem>
#include <map>
#include <string>

#include "image/frame.hpp"
#include "job/metadata.hpp"
#include "job/plans.hpp"
#include "status.hpp"

namespace dailyboy {

/*!
 * \brief Resolved input sequence: cursor over \c [frame_start, frame_end].
 *
 * Opens a \c JobSequence pattern (string substitutions expanded, frame maps
 * rejected). The iterator walks frame numbers and paths without I/O;
 * \c load() reads an \c ImageBuf. Direct \c path(frame) / \c load(frame)
 * seek without iterating.
 */
class Sequence {
 public:
  class Iterator {
   public:
    int frame() const { return frame_; }
    std::filesystem::path path() const;
    StatusOr<Frame> load() const;

    Iterator& operator++();
    bool operator==(const Iterator& other) const;
    bool operator!=(const Iterator& other) const { return !(*this == other); }

   private:
    friend class Sequence;
    Iterator(const Sequence* sequence, int frame);
    const Sequence* sequence_ = nullptr;
    int frame_ = 0;
  };

  Sequence() = default;

  /*!
   * \brief Parses the job sequence pattern and inclusive frame range.
   * \param sequence Path pattern plus \c frame_start / \c frame_end.
   * \param substitutions String tokens expanded in \c sequence.path; frame
   *        maps in that path yield a user error.
   * \return Opened sequence, or user status on empty/invalid pattern, inverted
   *         range, or a frame-map token in the path.
   */
  static StatusOr<Sequence> open(
      const JobSequence& sequence,
      const std::map<std::string, JobMetadataSubstitutionValue>& substitutions =
          {});

  int frame_start() const { return frame_start_; }
  int frame_end() const { return frame_end_; }
  std::size_t size() const;

  /*!
   * \brief Returns the file path for \a frame without reading the file.
   * \return User status when \a frame is outside the range.
   */
  StatusOr<std::filesystem::path> path(int frame) const;

  /*!
   * \brief Reads the image at \a frame into a \c Frame.
   * \return User status when \a frame is out of range, the file is missing, or
   *         OpenImageIO cannot read it.
   */
  StatusOr<Frame> load(int frame) const;

  Iterator begin() const;
  Iterator end() const;

 private:
  std::string pattern_;
  int frame_start_ = 0;
  int frame_end_ = 0;

  std::filesystem::path path_for_frame(int frame) const;
  bool contains(int frame) const;
};

}  // namespace dailyboy
