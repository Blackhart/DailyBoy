/*!
 * \file loader.cpp
 * \brief load_job glue: parse each job YAML section.
 */

#include "job/loader.hpp"

#include "job/parse_colorimetry.hpp"
#include "job/parse_layout.hpp"
#include "job/parse_metadata.hpp"
#include "job/parse_output.hpp"
#include "job/parse_plans.hpp"
#include "job/yaml_read.hpp"
#include "yaml/yml_loader.hpp"

namespace dailyboy {

StatusOr<Job> load_job(const std::filesystem::path& job_path) {
  YmlLoader loader;
  DAILYBOY_RETURN_IF_ERROR(loader.load(job_path));
  DAILYBOY_ASSIGN_OR_RETURN(const YAML::Node root,
                            expect_map(loader.root(), "job"));
  Job job;
  DAILYBOY_ASSIGN_OR_RETURN(int version,
                            as_required<int>(root, "dailyboy_version", "job"));
  DAILYBOY_ASSIGN_OR_RETURN(JobMetadata metadata,
                            parse_metadata(root["metadata"]));
  DAILYBOY_ASSIGN_OR_RETURN(JobColorimetry colorimetry,
                            parse_colorimetry(root["color"]));
  DAILYBOY_ASSIGN_OR_RETURN(JobLayout layout, parse_layout(root["layout"]));
  DAILYBOY_ASSIGN_OR_RETURN(JobOutput output, parse_output(root["output"]));
  DAILYBOY_ASSIGN_OR_RETURN(JobPlans plans, parse_plans(root["plans"]));
  job.set_dailyboy_version(version);
  job.set_metadata(std::move(metadata));
  job.set_colorimetry(std::move(colorimetry));
  job.set_layout(std::move(layout));
  job.set_output(std::move(output));
  job.set_plans(std::move(plans));
  return job;
}

}  // namespace dailyboy
