#pragma once

#include <yaml-cpp/yaml.h>

#include <filesystem>
#include <nlohmann/json.hpp>

#include "status.hpp"

namespace dailyboy {

nlohmann::json yaml_to_json(const YAML::Node& node);
Status validate_job_schema(const std::filesystem::path& job_path);

}  // namespace dailyboy
