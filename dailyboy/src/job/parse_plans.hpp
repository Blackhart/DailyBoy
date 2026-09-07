#pragma once

#include <yaml-cpp/yaml.h>

#include "job/plans.hpp"
#include "status.hpp"

namespace dailyboy {

StatusOr<JobPlans> parse_plans(const YAML::Node& node);

}  // namespace dailyboy
