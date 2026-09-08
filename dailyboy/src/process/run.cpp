/*!
 * \file run.cpp
 * \brief Daily: prepare color, open outputs, slates, plates, close.
 */

#include "process/run.hpp"

#include <dailyboy/log.hpp>

#include "process/burn_ins.hpp"
#include "process/colorimetry.hpp"
#include "process/output.hpp"
#include "process/slate.hpp"

namespace dailyboy {

Status run_job(const Job& job) {
  log_debug("run_job: ColorPipeline::prepare");
  DAILYBOY_ASSIGN_OR_RETURN(ColorPipeline color_pipeline,
                            ColorPipeline::prepare(job));

  log_debug("run_job: Outputs::open");
  DAILYBOY_ASSIGN_OR_RETURN(Outputs out, Outputs::open(job));

  log_debug("run_job: write_slates");
  Status slates = write_slates(job, out);
  if (!slates.ok()) {
    out.close();
    return slates;
  }

  log_debug("run_job: write_burnins");
  Status plates = write_burnins(job, out, color_pipeline);
  if (!plates.ok()) {
    out.close();
    return plates;
  }

  log_debug("run_job: Outputs::close");
  return out.close();
}

}  // namespace dailyboy
