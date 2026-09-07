#include <dailyboy/api/makeDaily.hpp>
#include <dailyboy/makeDaily.hpp>

namespace dailyboy::api {

int makeDaily(const std::string_view job_yaml_path) {
  return dailyboy::makeDaily(job_yaml_path);
}

}  // namespace dailyboy::api
