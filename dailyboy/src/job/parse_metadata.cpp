/*!
 * \file parse_metadata.cpp
 * \brief Parse job YAML metadata.substitutions.
 */

#include "job/parse_metadata.hpp"

#include "job/yaml_read.hpp"

namespace dailyboy {

StatusOr<JobMetadata> parse_metadata(const YAML::Node& node) {
  JobMetadata out;
  if (!node || node.IsNull()) {
    return out;
  }
  DAILYBOY_ASSIGN_OR_RETURN(const YAML::Node map, expect_map(node, "metadata"));
  const YAML::Node sub_map = map["substitutions"];
  if (!sub_map) {
    return Status::User(with_job_error(USER_ERROR_JOB_5));
  }
  DAILYBOY_RETURN_IF_ERROR(
      expect_map(sub_map, "metadata.substitutions").status());
  if (sub_map.size() == 0) {
    return Status::User(with_job_error(USER_ERROR_JOB_6));
  }

  std::map<std::string, JobMetadataSubstitutionValue> substitutions;
  for (const auto& it : sub_map) {
    if (!is_yaml_string_scalar(it.first)) {
      return Status::User(substitution_name_error(it.first));
    }
    const std::string key = it.first.as<std::string>();
    const YAML::Node value = it.second;
    if (value.IsScalar()) {
      if (!is_yaml_quoted_string(value)) {
        return Status::User(substitution_value_error(
            format_substitution_value_loc(key), describe_yaml_value(value)));
      }
      substitutions[key] = value.as<std::string>();
      continue;
    }
    if (!value.IsMap()) {
      return Status::User(substitution_value_error(
          format_substitution_value_loc(key), describe_yaml_value(value)));
    }
    std::map<std::string, std::string> per_frame;
    DAILYBOY_ASSIGN_OR_RETURN(per_frame, read_frame_map(value, key));
    substitutions[key] = std::move(per_frame);
  }
  out.set_substitutions(std::move(substitutions));
  return out;
}

}  // namespace dailyboy
