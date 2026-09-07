/*!
 * \file tokens.cpp
 * \brief Overlay {key} expansion (builtins, strings, frame maps).
 */

#include "process/tokens.hpp"

#include <charconv>
#include <dailyboy/log.hpp>
#include <optional>
#include <regex>
#include <string>
#include <string_view>
#include <system_error>
#include <variant>

namespace dailyboy {

namespace {

const std::regex kTokenRegex(R"(\{([a-zA-Z_][a-zA-Z0-9_]*)\})");

bool parse_int(std::string_view text, int& value) {
  if (text.empty()) {
    return false;
  }
  const char* first = text.data();
  const char* last = first + text.size();
  const auto parsed = std::from_chars(first, last, value);
  return parsed.ec == std::errc() && parsed.ptr == last;
}

bool range_covers_frame(const std::string& key, int frame) {
  const std::size_t dash = key.find('-');
  if (dash == std::string::npos || dash == 0 || dash + 1 >= key.size()) {
    return false;
  }
  int start = 0;
  int end = 0;
  if (!parse_int(std::string_view(key).substr(0, dash), start) ||
      !parse_int(std::string_view(key).substr(dash + 1), end)) {
    return false;
  }
  return frame >= start && frame <= end;
}

std::string lookup_frame_map(const std::map<std::string, std::string>& map,
                             int frame) {
  const std::string exact = std::to_string(frame);
  const auto found = map.find(exact);
  if (found != map.end()) {
    return found->second;
  }
  for (const auto& [key, value] : map) {
    if (range_covers_frame(key, frame)) {
      return value;
    }
  }
  return {};
}

std::optional<std::string> lookup_builtin(const std::string& key,
                                          const OverlayTokenContext& context) {
  if (key == "frame") {
    return std::to_string(context.frame);
  }
  if (key == "frame_start") {
    return std::to_string(context.frame_start);
  }
  if (key == "frame_end") {
    return std::to_string(context.frame_end);
  }
  if (key == "source_file") {
    return context.source_file.string();
  }
  if (key == "plan_id") {
    return context.plan_id;
  }
  return std::nullopt;
}

std::optional<std::string> lookup_substitution(
    const std::string& key, int frame,
    const std::map<std::string, JobMetadataSubstitutionValue>& substitutions) {
  const auto found = substitutions.find(key);
  if (found == substitutions.end()) {
    return std::nullopt;
  }
  if (std::holds_alternative<std::string>(found->second)) {
    return std::get<std::string>(found->second);
  }
  return lookup_frame_map(
      std::get<std::map<std::string, std::string>>(found->second), frame);
}

}  // namespace

std::string expand_overlay_tokens(
    const std::string& input, const OverlayTokenContext& context,
    const std::map<std::string, JobMetadataSubstitutionValue>& substitutions) {
  std::string out;
  std::size_t last = 0;
  for (std::sregex_iterator it(input.begin(), input.end(), kTokenRegex), end;
       it != end; ++it) {
    const auto& match = *it;
    out.append(input, last, static_cast<std::size_t>(match.position()) - last);
    const std::string key = match[1].str();
    if (const std::optional<std::string> builtin =
            lookup_builtin(key, context)) {
      out += *builtin;
    } else if (const std::optional<std::string> sub =
                   lookup_substitution(key, context.frame, substitutions)) {
      out += *sub;
    } else {
      log_warn("burn_in: unknown token {" + key + "}");
      out += match.str();
    }
    last = static_cast<std::size_t>(match.position() + match.length());
  }
  out.append(input, last, std::string::npos);
  return out;
}

}  // namespace dailyboy
