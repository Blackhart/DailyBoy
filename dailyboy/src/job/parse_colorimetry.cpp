/*!
 * \file parse_colorimetry.cpp
 * \brief Parse job YAML color section.
 */

#include "job/parse_colorimetry.hpp"

#include "job/yaml_read.hpp"

namespace dailyboy {

StatusOr<JobColorimetry> parse_colorimetry(const YAML::Node& node) {
  DAILYBOY_ASSIGN_OR_RETURN(const YAML::Node map, expect_map(node, "color"));
  JobColorimetry out;
  if (!map["ocio_config"]) {
    return Status::User(with_job_error(USER_ERROR_JOB_17));
  }
  DAILYBOY_ASSIGN_OR_RETURN(
      std::string ocio_config,
      read_quoted_nonempty_string(map["ocio_config"], USER_ERROR_JOB_48,
                                  "color.ocio_config"));
  out.set_ocio_config(std::move(ocio_config));

  if (map["working_colorspace"]) {
    DAILYBOY_ASSIGN_OR_RETURN(std::string working_colorspace,
                              read_quoted_nonempty_string(
                                  map["working_colorspace"], USER_ERROR_JOB_49,
                                  "color.working_colorspace"));
    out.set_working_colorspace(std::move(working_colorspace));
  }

  const YAML::Node ctx_map = map["context"];
  if (ctx_map) {
    DAILYBOY_RETURN_IF_ERROR(expect_map(ctx_map, "color.context").status());
    std::map<std::string, std::string> context;
    DAILYBOY_ASSIGN_OR_RETURN(context, read_ocio_context(ctx_map));
    out.set_context(std::move(context));
  }
  return out;
}

}  // namespace dailyboy
