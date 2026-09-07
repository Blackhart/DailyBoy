#pragma once

#include <dailyboy/export.hpp>

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * \brief Stable C interface — delegates to dailyboy::makeDaily().
 * \param job_yaml_path Path to the job YAML file, or \c NULL (treated as empty).
 * \return \c 0 on success, non-zero on error.
 */
DAILYBOY_API int dailyboy_make_daily(const char* job_yaml_path);

#ifdef __cplusplus
}
#endif
