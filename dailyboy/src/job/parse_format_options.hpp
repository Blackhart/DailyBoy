#pragma once

#include <yaml-cpp/yaml.h>

#include <string>

#include "job/output.hpp"
#include "status.hpp"

namespace dailyboy {

/*!
 * \brief Infers writer format from a path pattern extension.
 *
 * \return png/jpeg/tiff/exr/heif label, or empty if unsupported.
 */
std::string image_sequence_format_from_path(const std::string& path_pattern);

/*!
 * \brief Parses \c format_options for the format implied by \a path_pattern.
 *
 * Missing node yields empty optional. Unknown extension with a present node
 * is a user error.
 */
StatusOr<std::optional<JobOutputImageSequenceFormatOptions>>
parse_image_sequence_format_options(const YAML::Node& node,
                                    const std::string& path_pattern,
                                    const std::string& field);

}  // namespace dailyboy
