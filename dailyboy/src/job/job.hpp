#pragma once

#include <utility>

#include "job/colorimetry.hpp"
#include "job/layout.hpp"
#include "job/metadata.hpp"
#include "job/output.hpp"
#include "job/plans.hpp"

namespace dailyboy {

/*!
 * \brief Root in-memory representation of a DailyBoy job YAML document.
 *
 * Aggregates metadata, colorimetry, layout, output deliverables, and source
 * plans. Populated by load_job().
 */
class Job {
 public:
  Job() = default;
  int dailyboy_version() const { return dailyboy_version_; }
  void set_dailyboy_version(int dailyboy_version) {
    dailyboy_version_ = dailyboy_version;
  }

  const JobMetadata& metadata() const { return metadata_; }
  JobMetadata& metadata() { return metadata_; }
  void set_metadata(JobMetadata metadata) { metadata_ = std::move(metadata); }

  const JobColorimetry& colorimetry() const { return colorimetry_; }
  JobColorimetry& colorimetry() { return colorimetry_; }
  void set_colorimetry(JobColorimetry colorimetry) {
    colorimetry_ = std::move(colorimetry);
  }

  const JobLayout& layout() const { return layout_; }
  JobLayout& layout() { return layout_; }
  void set_layout(JobLayout layout) { layout_ = std::move(layout); }

  const JobOutput& output() const { return output_; }
  JobOutput& output() { return output_; }
  void set_output(JobOutput output) { output_ = std::move(output); }

  const JobPlans& plans() const { return plans_; }
  JobPlans& plans() { return plans_; }
  void set_plans(JobPlans plans) { plans_ = std::move(plans); }

 private:
  int dailyboy_version_ = 0;
  JobMetadata metadata_;
  JobColorimetry colorimetry_;
  JobLayout layout_;
  JobOutput output_;
  JobPlans plans_;
};

}  // namespace dailyboy
