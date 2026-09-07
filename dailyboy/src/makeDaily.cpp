#include <dailyboy/log.hpp>
#include <dailyboy/makeDaily.hpp>
#include <filesystem>
#include <string>

#include "job/loader.hpp"
#include "process/run.hpp"
#include "status.hpp"

namespace dailyboy {

/*!
 * Validates then loads the job, reformats source frames, composes them
 * onto \c layout.canvas, and writes each enabled \c output.videos[] MOV
 * and \c output.image_sequences[] file sequence. On failure, logs the
 * Status message and returns 1 so CLI, Python, and the C API share the
 * same exit convention.
 */
int makeDaily(const std::string_view job_yaml_path) {
  const std::filesystem::path job_path(job_yaml_path);
  const Status schema_status = validate_job_schema(job_path);
  if (!schema_status.ok()) {
    log_error(schema_status.message());
    return 1;
  }

  StatusOr<Job> job = load_job(job_path);
  if (!job.ok()) {
    log_error(job.status().message());
    return 1;
  }

  log_info("DailyBoy makeDaily — job: " + job_path.string());
  log_info("job dailyboy_version: " +
           std::to_string(job.value().dailyboy_version()));

  const Status render_status = run_job(job.value());
  if (!render_status.ok()) {
    log_error(render_status.message());
    return 1;
  }

  return 0;
}

}  // namespace dailyboy
