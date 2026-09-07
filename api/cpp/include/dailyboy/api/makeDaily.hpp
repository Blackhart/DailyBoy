#pragma once

#include <dailyboy/export.hpp>
#include <string_view>

namespace dailyboy::api {

/*!
 * \brief Public C++ API — delegates to the engine (\c dailyboy::makeDaily).
 * \param job_yaml_path Path to the job YAML file.
 * \return \c 0 on success, non-zero if the job cannot be loaded or parsed.
 */
DAILYBOY_API int makeDaily(std::string_view job_yaml_path);

}  // namespace dailyboy::api
