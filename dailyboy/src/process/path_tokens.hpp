#pragma once

#include <map>
#include <string>
#include <string_view>

#include "job/metadata.hpp"
#include "status.hpp"

namespace dailyboy {

/*!
 * \brief Expands \c {key} string substitutions in \a input.
 *
 * Unknown keys are left as literal \c {key}. A key whose value is a frame map
 * yields a user status with \a frame_map_error.
 *
 * \param input Path or pattern that may contain substitution tokens.
 * \param substitutions Job metadata substitutions.
 * \param frame_map_error Message returned when a frame-map token is used.
 * \return Expanded string, or user status when a frame-map token appears.
 */
StatusOr<std::string> expand_path_tokens(
    const std::string& input,
    const std::map<std::string, JobMetadataSubstitutionValue>& substitutions,
    std::string_view frame_map_error);

}  // namespace dailyboy
