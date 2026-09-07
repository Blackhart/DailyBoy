#pragma once

#include <fileseq/error.h>
#include <fileseq/sequence.h>
#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <cctype>
#include <charconv>
#include <map>
#include <nlohmann/json.hpp>
#include <regex>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "error/job.hpp"
#include "job/metadata.hpp"
#include "status.hpp"

namespace dailyboy {

using PathPart = std::variant<std::string, int>;
using SubstitutionValue =
    std::variant<std::string, std::map<std::string, std::string>>;

inline const std::regex kTokenRegex(R"(\{([a-zA-Z_][a-zA-Z0-9_]*)\})");

/*!
 * \brief Parses YAML boolean scalars written as strings.
 * \param text Scalar text from the YAML node.
 * \param out Parsed value when recognized.
 * \return \c true if \a text is a supported boolean literal.
 */
inline bool parse_bool_scalar(const std::string& text, bool& out) {
  if (text == "true" || text == "True" || text == "TRUE") {
    out = true;
    return true;
  }
  if (text == "false" || text == "False" || text == "FALSE") {
    out = false;
    return true;
  }
  return false;
}

/*!
 * \brief True when \a node is a quoted YAML string (\c "text" or \c 'text').
 */
inline bool is_yaml_quoted_string(const YAML::Node& node) {
  if (!node || !node.IsScalar()) {
    return false;
  }
  const std::string tag = node.Tag();
  return tag == "!!str" || tag == "!";
}

/*!
 * \brief True when \a node is a YAML string scalar, not int, bool, float, or
 *        null.
 */
inline bool is_yaml_string_scalar(const YAML::Node& node) {
  if (!node || !node.IsScalar()) {
    return false;
  }
  if (is_yaml_quoted_string(node)) {
    return true;
  }
  const std::string tag = node.Tag();
  if (tag == "!!int" || tag == "!!float" || tag == "!!bool" ||
      tag == "!!null") {
    return false;
  }
  const std::string scalar = node.as<std::string>();
  bool unused = false;
  if (parse_bool_scalar(scalar, unused)) {
    return false;
  }
  try {
    (void)node.as<long long>();
    return false;
  } catch (const YAML::Exception&) {
  }
  try {
    (void)node.as<double>();
    return false;
  } catch (const YAML::Exception&) {
  }
  return true;
}

/*!
 * \brief Short English description of a YAML node's type for user errors.
 */
inline std::string describe_yaml_value(const YAML::Node& node) {
  if (!node || node.IsNull()) {
    return "null";
  }
  if (node.IsSequence()) {
    return "a list";
  }
  if (node.IsMap()) {
    return node.size() == 0 ? "an empty map" : "a map";
  }
  if (!node.IsScalar()) {
    return "a value";
  }
  if (is_yaml_quoted_string(node)) {
    return "a string";
  }
  bool unused = false;
  if (parse_bool_scalar(node.as<std::string>(), unused)) {
    return "a boolean";
  }
  try {
    return "integer " + std::to_string(node.as<long long>());
  } catch (const YAML::Exception&) {
  }
  try {
    (void)node.as<double>();
    return "a number";
  } catch (const YAML::Exception&) {
  }
  return "unquoted string \"" + node.as<std::string>() + "\"";
}

/*!
 * \brief Builds a user/internal job error from a \c USER_ERROR_JOB_* /
 *        \c INTERNAL_ERROR_JOB_* constant, optionally appending a location.
 */
inline std::string with_job_error(std::string_view text) {
  return std::string(text);
}

inline std::string with_job_error(std::string_view text,
                                  const std::string& detail) {
  if (detail.empty()) {
    return std::string(text);
  }
  return std::string(text) + " " + detail;
}

/*!
 * \brief User error when a substitution value is not a quoted string or frame
 *        map.
 */
inline std::string substitution_value_error(const std::string& loc,
                                            const std::string& got) {
  return with_job_error(USER_ERROR_JOB_3, loc + " is " + got + ".");
}

inline bool json_pointer_token_is_digits(const std::string& part) {
  return !part.empty() &&
         std::all_of(part.begin(), part.end(),
                     [](unsigned char c) { return std::isdigit(c) != 0; });
}

inline bool substitution_key_needs_quotes(const std::string& key) {
  return json_pointer_token_is_digits(key) ||
         key.find_first_of(".[\"") != std::string::npos;
}

/*!
 * \brief YAML path for a \c metadata.substitutions value (quotes digit keys).
 */
inline std::string format_substitution_value_loc(const std::string& key) {
  if (substitution_key_needs_quotes(key)) {
    return "metadata.substitutions[\"" + key + "\"]";
  }
  return "metadata.substitutions." + key;
}

inline std::string format_frame_map_value_loc(const std::string& sub_key,
                                              const std::string& frame_key) {
  return format_substitution_value_loc(sub_key) + "[\"" + frame_key + "\"]";
}

inline std::string format_context_value_loc(const std::string& key) {
  if (substitution_key_needs_quotes(key)) {
    return "color.context[\"" + key + "\"]";
  }
  return "color.context." + key;
}

/*!
 * \brief User error when a quoted non-empty YAML string is required.
 */
inline Status require_quoted_nonempty_string(const YAML::Node& node,
                                             std::string_view error_text,
                                             const std::string& loc) {
  if (!is_yaml_quoted_string(node)) {
    return Status::User(with_job_error(
        error_text, loc + " is " + describe_yaml_value(node) + "."));
  }
  if (node.as<std::string>().empty()) {
    return Status::User(with_job_error(error_text, loc + " is empty."));
  }
  return Status::Ok();
}

inline StatusOr<std::string> read_quoted_nonempty_string(
    const YAML::Node& node, std::string_view error_text,
    const std::string& loc) {
  DAILYBOY_RETURN_IF_ERROR(
      require_quoted_nonempty_string(node, error_text, loc));
  return node.as<std::string>();
}

/*!
 * \brief Reads \c color.context; keys and values must be quoted YAML strings.
 */
inline StatusOr<std::map<std::string, std::string>> read_ocio_context(
    const YAML::Node& node) {
  if (node.size() == 0) {
    return Status::User(with_job_error(USER_ERROR_JOB_51));
  }
  std::map<std::string, std::string> out;
  for (const auto& kv : node) {
    if (!is_yaml_quoted_string(kv.first)) {
      std::string detail = "got " + describe_yaml_value(kv.first) + ".";
      if (kv.first && kv.first.IsScalar()) {
        detail = "'" + kv.first.as<std::string>() + "' is " +
                 describe_yaml_value(kv.first) + ".";
      }
      return Status::User(with_job_error(USER_ERROR_JOB_52, detail));
    }
    const std::string key = kv.first.as<std::string>();
    if (!is_yaml_quoted_string(kv.second)) {
      return Status::User(with_job_error(
          USER_ERROR_JOB_53, format_context_value_loc(key) + " is " +
                                 describe_yaml_value(kv.second) + "."));
    }
    out[key] = kv.second.as<std::string>();
  }
  return out;
}

/*!
 * \brief User error when a frame-map key is not a YAML string.
 */
inline std::string frame_map_key_error(const std::string& sub_key,
                                       const YAML::Node& key) {
  std::string detail = format_substitution_value_loc(sub_key) + ".";
  if (key && key.IsScalar()) {
    detail = format_substitution_value_loc(sub_key) + ": '" +
             key.as<std::string>() + "' is " + describe_yaml_value(key) + ".";
  }
  return with_job_error(USER_ERROR_JOB_45, detail);
}

/*!
 * \brief User error when a frame-map value is not a quoted YAML string.
 */
inline std::string frame_map_value_error(const std::string& sub_key,
                                         const std::string& frame_key,
                                         const std::string& got) {
  return with_job_error(
      USER_ERROR_JOB_46,
      format_frame_map_value_loc(sub_key, frame_key) + " is " + got + ".");
}

inline bool parse_frame_number(std::string_view text, int& value) {
  if (text.empty()) {
    return false;
  }
  const char* first = text.data();
  const char* last = first + text.size();
  const auto parsed = std::from_chars(first, last, value);
  return parsed.ec == std::errc() && parsed.ptr == last;
}

/*!
 * \brief Reads an unquoted YAML integer scalar.
 */
inline StatusOr<int> read_yaml_integer(const YAML::Node& node,
                                       std::string_view error_text,
                                       const std::string& loc) {
  int value = 0;
  if (!node || !node.IsScalar() || is_yaml_quoted_string(node) ||
      !parse_frame_number(node.as<std::string>(), value)) {
    return Status::User(with_job_error(
        error_text, loc + " is " + describe_yaml_value(node) + "."));
  }
  return value;
}

/*!
 * \brief Reads an unquoted YAML number scalar (int or float).
 */
inline StatusOr<double> read_yaml_number(const YAML::Node& node,
                                         std::string_view error_text,
                                         const std::string& loc) {
  if (!node || !node.IsScalar() || is_yaml_quoted_string(node)) {
    return Status::User(with_job_error(
        error_text, loc + " is " + describe_yaml_value(node) + "."));
  }
  bool unused = false;
  if (parse_bool_scalar(node.as<std::string>(), unused)) {
    return Status::User(with_job_error(error_text, loc + " is a boolean."));
  }
  try {
    return node.as<double>();
  } catch (const YAML::Exception&) {
    return Status::User(with_job_error(
        error_text, loc + " is " + describe_yaml_value(node) + "."));
  }
}

/*!
 * \brief True when \a key is a frame number or inclusive range (\c 1001 or
 *        \c 1005-1010).
 */
inline bool is_frame_map_key(const std::string& key) {
  int frame = 0;
  if (parse_frame_number(key, frame)) {
    return true;
  }
  const std::size_t dash = key.find('-');
  if (dash == std::string::npos || dash == 0 || dash + 1 >= key.size() ||
      key.find('-', dash + 1) != std::string::npos) {
    return false;
  }
  int start = 0;
  int end = 0;
  if (!parse_frame_number(std::string_view(key).substr(0, dash), start) ||
      !parse_frame_number(std::string_view(key).substr(dash + 1), end)) {
    return false;
  }
  return start <= end;
}

/*!
 * \brief User error when a frame-map key is not a frame or range.
 */
inline std::string frame_map_key_shape_error(const std::string& sub_key,
                                             const std::string& frame_key) {
  return with_job_error(USER_ERROR_JOB_47,
                        format_substitution_value_loc(sub_key) + ": '" +
                            frame_key + "' is not a frame or range.");
}

/*!
 * \brief Substitution name when \a yaml_path is a single substitutions entry.
 *
 * Matches \c metadata.substitutions.episode and \c metadata.substitutions["1"].
 * Nested paths (frame-map keys) return empty.
 */
inline std::string substitution_value_key_from_yaml_path(
    const std::string& yaml_path) {
  const std::string dotted = "metadata.substitutions.";
  if (yaml_path.compare(0, dotted.size(), dotted) == 0) {
    const std::string rest = yaml_path.substr(dotted.size());
    if (!rest.empty() && rest.find('.') == std::string::npos &&
        rest.find('[') == std::string::npos) {
      return rest;
    }
    return {};
  }
  const std::string quoted = "metadata.substitutions[\"";
  if (yaml_path.size() >= quoted.size() + 2 &&
      yaml_path.compare(0, quoted.size(), quoted) == 0 &&
      yaml_path[yaml_path.size() - 2] == '"' && yaml_path.back() == ']') {
    return yaml_path.substr(quoted.size(),
                            yaml_path.size() - quoted.size() - 2);
  }
  return {};
}

/*!
 * \brief Parses \c metadata.substitutions.<name>.<frame> from a YAML path.
 * \return \c true when \a yaml_path is a frame-map entry (not the map itself).
 */
inline bool parse_frame_map_entry_yaml_path(const std::string& yaml_path,
                                            std::string& sub_name,
                                            std::string& frame_key) {
  const std::string prefix = "metadata.substitutions.";
  if (yaml_path.compare(0, prefix.size(), prefix) != 0) {
    return false;
  }
  const std::string rest = yaml_path.substr(prefix.size());
  const std::size_t bracket = rest.find("[\"");
  if (bracket != std::string::npos && rest.size() >= bracket + 4 &&
      rest[rest.size() - 2] == '"' && rest.back() == ']') {
    sub_name = rest.substr(0, bracket);
    frame_key = rest.substr(bracket + 2, rest.size() - bracket - 4);
    return !sub_name.empty() && sub_name.find('.') == std::string::npos &&
           !frame_key.empty();
  }
  const std::size_t dot = rest.find('.');
  if (dot == std::string::npos || rest.find('[') != std::string::npos) {
    return false;
  }
  sub_name = rest.substr(0, dot);
  frame_key = rest.substr(dot + 1);
  return !sub_name.empty() && !frame_key.empty() &&
         frame_key.find('.') == std::string::npos;
}

/*!
 * \brief User error when a substitution name is not a YAML string.
 */
inline std::string substitution_name_error(const YAML::Node& key) {
  std::string detail = "got " + describe_yaml_value(key) + ".";
  if (key && key.IsScalar()) {
    detail =
        "'" + key.as<std::string>() + "' is " + describe_yaml_value(key) + ".";
  }
  return with_job_error(USER_ERROR_JOB_2, detail);
}

/*!
 * \brief Returns whether a YAML path points to a burn-in template or slate text.
 */
inline bool is_text_template_path(const std::vector<PathPart>& path) {
  if (path.size() < 4) {
    return false;
  }
  const auto* last = std::get_if<std::string>(&path[path.size() - 1]);
  if (last == nullptr) {
    return false;
  }
  if (*last == "template") {
    const auto* burn_ins = std::get_if<std::string>(&path[path.size() - 3]);
    const auto* idx = std::get_if<int>(&path[path.size() - 2]);
    return burn_ins != nullptr && *burn_ins == "burn_ins" && idx != nullptr;
  }
  if (*last == "text") {
    const auto* lines = std::get_if<std::string>(&path[path.size() - 3]);
    const auto* slate = std::get_if<std::string>(&path[path.size() - 4]);
    const auto* idx = std::get_if<int>(&path[path.size() - 2]);
    return lines != nullptr && *lines == "lines" && slate != nullptr &&
           *slate == "slate" && idx != nullptr;
  }
  return false;
}

inline std::string path_to_string(const std::vector<PathPart>& path) {
  std::string out;
  for (std::size_t i = 0; i < path.size(); ++i) {
    if (i > 0) {
      out += "/";
    }
    if (const auto* key = std::get_if<std::string>(&path[i])) {
      out += *key;
    } else if (const auto* idx = std::get_if<int>(&path[i])) {
      out += std::to_string(*idx);
    }
  }
  return out;
}

/*!
 * \brief Walks the job YAML tree and records \c {token} usages by location.
 */
inline void collect_token_usages(
    const YAML::Node& value, std::vector<PathPart> path,
    std::map<std::string, std::vector<std::pair<bool, std::string>>>& usages) {
  if (!value || value.IsNull()) {
    return;
  }
  if (value.IsMap()) {
    for (const auto& kv : value) {
      auto next = path;
      next.emplace_back(kv.first.as<std::string>());
      collect_token_usages(kv.second, std::move(next), usages);
    }
    return;
  }
  if (value.IsSequence()) {
    for (std::size_t i = 0; i < value.size(); ++i) {
      auto next = path;
      next.emplace_back(static_cast<int>(i));
      collect_token_usages(value[i], std::move(next), usages);
    }
    return;
  }
  if (!value.IsScalar()) {
    return;
  }

  const bool is_text = is_text_template_path(path);
  const std::string loc = path_to_string(path);
  const std::string text = value.as<std::string>();
  for (std::sregex_iterator it(text.begin(), text.end(), kTokenRegex), end;
       it != end; ++it) {
    usages[(*it)[1].str()].push_back({is_text, loc});
  }
}

/*!
 * \brief Reads one frame map; keys must be YAML strings and values quoted
 *        YAML strings.
 */
inline StatusOr<std::map<std::string, std::string>> read_frame_map(
    const YAML::Node& node, const std::string& sub_key) {
  std::map<std::string, std::string> frame_map;
  for (const auto& kv : node) {
    if (!is_yaml_string_scalar(kv.first)) {
      return Status::User(frame_map_key_error(sub_key, kv.first));
    }
    const std::string frame_key = kv.first.as<std::string>();
    if (!is_frame_map_key(frame_key)) {
      return Status::User(frame_map_key_shape_error(sub_key, frame_key));
    }
    if (!kv.second || !kv.second.IsScalar() ||
        !is_yaml_quoted_string(kv.second)) {
      return Status::User(frame_map_value_error(
          sub_key, frame_key, describe_yaml_value(kv.second)));
    }
    frame_map[frame_key] = kv.second.as<std::string>();
  }
  return frame_map;
}

/*!
 * \brief Reads \c metadata.substitutions from a YAML node.
 * \return User status if the node is not a map or a value has an invalid type.
 */
inline StatusOr<std::map<std::string, SubstitutionValue>> read_substitutions(
    const YAML::Node& subs) {
  if (!subs) {
    return std::map<std::string, SubstitutionValue>{};
  }
  if (!subs.IsMap()) {
    return Status::User(with_job_error(USER_ERROR_JOB_4));
  }

  std::map<std::string, SubstitutionValue> out;
  for (const auto& it : subs) {
    if (!is_yaml_string_scalar(it.first)) {
      return Status::User(substitution_name_error(it.first));
    }
    const std::string key = it.first.as<std::string>();
    const YAML::Node value = it.second;
    if (value.IsScalar()) {
      if (!is_yaml_quoted_string(value)) {
        return Status::User(substitution_value_error(
            format_substitution_value_loc(key), describe_yaml_value(value)));
      }
      out[key] = value.as<std::string>();
    } else if (value.IsMap() && value.size() > 0) {
      std::map<std::string, std::string> frame_map;
      DAILYBOY_ASSIGN_OR_RETURN(frame_map, read_frame_map(value, key));
      out[key] = std::move(frame_map);
    } else {
      return Status::User(substitution_value_error(
          format_substitution_value_loc(key), describe_yaml_value(value)));
    }
  }
  return out;
}

/*!
 * \brief Expands \c {key} tokens using string substitutions only.
 *
 * Frame-map substitutions are left as literal \c {key} placeholders.
 */
inline std::string expand_tokens_keep_frame_maps(
    const std::string& input,
    const std::map<std::string, SubstitutionValue>& substitutions) {
  std::string out;
  std::size_t last = 0;
  for (std::sregex_iterator it(input.begin(), input.end(), kTokenRegex), end;
       it != end; ++it) {
    const auto& m = *it;
    out.append(input, last, static_cast<std::size_t>(m.position()) - last);
    const std::string key = m[1].str();
    auto found = substitutions.find(key);
    if (found == substitutions.end() ||
        std::holds_alternative<std::map<std::string, std::string>>(
            found->second)) {
      out += m.str();
    } else {
      out += std::get<std::string>(found->second);
    }
    last = static_cast<std::size_t>(m.position() + m.length());
  }
  out.append(input, last, std::string::npos);
  return out;
}

/*!
 * \brief Ensures frame-map substitutions appear only in burn-in / slate text.
 * \return User status when a frame map is referenced elsewhere in the job.
 */
inline Status validate_substitution_usage(
    const YAML::Node& job_root,
    const std::map<std::string, SubstitutionValue>& substitutions) {
  std::map<std::string, std::vector<std::pair<bool, std::string>>> usages;
  collect_token_usages(job_root, {}, usages);

  for (const auto& it : substitutions) {
    if (!std::holds_alternative<std::map<std::string, std::string>>(
            it.second)) {
      continue;
    }
    auto use_it = usages.find(it.first);
    if (use_it == usages.end()) {
      continue;
    }
    for (const auto& usage : use_it->second) {
      if (!usage.first) {
        return Status::User(
            with_job_error(USER_ERROR_JOB_7,
                           "{" + it.first + "} used at " + usage.second + "."));
      }
    }
  }
  return Status::Ok();
}

/*!
 * \brief Validates a file sequence pattern with libfileseq.
 * \return User status when the pattern is empty or invalid.
 */
inline Status validate_fileseq_pattern(const std::string& pattern,
                                       const std::string& field) {
  if (pattern.empty()) {
    return Status::User(with_job_error(USER_ERROR_JOB_8, field + "."));
  }
  fileseq::Status status;
  fileseq::FileSequence seq(pattern, &status);
  if (!status || !seq.isValid()) {
    return Status::User(
        with_job_error(USER_ERROR_JOB_9, field + ": " + pattern));
  }
  return Status::Ok();
}

/*!
 * \brief Validates plan and output image-sequence patterns after token expansion.
 */
inline Status validate_fileseq_patterns(
    const YAML::Node& job_root,
    const std::map<std::string, SubstitutionValue>& substitutions) {
  const YAML::Node plans = job_root["plans"];
  if (plans && plans.IsSequence()) {
    for (std::size_t i = 0; i < plans.size(); ++i) {
      const YAML::Node pat = plans[i]["sequence"]["path"];
      if (pat && pat.IsScalar()) {
        DAILYBOY_RETURN_IF_ERROR(validate_fileseq_pattern(
            expand_tokens_keep_frame_maps(pat.as<std::string>(), substitutions),
            "plans[" + std::to_string(i) + "].sequence.path"));
      }
    }
  }

  const YAML::Node image_sequences = job_root["output"]["image_sequences"];
  if (image_sequences && image_sequences.IsSequence()) {
    for (std::size_t i = 0; i < image_sequences.size(); ++i) {
      const YAML::Node pat = image_sequences[i]["path_pattern"];
      if (pat && pat.IsScalar()) {
        DAILYBOY_RETURN_IF_ERROR(validate_fileseq_pattern(
            expand_tokens_keep_frame_maps(pat.as<std::string>(), substitutions),
            "output.image_sequences[" + std::to_string(i) + "].path_pattern"));
      }
    }
  }
  return Status::Ok();
}

/*!
 * \brief Converts a YAML subtree to JSON for JSON Schema validation.
 *
 * Preserves quoted / \c !!str scalars as strings so numeric-looking values
 * such as \c "103" are not coerced to JSON numbers.
 */
inline nlohmann::json yaml_to_json(const YAML::Node& node) {
  if (!node || node.IsNull()) {
    return nullptr;
  }
  if (node.IsScalar()) {
    const std::string tag = node.Tag();
    if (tag == "!!str" || tag == "!") {
      return node.as<std::string>();
    }

    const std::string scalar = node.as<std::string>();
    bool bool_value = false;
    if (parse_bool_scalar(scalar, bool_value)) {
      return bool_value;
    }
    try {
      return node.as<long long>();
    } catch (const YAML::Exception&) {
    }
    try {
      return node.as<double>();
    } catch (const YAML::Exception&) {
    }
    return scalar;
  }
  if (node.IsSequence()) {
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& item : node) {
      arr.push_back(yaml_to_json(item));
    }
    return arr;
  }
  if (node.IsMap()) {
    nlohmann::json obj = nlohmann::json::object();
    for (const auto& kv : node) {
      obj[kv.first.as<std::string>()] = yaml_to_json(kv.second);
    }
    return obj;
  }
  return nullptr;
}

