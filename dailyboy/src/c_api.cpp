#include <dailyboy/c_api.h>

#include <dailyboy/makeDaily.hpp>

namespace {

/*!
 * Forwards a C string to makeDaily. A NULL pointer becomes an empty path.
 */
int make_daily_from_c(const char* job_yaml_path) {
  if (job_yaml_path == nullptr) {
    return dailyboy::makeDaily({});
  }
  return dailyboy::makeDaily(job_yaml_path);
}

}  // namespace

extern "C" int dailyboy_make_daily(const char* job_yaml_path) {
  return make_daily_from_c(job_yaml_path);
}
