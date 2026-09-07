#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <utility>

namespace dailyboy {

/*!
 * \brief OCIO configuration and working space for the job pipeline.
 *
 * Corresponds to the \c color section in the job YAML.
 */
class JobColorimetry {
 public:
  JobColorimetry() = default;
  ~JobColorimetry() = default;

  const std::filesystem::path& ocio_config() const { return ocio_config_; }
  void set_ocio_config(std::filesystem::path ocio_config) {
    ocio_config_ = std::move(ocio_config);
  }

  const std::string& working_colorspace() const { return working_colorspace_; }
  void set_working_colorspace(std::string working_colorspace) {
    working_colorspace_ = std::move(working_colorspace);
  }

  const std::map<std::string, std::string>& context() const { return context_; }
  std::map<std::string, std::string>& context() { return context_; }
  void set_context(std::map<std::string, std::string> context) {
    context_ = std::move(context);
  }

 private:
  std::filesystem::path ocio_config_;
  std::string working_colorspace_;
  std::map<std::string, std::string> context_;
};

}  // namespace dailyboy
