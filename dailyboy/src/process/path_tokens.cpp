/*!
 * \file path_tokens.cpp
 * \brief String substitution expansion for file paths (rejects frame maps).
 */

#include "process/path_tokens.hpp"

#include <regex>
#include <string>
#include <variant>

namespace dailyboy {

namespace {

const std::regex kTokenRegex(R"(\{([a-zA-Z_][a-zA-Z0-9_]*)\})");

}  // namespace

StatusOr<std::string> expand_path_tokens(
    const std::string& input,
    const std::map<std::string, JobMetadataSubstitutionValue>& substitutions,
    std::string_view frame_map_error) {
  std::string out;
  std::size_t last = 0;
  for (std::sregex_iterator it(input.begin(), input.end(), kTokenRegex), end;
       it != end; ++it) {
    const auto& match = *it;
    out.append(input, last, static_cast<std::size_t>(match.position()) - last);
    const std::string key = match[1].str();
    const auto found = substitutions.find(key);
    if (found == substitutions.end()) {
      out += match.str();
    } else if (std::holds_alternative<std::map<std::string, std::string>>(
                   found->second)) {
      return Status::User(std::string(frame_map_error));
    } else {
      out += std::get<std::string>(found->second);
    }
    last = static_cast<std::size_t>(match.position() + match.length());
  }
  out.append(input, last, std::string::npos);
  return out;
}

}  // namespace dailyboy
