#pragma once

#include <yaml-cpp/yaml.h>

#include "job/layout.hpp"
#include "job/primitives.hpp"
#include "status.hpp"

namespace dailyboy {

StatusOr<JobLayout> parse_layout(const YAML::Node& node);
StatusOr<RGBColor> parse_rgb_color(const YAML::Node& node,
                                   const std::string& field,
                                   double missing_channel = 0.0);

}  // namespace dailyboy
