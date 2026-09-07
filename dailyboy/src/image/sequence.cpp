/*!
 * \file sequence.cpp
 * \brief Job sequence pattern resolution (fileseq + tokens) and ImageBuf load.
 */

#include "image/sequence.hpp"

#include <OpenImageIO/imagebuf.h>
#include <fileseq/error.h>
#include <fileseq/sequence.h>

#include <exception>
#include <string>
#include <system_error>
#include <utility>

#include "error/image.hpp"
#include "process/path_tokens.hpp"
#include "status.hpp"

namespace dailyboy {

namespace {

StatusOr<std::string> validated_pattern(const std::string& pattern) {
  if (pattern.empty()) {
    return Status::User(std::string(USER_ERROR_IO_2));
  }
  fileseq::Status ok;
  const fileseq::FileSequence seq(pattern, &ok);
  if (!ok || !seq.isValid()) {
    return Status::User(std::string(USER_ERROR_IO_3) + " " + pattern);
  }
  return pattern;
}

StatusOr<Frame> read_image(const std::filesystem::path& path) {
  try {
    OIIO::ImageBuf buf(path.string());
    if (!buf.read()) {
      return Status::User(std::string(USER_ERROR_IO_7) + " " + path.string() +
                          ": " + buf.geterror());
    }
    return Frame(std::move(buf));
  } catch (const std::exception& ex) {
    return Status::User(std::string(USER_ERROR_IO_7) + " " + path.string() +
                        ": " + ex.what());
  }
}

}  // namespace

Sequence::Iterator::Iterator(const Sequence* sequence, int frame)
    : sequence_(sequence), frame_(frame) {}

std::filesystem::path Sequence::Iterator::path() const {
  return sequence_->path_for_frame(frame_);
}

StatusOr<Frame> Sequence::Iterator::load() const {
  return sequence_->load(frame_);
}

Sequence::Iterator& Sequence::Iterator::operator++() {
  ++frame_;
  return *this;
}

bool Sequence::Iterator::operator==(const Iterator& other) const {
  return sequence_ == other.sequence_ && frame_ == other.frame_;
}

StatusOr<Sequence> Sequence::open(
    const JobSequence& sequence,
    const std::map<std::string, JobMetadataSubstitutionValue>& substitutions) {
  if (sequence.frame_start() > sequence.frame_end()) {
    return Status::User(std::string(USER_ERROR_IO_1));
  }
  DAILYBOY_ASSIGN_OR_RETURN(
      std::string expanded,
      expand_path_tokens(sequence.path(), substitutions, USER_ERROR_IO_6));
  DAILYBOY_ASSIGN_OR_RETURN(std::string pattern, validated_pattern(expanded));

  Sequence out;
  out.pattern_ = std::move(pattern);
  out.frame_start_ = sequence.frame_start();
  out.frame_end_ = sequence.frame_end();
  return out;
}

std::size_t Sequence::size() const {
  return static_cast<std::size_t>(frame_end_ - frame_start_) + 1;
}

bool Sequence::contains(int frame) const {
  return frame >= frame_start_ && frame <= frame_end_;
}

std::filesystem::path Sequence::path_for_frame(int frame) const {
  fileseq::Status ok;
  const fileseq::FileSequence seq(pattern_, &ok);
  return std::filesystem::path(seq.frame(static_cast<fileseq::Frame>(frame)));
}

StatusOr<std::filesystem::path> Sequence::path(int frame) const {
  if (!contains(frame)) {
    return Status::User(std::string(USER_ERROR_IO_4));
  }
  return path_for_frame(frame);
}

StatusOr<Frame> Sequence::load(int frame) const {
  DAILYBOY_ASSIGN_OR_RETURN(const std::filesystem::path file, path(frame));
  std::error_code ec;
  if (!std::filesystem::is_regular_file(file, ec) || ec) {
    return Status::User(std::string(USER_ERROR_IO_5) + " " + file.string());
  }
  return read_image(file);
}

Sequence::Iterator Sequence::begin() const {
  return Iterator(this, frame_start_);
}

Sequence::Iterator Sequence::end() const {
  return Iterator(this, frame_end_ + 1);
}

}  // namespace dailyboy