inline Status type_error(const std::string& field,
                         const std::string& expected) {
  return Status::User(with_job_error(USER_ERROR_JOB_10,
                                     field + " (expected " + expected + ")."));
}

inline StatusOr<YAML::Node> expect_map(const YAML::Node& node,
                                       const std::string& field) {
  if (!node || !node.IsMap()) {
    return type_error(field, "map");
  }
  return node;
}

inline StatusOr<YAML::Node> expect_sequence(const YAML::Node& node,
                                            const std::string& field) {
  if (!node || !node.IsSequence()) {
    return type_error(field, "sequence");
  }
  return node;
}

inline StatusOr<YAML::Node> optional_sequence(const YAML::Node& node,
                                              const std::string& field) {
  if (!node || node.IsNull()) {
    return YAML::Node(YAML::NodeType::Sequence);
  }
  return expect_sequence(node, field);
}

template <typename T>
inline StatusOr<T> as_required(const YAML::Node& map, const std::string& key,
                               const std::string& field) {
  const YAML::Node value = map[key];
  if (!value) {
    return Status::User(
        with_job_error(USER_ERROR_JOB_11, field + " '" + key + "'."));
  }
  try {
    return value.as<T>();
  } catch (const YAML::Exception& ex) {
    return Status::User(with_job_error(
        USER_ERROR_JOB_18, field + "." + key + " (" + ex.what() + ")."));
  }
}

template <typename T>
inline StatusOr<T> as_optional(const YAML::Node& map, const std::string& key,
                               const T& fallback, const std::string& field) {
  const YAML::Node value = map[key];
  if (!value) {
    return fallback;
  }
  try {
    return value.as<T>();
  } catch (const YAML::Exception& ex) {
    return Status::User(with_job_error(
        USER_ERROR_JOB_18, field + "." + key + " (" + ex.what() + ")."));
  }
}

}  // namespace dailyboy
