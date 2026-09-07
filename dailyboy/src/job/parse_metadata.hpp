#pragma once

#include <yaml-cpp/yaml.h>

#include "job/metadata.hpp"
#include "status.hpp"

namespace dailyboy {

StatusOr<JobMetadata> parse_metadata(const YAML::Node& node);

}  // namespace dailyboy
