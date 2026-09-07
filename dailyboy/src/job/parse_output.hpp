#pragma once

#include <yaml-cpp/yaml.h>

#include "job/output.hpp"
#include "status.hpp"

namespace dailyboy {

StatusOr<JobOutput> parse_output(const YAML::Node& node);

}  // namespace dailyboy
