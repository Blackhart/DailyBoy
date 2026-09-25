#pragma once

#include <yaml-cpp/yaml.h>

#include "job/layout.hpp"
#include "job/primitives.hpp"
#include "status.hpp"

namespace dailyboy {

StatusOr<JobLayout> parse_layout(const YAML::Node& node);

/*!
 * \brief Parses an optional \c r/\c g/\c b map with channels in [0, 1].
 * \param node Color node; an absent or null node yields \a missing_channel.
 * \param field YAML path used in error messages.
 * \param missing_channel Value used for an absent block or channel.
 * \return User status when the node is not a map, holds a key other than
 *         \c r/\c g/\c b, or a channel is outside [0, 1].
 */
StatusOr<RGBColor> parse_rgb_color(const YAML::Node& node,
                                   const std::string& field,
                                   double missing_channel = 0.0);

}  // namespace dailyboy
