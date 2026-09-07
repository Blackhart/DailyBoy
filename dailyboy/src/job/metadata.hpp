#pragma once

#include <map>
#include <string>
#include <utility>
#include <variant>

namespace dailyboy {

/*!
 * \brief Substitution value: constant string or per-frame text map.
 *
 * Frame maps (keys = frame number or inclusive range) are allowed only in
 * burn-in templates and slate line text.
 */
using JobMetadataSubstitutionValue =
    std::variant<std::string, std::map<std::string, std::string>>;

/*!
 * \brief Production substitutions for \c {token} expansion in job strings.
 *
 * Corresponds to \c metadata.substitutions in the job YAML.
 */
class JobMetadata {
 public:
  JobMetadata() = default;
  ~JobMetadata() = default;

  const std::map<std::string, JobMetadataSubstitutionValue>& substitutions()
      const {
    return substitutions_;
  }
  std::map<std::string, JobMetadataSubstitutionValue>& substitutions() {
    return substitutions_;
  }
  void set_substitutions(
      std::map<std::string, JobMetadataSubstitutionValue> substitutions) {
    substitutions_ = std::move(substitutions);
  }

 private:
  std::map<std::string, JobMetadataSubstitutionValue> substitutions_;
};

}  // namespace dailyboy
