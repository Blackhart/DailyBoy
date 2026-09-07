#pragma once

#include <dailyboy/export.hpp>
#include <string_view>

namespace dailyboy {

/*!
 * \brief Main DailyBoy engine entry point (dailies rendering).
 * \param job_yaml_path Path to the job YAML file (`.yaml` / `.yml`).
 * \return \c 0 on success, non-zero if the job cannot be loaded or an
 *         enabled video or image sequence cannot be written. Source frames
 *         are reformatted and composed onto the job canvas before encode.
 */
DAILYBOY_API int makeDaily(std::string_view job_yaml_path);

}  // namespace dailyboy
