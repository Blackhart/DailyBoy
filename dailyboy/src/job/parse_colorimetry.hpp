#pragma once

#include <yaml-cpp/yaml.h>

#include "job/colorimetry.hpp"
#include "status.hpp"

namespace dailyboy {

StatusOr<JobColorimetry> parse_colorimetry(const YAML::Node& node);

}  // namespace dailyboy
