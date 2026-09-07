#pragma once

#include <filesystem>
#include <map>
#include <string>

#include "job/metadata.hpp"

namespace dailyboy {

/*!
 * \brief Per-frame values for overlay \c {key} expansion.
 */
struct OverlayTokenContext {
  int frame = 0;
  int frame_start = 0;
  int frame_end = 0;
  std::filesystem::path source_file;
  std::string plan_id;
};

/*!
 * \brief Expands \c {key} tokens in burn-in / slate templates.
 *
 * Builtins (\c frame, \c frame_start, \c frame_end, \c source_file,
 * \c plan_id) win over a substitution of the same name. String
 * substitutions come next, then frame maps (miss yields an empty
 * string). Unknown keys stay as literal \c {key} and log a warning.
 *
 * \param input Template that may contain substitution tokens.
 * \param context Builtin values for this frame.
 * \param substitutions Job metadata substitutions.
 * \return Expanded text (never fails).
 */
std::string expand_overlay_tokens(
    const std::string& input, const OverlayTokenContext& context,
    const std::map<std::string, JobMetadataSubstitutionValue>& substitutions);

}  // namespace dailyboy
