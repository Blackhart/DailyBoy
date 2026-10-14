/*!
 * \file schema.cpp
 * \brief JSON Schema plus DailyBoy contract checks for a job YAML file.
 */

#include "job/schema.hpp"

#include <fstream>
#include <map>
#include <nlohmann/json-schema.hpp>
#include <optional>
#include <regex>
#include <string>
#include <string_view>
#include <utility>

#include "error/job.hpp"
#include "job/parse_colorimetry.hpp"
#include "job/parse_layout.hpp"
#include "job/parse_output.hpp"
#include "job/parse_plans.hpp"
#include "job/yaml_read.hpp"
#include "yaml/yml_loader.hpp"

namespace dailyboy {

namespace {

bool is_layout_background_channel(const std::string& yaml_path) {
  return yaml_path == "layout.background.r" ||
         yaml_path == "layout.background.g" ||
         yaml_path == "layout.background.b";
}

/*!
 * \brief True when \a yaml_path is a \c layout.burn_ins list item.
 */
bool is_layout_burn_in_item_path(const std::string& yaml_path) {
  static const std::regex kBurnInItem(R"(^layout\.burn_ins\[\d+\]$)");
  return std::regex_match(yaml_path, kBurnInItem);
}

/*!
 * \brief True when \a yaml_path is \c layout.burn_ins[N].\a field.
 */
bool is_burn_in_yaml_field(const std::string& yaml_path,
                           std::string_view field) {
  const std::regex re(std::string(R"(^layout\.burn_ins\[\d+\]\.)") +
                      std::string(field) + "$");
  return std::regex_match(yaml_path, re);
}

/*!
 * \brief True when \a yaml_path is a \c layout.burn_ins[N].font.color channel.
 */
bool is_burn_in_font_color_channel(const std::string& yaml_path) {
  return is_burn_in_yaml_field(yaml_path, "font.color.r") ||
         is_burn_in_yaml_field(yaml_path, "font.color.g") ||
         is_burn_in_yaml_field(yaml_path, "font.color.b");
}

/*!
 * \brief True when \a yaml_path is a \c layout.burn_ins[N].box.color channel.
 */
bool is_burn_in_box_color_channel(const std::string& yaml_path) {
  return is_burn_in_yaml_field(yaml_path, "box.color.r") ||
         is_burn_in_yaml_field(yaml_path, "box.color.g") ||
         is_burn_in_yaml_field(yaml_path, "box.color.b");
}

/*!
 * \brief True when \a yaml_path is a \c layout.burn_ins[N].box.margin side.
 */
bool is_burn_in_box_margin_side(const std::string& yaml_path) {
  return is_burn_in_yaml_field(yaml_path, "box.margin.top") ||
         is_burn_in_yaml_field(yaml_path, "box.margin.right") ||
         is_burn_in_yaml_field(yaml_path, "box.margin.bottom") ||
         is_burn_in_yaml_field(yaml_path, "box.margin.left");
}

/*!
 * \brief True when \a yaml_path is a \c layout.slate.lines list item.
 */
bool is_layout_slate_line_item_path(const std::string& yaml_path) {
  static const std::regex kSlateLineItem(R"(^layout\.slate\.lines\[\d+\]$)");
  return std::regex_match(yaml_path, kSlateLineItem);
}

/*!
 * \brief True when \a yaml_path is \c layout.slate.lines[N].\a field.
 */
bool is_slate_line_yaml_field(const std::string& yaml_path,
                              std::string_view field) {
  const std::regex re(std::string(R"(^layout\.slate\.lines\[\d+\]\.)") +
                      std::string(field) + "$");
  return std::regex_match(yaml_path, re);
}

/*!
 * \brief True when \a field belongs to a burn-in or to a slate line.
 *
 * Burn-ins and slate lines share the position and font contracts, so both
 * report the same errors.
 */
bool is_text_overlay_yaml_field(const std::string& yaml_path,
                                std::string_view field) {
  return is_burn_in_yaml_field(yaml_path, field) ||
         is_slate_line_yaml_field(yaml_path, field);
}

/*!
 * \brief True when \a yaml_path is a \c plans list item.
 */
bool is_plan_item_path(const std::string& yaml_path) {
  static const std::regex kPlanItem(R"(^plans\[\d+\]$)");
  return std::regex_match(yaml_path, kPlanItem);
}

/*!
 * \brief True when \a yaml_path is \c plans[N].\a field.
 */
bool is_plan_yaml_field(const std::string& yaml_path, std::string_view field) {
  const std::regex re(std::string(R"(^plans\[\d+\]\.)") + std::string(field) +
                      "$");
  return std::regex_match(yaml_path, re);
}

/*!
 * \brief True when \a yaml_path is an \c output.videos list item.
 */
bool is_output_video_item_path(const std::string& yaml_path) {
  static const std::regex kVideoItem(R"(^output\.videos\[\d+\]$)");
  return std::regex_match(yaml_path, kVideoItem);
}

/*!
 * \brief True when \a yaml_path is \c output.videos[N].\a field.
 */
bool is_output_video_yaml_field(const std::string& yaml_path,
                                std::string_view field) {
  const std::regex re(std::string(R"(^output\.videos\[\d+\]\.)") +
                      std::string(field) + "$");
  return std::regex_match(yaml_path, re);
}

/*!
 * \brief True when \a yaml_path is an \c output.image_sequences list item.
 */
bool is_output_image_sequence_item_path(const std::string& yaml_path) {
  static const std::regex kItem(R"(^output\.image_sequences\[\d+\]$)");
  return std::regex_match(yaml_path, kItem);
}

/*!
 * \brief True when \a yaml_path is \c output.image_sequences[N].\a field.
 */
bool is_output_image_sequence_yaml_field(const std::string& yaml_path,
                                         std::string_view field) {
  const std::regex re(std::string(R"(^output\.image_sequences\[\d+\]\.)") +
                      std::string(field) + "$");
  return std::regex_match(yaml_path, re);
}

/*!
 * \brief Codec string for \c output.videos[N] from a nested field path.
 */
std::string output_video_codec_at(const nlohmann::json& root,
                                  const std::string& yaml_path) {
  static const std::regex kVideo(R"(^output\.videos\[(\d+)\](?:\.|$))");
  std::smatch match;
  if (!std::regex_search(yaml_path, match, kVideo) || !root.is_object()) {
    return {};
  }
  const auto output = root.find("output");
  if (output == root.end() || !output->is_object()) {
    return {};
  }
  const auto videos = output->find("videos");
  if (videos == output->end() || !videos->is_array()) {
    return {};
  }
  const std::size_t index = static_cast<std::size_t>(std::stoul(match[1]));
  if (index >= videos->size() || !(*videos)[index].is_object()) {
    return {};
  }
  const auto codec = (*videos)[index].find("codec");
  if (codec == (*videos)[index].end() || !codec->is_string()) {
    return {};
  }
  return codec->get<std::string>();
}

/*!
 * \brief True when \a yaml_path is a \c layout.image.min_margin_px side.
 */
bool is_layout_image_margin_side(const std::string& yaml_path) {
  return yaml_path == "layout.image.min_margin_px.top" ||
         yaml_path == "layout.image.min_margin_px.right" ||
         yaml_path == "layout.image.min_margin_px.bottom" ||
         yaml_path == "layout.image.min_margin_px.left";
}

/*!
 * \brief Reads one \c layout.background channel: unquoted number in [0, 1].
 */
Status validate_output_contract(const YAML::Node& job_root) {
  const YAML::Node output = job_root["output"];
  if (!output || !output.IsMap()) {
    return Status::Ok();
  }

  const bool has_videos_key = output["videos"] && !output["videos"].IsNull();
  const bool has_sequences_key =
      output["image_sequences"] && !output["image_sequences"].IsNull();
  if (!has_videos_key && !has_sequences_key) {
    return Status::User(with_job_error(USER_ERROR_JOB_39));
  }

  auto has_enabled = [](const YAML::Node& seq) -> bool {
    if (!seq || !seq.IsSequence()) {
      return false;
    }
    for (std::size_t i = 0; i < seq.size(); ++i) {
      const YAML::Node enabled = seq[i]["enabled"];
      if (!enabled) {
        continue;
      }
      try {
        if (enabled.as<bool>()) {
          return true;
        }
      } catch (const YAML::Exception&) {
        continue;
      }
    }
    return false;
  };
  if (!has_enabled(output["videos"]) &&
      !has_enabled(output["image_sequences"])) {
    return Status::User(with_job_error(USER_ERROR_JOB_36));
  }

  std::map<std::string, std::string> seen;
  auto visit = [&](const YAML::Node& seq, const char* prefix) -> Status {
    if (!seq || !seq.IsSequence()) {
      return Status::Ok();
    }
    for (std::size_t i = 0; i < seq.size(); ++i) {
      const YAML::Node id_node = seq[i]["id"];
      if (!id_node) {
        continue;
      }
      std::string id;
      try {
        id = id_node.as<std::string>();
      } catch (const YAML::Exception&) {
        continue;
      }
      const std::string field =
          std::string(prefix) + "[" + std::to_string(i) + "].id";
      const auto it = seen.find(id);
      if (it != seen.end()) {
        std::string detail = field;
        detail += " '";
        detail += id;
        detail += "' (already used by ";
        detail += it->second;
        detail += ").";
        return Status::User(with_job_error(USER_ERROR_JOB_35, detail));
      }
      seen.emplace(id, field);
    }
    return Status::Ok();
  };
  DAILYBOY_RETURN_IF_ERROR(visit(output["videos"], "output.videos"));
  DAILYBOY_RETURN_IF_ERROR(
      visit(output["image_sequences"], "output.image_sequences"));
  return Status::Ok();
}

/*!
 * \brief When \c plans[].timecode is set, enabled videos must share one fps.
 *
 * Also rejects \c drop_frame: true unless that fps is 30 or 60. Omitted video
 * \c fps counts as 24 (same default as the writer).
 */
Status validate_timecode_contract(const YAML::Node& job_root) {
  const YAML::Node plans = job_root["plans"];
  if (!plans || !plans.IsSequence()) {
    return Status::Ok();
  }
  bool has_timecode = false;
  bool drop_frame = false;
  for (std::size_t i = 0; i < plans.size(); ++i) {
    const YAML::Node timecode = plans[i]["timecode"];
    if (!timecode || !timecode.IsMap()) {
      continue;
    }
    has_timecode = true;
    if (timecode["drop_frame"]) {
      try {
        drop_frame = timecode["drop_frame"].as<bool>();
      } catch (const YAML::Exception&) {
        continue;
      }
    }
  }
  if (!has_timecode) {
    return Status::Ok();
  }

  constexpr int kDefaultFps = 24;
  std::optional<int> fps;
  const YAML::Node videos = job_root["output"]["videos"];
  if (videos && videos.IsSequence()) {
    for (std::size_t i = 0; i < videos.size(); ++i) {
      const YAML::Node enabled = videos[i]["enabled"];
      bool is_enabled = false;
      if (enabled) {
        try {
          is_enabled = enabled.as<bool>();
        } catch (const YAML::Exception&) {
          continue;
        }
      }
      if (!is_enabled) {
        continue;
      }
      int video_fps = kDefaultFps;
      if (videos[i]["fps"]) {
        try {
          video_fps = videos[i]["fps"].as<int>();
        } catch (const YAML::Exception&) {
          continue;
        }
      }
      if (fps.has_value() && *fps != video_fps) {
        return Status::User(std::string(USER_ERROR_JOB_104));
      }
      fps = video_fps;
    }
  }
  const int resolved = fps.value_or(kDefaultFps);
  if (drop_frame && resolved != 30 && resolved != 60) {
    return Status::User(std::string(USER_ERROR_JOB_103));
  }
  return Status::Ok();
}

std::string unescape_json_pointer_token(std::string part) {
  for (std::size_t p = 0; p + 1 < part.size();) {
    if (part[p] == '~' && part[p + 1] == '1') {
      part.replace(p, 2, "/");
    } else if (part[p] == '~' && part[p + 1] == '0') {
      part.replace(p, 2, "~");
    } else {
      ++p;
    }
  }
  return part;
}

bool yaml_path_names_array(const std::string& path) {
  return path == "plans" || path == "output.videos" ||
         path == "output.image_sequences" || path == "layout.burn_ins" ||
         path == "layout.slate.lines";
}

/*!
 * \brief Converts a JSON Schema pointer to a dotted YAML path (\c job at root).
 *
 * Digit tokens are array indices only under known list fields (\c plans,
 * \c videos, …). Elsewhere they are object keys, written \c ["1"].
 */
std::string json_pointer_to_yaml_path(const nlohmann::json::json_pointer& ptr) {
  const std::string raw = ptr.to_string();
  if (raw.empty()) {
    return "job";
  }
  std::string out;
  std::size_t i = 1;
  while (i <= raw.size()) {
    const std::size_t slash = raw.find('/', i);
    std::string part = unescape_json_pointer_token(raw.substr(
        i, slash == std::string::npos ? std::string::npos : slash - i));
    if (json_pointer_token_is_digits(part) && yaml_path_names_array(out)) {
      out += "[" + part + "]";
    } else if (json_pointer_token_is_digits(part)) {
      out += "[\"" + part + "\"]";
    } else {
      if (!out.empty()) {
        out += ".";
      }
      out += part;
    }
    if (slash == std::string::npos) {
      break;
    }
    i = slash + 1;
  }
  return out.empty() ? "job" : out;
}

/*!
 * \brief Short English description of a JSON value for user errors.
 */
std::string describe_json_value(const nlohmann::json& value) {
  if (value.is_null()) {
    return "null";
  }
  if (value.is_boolean()) {
    return value.get<bool>() ? "boolean true" : "boolean false";
  }
  if (value.is_number_integer()) {
    return "integer " + value.dump();
  }
  if (value.is_number()) {
    return "number " + value.dump();
  }
  if (value.is_string()) {
    const std::string text = value.get<std::string>();
    if (text.size() <= 40) {
      return "string \"" + text + "\"";
    }
    return "a string";
  }
  if (value.is_array()) {
    return "a list";
  }
  if (value.is_object()) {
    return "a map";
  }
  return "a value";
}

/*!
 * \brief Message for a missing key whose name alone identifies the block.
 * \return Empty string when \a key needs its parent path to be mapped.
 */
std::string missing_named_key_error(const std::string& /*path*/,
                                    const std::string& key) {
  static const std::map<std::string, std::string_view> kNamedKeys = {
      {"plans", USER_ERROR_JOB_12},
      {"color", USER_ERROR_JOB_13},
      {"layout", USER_ERROR_JOB_14},
      {"output", USER_ERROR_JOB_15},
      {"dailyboy_version", USER_ERROR_JOB_16},
      {"substitutions", USER_ERROR_JOB_5},
      {"ocio_config", USER_ERROR_JOB_17},
      {"canvas", USER_ERROR_JOB_54},
      {"image", USER_ERROR_JOB_62}};
  const auto it = kNamedKeys.find(key);
  return it == kNamedKeys.end() ? std::string() : with_job_error(it->second);
}

/*!
 * \brief Message for a missing \c layout.canvas or \c layout.image key.
 */
std::string missing_layout_key_error(const std::string& path,
                                     const std::string& key) {
  if (key == "fit" && path == "layout.image") {
    return with_job_error(USER_ERROR_JOB_64);
  }
  if (key == "width" && path == "layout.canvas") {
    return with_job_error(USER_ERROR_JOB_55);
  }
  if (key == "height" && path == "layout.canvas") {
    return with_job_error(USER_ERROR_JOB_57);
  }
  return {};
}

/*!
 * \brief Message for a missing \c layout.burn_ins[N] key.
 */
std::string missing_burn_in_key_error(const std::string& path,
                                      const std::string& key) {
  if (!is_layout_burn_in_item_path(path)) {
    return {};
  }
  if (key == "template") {
    return with_job_error(USER_ERROR_JOB_73);
  }
  if (key == "position") {
    return with_job_error(USER_ERROR_JOB_74);
  }
  if (key == "font") {
    return with_job_error(USER_ERROR_JOB_75);
  }
  return {};
}

/*!
 * \brief Message for a missing \c layout.slate key.
 */
std::string missing_slate_key_error(const std::string& path,
                                    const std::string& key) {
  if (path != "layout.slate") {
    return {};
  }
  if (key == "duration_frames") {
    return with_job_error(USER_ERROR_JOB_110);
  }
  if (key == "lines") {
    return with_job_error(USER_ERROR_JOB_113);
  }
  return {};
}

/*!
 * \brief Message for a missing \c layout.slate.lines[N] key.
 */
std::string missing_slate_line_key_error(const std::string& path,
                                         const std::string& key) {
  if (!is_layout_slate_line_item_path(path)) {
    return {};
  }
  if (key == "text") {
    return with_job_error(USER_ERROR_JOB_114);
  }
  if (key == "position") {
    return with_job_error(USER_ERROR_JOB_115);
  }
  if (key == "font") {
    return with_job_error(USER_ERROR_JOB_116);
  }
  return {};
}

/*!
 * \brief Message for a missing \c position or \c font key of a burn-in or of a
 *        slate line.
 */
std::string missing_text_overlay_key_error(const std::string& path,
                                           const std::string& key) {
  if (key == "mode" && is_text_overlay_yaml_field(path, "position")) {
    return with_job_error(USER_ERROR_JOB_136);
  }
  if (!is_text_overlay_yaml_field(path, "font")) {
    return {};
  }
  if (key == "path") {
    return with_job_error(USER_ERROR_JOB_137);
  }
  if (key == "size_px") {
    return with_job_error(USER_ERROR_JOB_138);
  }
  return {};
}

/*!
 * \brief Message for a missing \c plans[N] key.
 */
std::string missing_plan_key_error(const std::string& path,
                                   const std::string& key) {
  if (!is_plan_item_path(path)) {
    return {};
  }
  if (key == "id") {
    return with_job_error(USER_ERROR_JOB_118);
  }
  if (key == "input_colorspace") {
    return with_job_error(USER_ERROR_JOB_120);
  }
  if (key == "sequence") {
    return with_job_error(USER_ERROR_JOB_122);
  }
  return {};
}

/*!
 * \brief Message for a missing \c plans[N].sequence key.
 */
std::string missing_plan_sequence_key_error(const std::string& path,
                                            const std::string& key) {
  if (!is_plan_yaml_field(path, "sequence")) {
    return {};
  }
  if (key == "path") {
    return with_job_error(USER_ERROR_JOB_123);
  }
  if (key == "frame_start") {
    return with_job_error(USER_ERROR_JOB_125);
  }
  if (key == "frame_end") {
    return with_job_error(USER_ERROR_JOB_127);
  }
  return {};
}

/*!
 * \brief Message for a missing \c plans[N].audio or \c timecode key.
 */
std::string missing_plan_option_key_error(const std::string& path,
                                          const std::string& key) {
  if (key == "path" && is_plan_yaml_field(path, "audio")) {
    return with_job_error(USER_ERROR_JOB_129);
  }
  if (key == "start" && is_plan_yaml_field(path, "timecode")) {
    return with_job_error(USER_ERROR_JOB_101);
  }
  return {};
}

/*!
 * \brief Message for a missing \c output.videos[N] key.
 */
std::string missing_output_video_key_error(const std::string& path,
                                           const std::string& key) {
  if (!is_output_video_item_path(path)) {
    return {};
  }
  static const std::map<std::string, std::string_view> kKeys = {
      {"id", USER_ERROR_JOB_143},           {"enabled", USER_ERROR_JOB_145},
      {"display_view", USER_ERROR_JOB_147}, {"signal", USER_ERROR_JOB_92},
      {"path", USER_ERROR_JOB_165},         {"codec", USER_ERROR_JOB_169}};
  const auto it = kKeys.find(key);
  return it == kKeys.end() ? std::string() : with_job_error(it->second);
}

/*!
 * \brief Message for a missing \c output.videos[N].display_view key.
 */
std::string missing_output_video_display_view_key_error(
    const std::string& path, const std::string& key) {
  if (!is_output_video_yaml_field(path, "display_view")) {
    return {};
  }
  if (key == "display") {
    return with_job_error(USER_ERROR_JOB_148);
  }
  if (key == "view") {
    return with_job_error(USER_ERROR_JOB_150);
  }
  return {};
}

/*!
 * \brief Message for a missing \c output.videos[N].signal key.
 */
std::string missing_output_video_signal_key_error(const std::string& path,
                                                  const std::string& key) {
  if (!is_output_video_yaml_field(path, "signal")) {
    return {};
  }
  static const std::map<std::string, std::string_view> kKeys = {
      {"range", USER_ERROR_JOB_153},
      {"matrix", USER_ERROR_JOB_156},
      {"primaries", USER_ERROR_JOB_159},
      {"transfer", USER_ERROR_JOB_162}};
  const auto it = kKeys.find(key);
  return it == kKeys.end() ? std::string() : with_job_error(it->second);
}

/*!
 * \brief Message for a missing \c output.image_sequences[N] key.
 */
std::string missing_output_image_sequence_key_error(const std::string& path,
                                                    const std::string& key) {
  if (!is_output_image_sequence_item_path(path)) {
    return {};
  }
  static const std::map<std::string, std::string_view> kKeys = {
      {"id", USER_ERROR_JOB_204},
      {"enabled", USER_ERROR_JOB_206},
      {"display_view", USER_ERROR_JOB_208},
      {"path_pattern", USER_ERROR_JOB_213}};
  const auto it = kKeys.find(key);
  return it == kKeys.end() ? std::string() : with_job_error(it->second);
}

/*!
 * \brief Message for a missing \c output.image_sequences[N].display_view key.
 */
std::string missing_output_image_sequence_display_view_key_error(
    const std::string& path, const std::string& key) {
  if (!is_output_image_sequence_yaml_field(path, "display_view")) {
    return {};
  }
  if (key == "display") {
    return with_job_error(USER_ERROR_JOB_209);
  }
  if (key == "view") {
    return with_job_error(USER_ERROR_JOB_211);
  }
  return {};
}

std::string missing_required_key_error(const std::string& path,
                                       const std::string& key) {
  using Resolver = std::string (*)(const std::string&, const std::string&);
  static constexpr Resolver kResolvers[] = {
      &missing_named_key_error,
      &missing_layout_key_error,
      &missing_burn_in_key_error,
      &missing_slate_key_error,
      &missing_slate_line_key_error,
      &missing_text_overlay_key_error,
      &missing_plan_key_error,
      &missing_plan_sequence_key_error,
      &missing_plan_option_key_error,
      &missing_output_video_key_error,
      &missing_output_video_display_view_key_error,
      &missing_output_video_signal_key_error,
      &missing_output_image_sequence_key_error,
      &missing_output_image_sequence_display_view_key_error};
  for (Resolver resolve : kResolvers) {
    const std::string message = resolve(path, key);
    if (!message.empty()) {
      return message;
    }
  }
  if (key == "bitrate_kbps" &&
      is_output_video_yaml_field(path, "codec_options")) {
    return with_job_error(USER_ERROR_JOB_31);
  }
  return with_job_error(USER_ERROR_JOB_11, path + " '" + key + "'.");
}

/*!
 * \brief "\<path\> (got \<value\>)." detail used by numeric range errors.
 */
std::string got_detail(const std::string& yaml_path,
                       const nlohmann::json& instance) {
  return yaml_path + " (got " + describe_json_value(instance) + ").";
}

/*!
 * \brief "\<path\> is \<value\>." detail used by wrong-type errors.
 */
std::string is_detail(const std::string& yaml_path,
                      const nlohmann::json& instance) {
  return yaml_path + " is " + describe_json_value(instance) + ".";
}

/*!
 * \brief Message for one \c metadata.substitutions value.
 *
 * Reports the first offending frame-map entry when \a instance is a map, so a
 * malformed frame gets a precise location instead of the generic value error.
 */
std::string substitution_entry_error(const std::string& sub_key,
                                     const nlohmann::json& instance) {
  nlohmann::json got = instance;
  if (instance.is_object()) {
    const auto named = instance.find(sub_key);
    if (named != instance.end()) {
      got = *named;
    }
    const nlohmann::json& map = got.is_object() ? got : instance;
    for (auto& el : map.items()) {
      if (!el.value().is_string()) {
        return frame_map_value_error(sub_key, el.key(),
                                     describe_json_value(el.value()));
      }
    }
    for (auto& el : map.items()) {
      if (!is_frame_map_key(el.key())) {
        return frame_map_key_shape_error(sub_key, el.key());
      }
    }
  }
  return substitution_value_error(format_substitution_value_loc(sub_key),
                                  describe_json_value(got));
}

/*!
 * \brief Message for a \c metadata.substitutions entry or frame-map key.
 * \return Empty string when \a yaml_path is outside the substitutions block.
 */
std::string substitution_schema_error(const std::string& yaml_path,
                                      const nlohmann::json& instance,
                                      const std::string& /*message*/) {
  const std::string sub_key = substitution_value_key_from_yaml_path(yaml_path);
  if (!sub_key.empty()) {
    return substitution_entry_error(sub_key, instance);
  }
  std::string frame_sub;
  std::string frame_key;
  if (!parse_frame_map_entry_yaml_path(yaml_path, frame_sub, frame_key)) {
    return {};
  }
  nlohmann::json got = instance;
  if (instance.is_object()) {
    const auto it = instance.find(frame_key);
    if (it != instance.end()) {
      got = *it;
    }
  }
  return frame_map_value_error(frame_sub, frame_key, describe_json_value(got));
}

/*!
 * \brief Message for a \c required violation, or empty for other errors.
 */
std::string required_key_error(const std::string& yaml_path,
                               const nlohmann::json& /*instance*/,
                               const std::string& message) {
  const std::string mark = "required property '";
  const std::size_t req = message.find(mark);
  if (req == std::string::npos) {
    return {};
  }
  const std::size_t start = req + mark.size();
  const std::size_t end = message.find('\'', start);
  if (end == std::string::npos) {
    return {};
  }
  return missing_required_key_error(yaml_path,
                                    message.substr(start, end - start));
}

/*!
 * \brief Message for a list whose item count breaks the contract, or empty.
 */
std::string array_size_error(const std::string& yaml_path,
                             const nlohmann::json& instance,
                             const std::string& message) {
  const bool too_few = message == "array has too few items";
  if (!too_few && message != "array has too many items") {
    return {};
  }
  static const std::map<std::string, std::string_view> kTooFewItems = {
      {"layout.burn_ins", USER_ERROR_JOB_72},
      {"layout.slate.lines", USER_ERROR_JOB_113}};
  const std::size_t count = instance.is_array() ? instance.size() : 0;
  const std::string got = "(got " + std::to_string(count) + ").";
  const auto it = kTooFewItems.find(yaml_path);
  if (too_few && it != kTooFewItems.end()) {
    return with_job_error(it->second, got);
  }
  return yaml_path == "plans" ? with_job_error(USER_ERROR_JOB_37, got)
                              : std::string();
}

/*!
 * \brief Message for a map that must not be empty, or empty for other errors.
 */
std::string property_count_error(const std::string& yaml_path,
                                 const nlohmann::json& /*instance*/,
                                 const std::string& message) {
  if (message != "too few properties") {
    return {};
  }
  if (yaml_path == "metadata.substitutions") {
    return with_job_error(USER_ERROR_JOB_6);
  }
  if (yaml_path == "color.context") {
    return with_job_error(USER_ERROR_JOB_51);
  }
  if (is_output_image_sequence_yaml_field(yaml_path, "format_options")) {
    return with_job_error(USER_ERROR_JOB_215);
  }
  return {};
}

/*!
 * \brief Message for a \c dailyboy_version violation, or empty.
 *
 * A non-integer is reported apart from an integer other than 1.
 */
std::string version_constraint_error(const std::string& yaml_path,
                                     const nlohmann::json& instance,
                                     const std::string& message) {
  if (yaml_path != "dailyboy_version") {
    return {};
  }
  const std::string detail = "(got " + describe_json_value(instance) + ").";
  if (message == "instance not const") {
    return with_job_error(USER_ERROR_JOB_38, detail);
  }
  if (message == "unexpected instance type") {
    return with_job_error(USER_ERROR_JOB_131, detail);
  }
  return {};
}

/*!
 * \brief True when \a message reports an \c exclusiveMinimum violation.
 */
bool is_exclusive_minimum_violation(const std::string& message) {
  return message.find("exclusiveMinimum") != std::string::npos ||
         message.find("below or equals minimum") != std::string::npos;
}

/*!
 * \brief Message for a burn-in \c box value outside its range, or empty.
 */
std::string burn_in_box_range_error(const std::string& yaml_path,
                                    const std::string& detail) {
  if (is_burn_in_box_color_channel(yaml_path)) {
    return with_job_error(USER_ERROR_JOB_107, detail);
  }
  if (is_burn_in_yaml_field(yaml_path, "box.opacity")) {
    return with_job_error(USER_ERROR_JOB_108, detail);
  }
  if (is_burn_in_yaml_field(yaml_path, "box.margin") ||
      is_burn_in_box_margin_side(yaml_path)) {
    return with_job_error(USER_ERROR_JOB_109, detail);
  }
  return {};
}

/*!
 * \brief Message for a \c layout field below its minimum, or empty.
 */
std::string layout_below_minimum_error(const std::string& yaml_path,
                                       const nlohmann::json& instance) {
  static const std::map<std::string, std::string_view> kBareDetail = {
      {"layout.canvas.width", USER_ERROR_JOB_60},
      {"layout.canvas.height", USER_ERROR_JOB_61},
      {"layout.slate.duration_frames", USER_ERROR_JOB_112}};
  const auto it = kBareDetail.find(yaml_path);
  if (it != kBareDetail.end()) {
    return with_job_error(it->second,
                          "(got " + describe_json_value(instance) + ").");
  }
  if (is_layout_image_margin_side(yaml_path)) {
    return with_job_error(USER_ERROR_JOB_69, got_detail(yaml_path, instance));
  }
  if (is_layout_background_channel(yaml_path)) {
    return with_job_error(USER_ERROR_JOB_71, got_detail(yaml_path, instance));
  }
  return {};
}

/*!
 * \brief Message for a burn-in / slate text field below its minimum, or empty.
 */
std::string text_overlay_below_minimum_error(const std::string& yaml_path,
                                             const nlohmann::json& instance) {
  const std::string detail = got_detail(yaml_path, instance);
  if (is_text_overlay_yaml_field(yaml_path, "font.size_px")) {
    return with_job_error(USER_ERROR_JOB_89, detail);
  }
  if (is_burn_in_font_color_channel(yaml_path)) {
    return with_job_error(USER_ERROR_JOB_91, detail);
  }
  return burn_in_box_range_error(yaml_path, detail);
}

/*!
 * \brief Message for a \c plans field below its minimum, or empty.
 */
std::string plan_below_minimum_error(const std::string& yaml_path,
                                     const nlohmann::json& instance) {
  if (is_plan_yaml_field(yaml_path, "sequence.handles.head")) {
    return with_job_error(USER_ERROR_JOB_140, got_detail(yaml_path, instance));
  }
  if (is_plan_yaml_field(yaml_path, "sequence.handles.tail")) {
    return with_job_error(USER_ERROR_JOB_141, got_detail(yaml_path, instance));
  }
  return {};
}

/*!
 * \brief Message for an \c output.videos field below its minimum, or empty.
 */
std::string output_video_below_minimum_error(const std::string& yaml_path,
                                             const nlohmann::json& instance) {
  const std::string detail = got_detail(yaml_path, instance);
  if (is_output_video_yaml_field(yaml_path, "fps")) {
    return with_job_error(USER_ERROR_JOB_168, detail);
  }
  if (is_output_video_yaml_field(yaml_path, "codec_options.crf")) {
    return with_job_error(USER_ERROR_JOB_174, detail);
  }
  if (is_output_video_yaml_field(yaml_path, "codec_options.bitrate_kbps")) {
    return with_job_error(USER_ERROR_JOB_175, detail);
  }
  if (is_output_video_yaml_field(yaml_path, "codec_options.gop")) {
    return with_job_error(USER_ERROR_JOB_176, detail);
  }
  if (is_output_video_yaml_field(yaml_path, "codec_options.qscale")) {
    return with_job_error(USER_ERROR_JOB_29, detail);
  }
  if (is_output_video_yaml_field(yaml_path, "codec_options.bits_per_mb")) {
    return with_job_error(USER_ERROR_JOB_98, detail);
  }
  if (is_output_video_yaml_field(yaml_path, "codec_options.mbs_per_slice")) {
    return with_job_error(USER_ERROR_JOB_99, detail);
  }
  return {};
}

/*!
 * \brief Message for any field below its schema minimum, or empty.
 */
std::string below_minimum_error(const std::string& yaml_path,
                                const nlohmann::json& instance) {
  std::string message = layout_below_minimum_error(yaml_path, instance);
  if (message.empty()) {
    message = text_overlay_below_minimum_error(yaml_path, instance);
  }
  if (message.empty()) {
    message = plan_below_minimum_error(yaml_path, instance);
  }
  if (message.empty()) {
    message = output_video_below_minimum_error(yaml_path, instance);
  }
  return message;
}

/*!
 * \brief Message for any field above its schema maximum, or empty.
 */
std::string exceeds_maximum_error(const std::string& yaml_path,
                                  const nlohmann::json& instance) {
  const std::string detail = got_detail(yaml_path, instance);
  if (is_layout_background_channel(yaml_path)) {
    return with_job_error(USER_ERROR_JOB_71, detail);
  }
  if (is_burn_in_font_color_channel(yaml_path)) {
    return with_job_error(USER_ERROR_JOB_91, detail);
  }
  if (is_output_video_yaml_field(yaml_path, "codec_options.crf")) {
    return with_job_error(USER_ERROR_JOB_174, detail);
  }
  if (is_output_video_yaml_field(yaml_path, "codec_options.qscale")) {
    return with_job_error(USER_ERROR_JOB_29, detail);
  }
  if (is_output_video_yaml_field(yaml_path, "codec_options.bits_per_mb")) {
    return with_job_error(USER_ERROR_JOB_98, detail);
  }
  if (is_output_video_yaml_field(yaml_path, "codec_options.mbs_per_slice")) {
    return with_job_error(USER_ERROR_JOB_99, detail);
  }
  return burn_in_box_range_error(yaml_path, detail);
}

/*!
 * \brief Message for a numeric range or \c multipleOf violation, or empty.
 */
std::string numeric_constraint_error(const std::string& yaml_path,
                                     const nlohmann::json& instance,
                                     const std::string& message) {
  if (message.find("multiple of 2") != std::string::npos) {
    return with_job_error(USER_ERROR_JOB_1, yaml_path + ".");
  }
  if (yaml_path == "layout.pixel_aspect" &&
      is_exclusive_minimum_violation(message)) {
    return with_job_error(USER_ERROR_JOB_132, got_detail(yaml_path, instance));
  }
  if (message.find("below minimum") != std::string::npos) {
    return below_minimum_error(yaml_path, instance);
  }
  if (message.find("exceeds maximum") != std::string::npos) {
    return exceeds_maximum_error(yaml_path, instance);
  }
  return {};
}

/*!
 * \brief Message for a wrong-typed field named by its full path, or empty.
 */
std::string named_field_type_error(const std::string& yaml_path,
                                   const nlohmann::json& instance) {
  static const std::map<std::string, std::string_view> kNamedFields = {
      {"metadata.substitutions", USER_ERROR_JOB_4},
      {"layout.canvas.width", USER_ERROR_JOB_56},
      {"layout.canvas.height", USER_ERROR_JOB_58},
      {"layout.pixel_aspect", USER_ERROR_JOB_59},
      {"layout.image.fit", USER_ERROR_JOB_65},
      {"layout.image.filter", USER_ERROR_JOB_66},
      {"layout.image.min_margin_px", USER_ERROR_JOB_67},
      {"layout.background", USER_ERROR_JOB_70},
      {"layout.slate.duration_frames", USER_ERROR_JOB_111},
      {"color.context", USER_ERROR_JOB_133}};
  const auto it = kNamedFields.find(yaml_path);
  if (it != kNamedFields.end()) {
    return with_job_error(it->second, is_detail(yaml_path, instance));
  }
  if (is_layout_image_margin_side(yaml_path)) {
    return with_job_error(USER_ERROR_JOB_68, is_detail(yaml_path, instance));
  }
  if (is_layout_background_channel(yaml_path)) {
    return with_job_error(USER_ERROR_JOB_71, is_detail(yaml_path, instance));
  }
  return {};
}

/*!
 * \brief Message for a wrong-typed burn-in or slate line field, or empty.
 */
std::string text_overlay_type_error(const std::string& yaml_path,
                                    const nlohmann::json& instance) {
  static constexpr std::pair<std::string_view, std::string_view> kFields[] = {
      {"position.mode", USER_ERROR_JOB_77},
      {"position.anchor", USER_ERROR_JOB_78},
      {"position.x", USER_ERROR_JOB_81},
      {"position.y", USER_ERROR_JOB_82},
      {"font.path", USER_ERROR_JOB_87},
      {"font.size_px", USER_ERROR_JOB_88}};
  if (is_slate_line_yaml_field(yaml_path, "text")) {
    return with_job_error(USER_ERROR_JOB_117, is_detail(yaml_path, instance));
  }
  for (const auto& field : kFields) {
    if (is_text_overlay_yaml_field(yaml_path, field.first)) {
      return with_job_error(field.second, is_detail(yaml_path, instance));
    }
  }
  return {};
}

/*!
 * \brief Message for a wrong-typed burn-in \c box field, or empty.
 */
std::string burn_in_box_type_error(const std::string& yaml_path,
                                   const std::string& detail) {
  if (is_burn_in_yaml_field(yaml_path, "box.mode")) {
    return with_job_error(USER_ERROR_JOB_105, detail);
  }
  if (is_burn_in_yaml_field(yaml_path, "box.color")) {
    return with_job_error(USER_ERROR_JOB_106, detail);
  }
  if (is_burn_in_yaml_field(yaml_path, "box.opacity")) {
    return with_job_error(USER_ERROR_JOB_108, detail);
  }
  return {};
}

/*!
 * \brief Message for a wrong-typed burn-in only field, or empty.
 */
std::string burn_in_type_error(const std::string& yaml_path,
                               const nlohmann::json& instance) {
  const std::string detail = is_detail(yaml_path, instance);
  if (is_burn_in_yaml_field(yaml_path, "template")) {
    return with_job_error(USER_ERROR_JOB_76, detail);
  }
  if (is_burn_in_yaml_field(yaml_path, "font.color")) {
    return with_job_error(USER_ERROR_JOB_90, detail);
  }
  if (is_burn_in_font_color_channel(yaml_path)) {
    return with_job_error(USER_ERROR_JOB_91, detail);
  }
  return burn_in_box_type_error(yaml_path, detail);
}

/*!
 * \brief Message for a wrong-typed \c plans field, or empty.
 */
std::string plan_type_error(const std::string& yaml_path,
                            const nlohmann::json& instance) {
  static constexpr std::pair<std::string_view, std::string_view> kFields[] = {
      {"id", USER_ERROR_JOB_119},
      {"input_colorspace", USER_ERROR_JOB_121},
      {"sequence.path", USER_ERROR_JOB_124},
      {"sequence.frame_start", USER_ERROR_JOB_126},
      {"sequence.frame_end", USER_ERROR_JOB_128},
      {"sequence.handles.head", USER_ERROR_JOB_140},
      {"sequence.handles.tail", USER_ERROR_JOB_141},
      {"audio.path", USER_ERROR_JOB_130},
      {"timecode.drop_frame", USER_ERROR_JOB_142}};
  for (const auto& field : kFields) {
    if (is_plan_yaml_field(yaml_path, field.first)) {
      return with_job_error(field.second, is_detail(yaml_path, instance));
    }
  }
  return {};
}

/*!
 * \brief Type message for codec-specific \c codec_options fields, or empty.
 */
std::string codec_options_type_error(const std::string& yaml_path,
                                     const std::string& codec,
                                     const std::string& detail) {
  if (is_output_video_yaml_field(yaml_path, "codec_options.qscale")) {
    return with_job_error(USER_ERROR_JOB_29, detail);
  }
  if (is_output_video_yaml_field(yaml_path, "codec_options.huffman")) {
    return with_job_error(USER_ERROR_JOB_188, detail);
  }
  if (is_output_video_yaml_field(yaml_path, "codec_options.interlaced")) {
    return with_job_error(USER_ERROR_JOB_194, detail);
  }
  if (is_output_video_yaml_field(yaml_path, "codec_options.nitris_compat")) {
    return with_job_error(USER_ERROR_JOB_195, detail);
  }
  if (is_output_video_yaml_field(yaml_path, "codec_options.quant_mat")) {
    return with_job_error(USER_ERROR_JOB_201, detail);
  }
  if (is_output_video_yaml_field(yaml_path, "codec_options.bits_per_mb")) {
    return with_job_error(USER_ERROR_JOB_98, detail);
  }
  if (is_output_video_yaml_field(yaml_path, "codec_options.mbs_per_slice")) {
    return with_job_error(USER_ERROR_JOB_99, detail);
  }
  if (is_output_video_yaml_field(yaml_path, "codec_options.vendor")) {
    return with_job_error(USER_ERROR_JOB_203, detail);
  }
  if (is_output_video_yaml_field(yaml_path, "codec_options.faststart")) {
    return with_job_error(USER_ERROR_JOB_185, detail);
  }
  if (is_output_video_yaml_field(yaml_path, "codec_options.pix_fmt")) {
    if (codec == "mjpeg") {
      return with_job_error(USER_ERROR_JOB_186, detail);
    }
    if (codec == "dnxhd") {
      return with_job_error(USER_ERROR_JOB_192, detail);
    }
    if (codec == "prores") {
      return with_job_error(USER_ERROR_JOB_199, detail);
    }
    return with_job_error(USER_ERROR_JOB_177, detail);
  }
  if (is_output_video_yaml_field(yaml_path, "codec_options.profile")) {
    if (codec == "dnxhd") {
      return with_job_error(USER_ERROR_JOB_190, detail);
    }
    if (codec == "prores") {
      return with_job_error(USER_ERROR_JOB_197, detail);
    }
    return with_job_error(USER_ERROR_JOB_181, detail);
  }
  return {};
}

/*!
 * \brief Message for a wrong-typed \c output.videos field, or empty.
 */
std::string output_video_type_error(const std::string& yaml_path,
                                    const nlohmann::json& instance,
                                    const std::string& codec) {
  static constexpr std::pair<std::string_view, std::string_view> kFields[] = {
      {"id", USER_ERROR_JOB_144},
      {"enabled", USER_ERROR_JOB_146},
      {"display_view.display", USER_ERROR_JOB_149},
      {"display_view.view", USER_ERROR_JOB_151},
      {"signal.range", USER_ERROR_JOB_154},
      {"signal.matrix", USER_ERROR_JOB_157},
      {"signal.primaries", USER_ERROR_JOB_160},
      {"signal.transfer", USER_ERROR_JOB_163},
      {"path", USER_ERROR_JOB_166},
      {"fps", USER_ERROR_JOB_168},
      {"codec", USER_ERROR_JOB_170},
      {"codec_options.preset", USER_ERROR_JOB_172},
      {"codec_options.crf", USER_ERROR_JOB_174},
      {"codec_options.gop", USER_ERROR_JOB_176},
      {"codec_options.tune", USER_ERROR_JOB_179}};
  const std::string detail = is_detail(yaml_path, instance);
  for (const auto& field : kFields) {
    if (is_output_video_yaml_field(yaml_path, field.first)) {
      return with_job_error(field.second, detail);
    }
  }
  if (is_output_video_yaml_field(yaml_path, "codec_options.bitrate_kbps")) {
    return with_job_error(
        codec == "dnxhd" ? USER_ERROR_JOB_32 : USER_ERROR_JOB_175, detail);
  }
  return codec_options_type_error(yaml_path, codec, detail);
}

/*!
 * \brief Message for a wrong-typed \c output.image_sequences field, or empty.
 */
std::string output_image_sequence_type_error(const std::string& yaml_path,
                                             const nlohmann::json& instance) {
  static constexpr std::pair<std::string_view, std::string_view> kFields[] = {
      {"id", USER_ERROR_JOB_205},
      {"enabled", USER_ERROR_JOB_207},
      {"display_view.display", USER_ERROR_JOB_210},
      {"display_view.view", USER_ERROR_JOB_212},
      {"path_pattern", USER_ERROR_JOB_214}};
  const std::string detail = is_detail(yaml_path, instance);
  for (const auto& field : kFields) {
    if (is_output_image_sequence_yaml_field(yaml_path, field.first)) {
      return with_job_error(field.second, detail);
    }
  }
  return {};
}

/*!
 * \brief Message for a field whose JSON type breaks the contract.
 */
std::string wrong_type_error(const std::string& yaml_path,
                             const nlohmann::json& instance,
                             const nlohmann::json& root) {
  const std::string named = named_field_type_error(yaml_path, instance);
  if (!named.empty()) {
    return named;
  }
  const std::string overlay = text_overlay_type_error(yaml_path, instance);
  if (!overlay.empty()) {
    return overlay;
  }
  const std::string burn_in = burn_in_type_error(yaml_path, instance);
  if (!burn_in.empty()) {
    return burn_in;
  }
  const std::string plan = plan_type_error(yaml_path, instance);
  if (!plan.empty()) {
    return plan;
  }
  const std::string video = output_video_type_error(
      yaml_path, instance, output_video_codec_at(root, yaml_path));
  if (!video.empty()) {
    return video;
  }
  const std::string sequence =
      output_image_sequence_type_error(yaml_path, instance);
  if (!sequence.empty()) {
    return sequence;
  }
  return with_job_error(USER_ERROR_JOB_41, got_detail(yaml_path, instance));
}

/*!
 * \brief Enum message for codec-specific \c codec_options fields, or empty.
 */
std::string codec_options_enum_error(const std::string& yaml_path,
                                     const std::string& codec,
                                     const std::string& detail) {
  if (is_output_video_yaml_field(yaml_path, "codec_options.huffman")) {
    return with_job_error(USER_ERROR_JOB_189, detail);
  }
  if (is_output_video_yaml_field(yaml_path, "codec_options.quant_mat")) {
    return with_job_error(USER_ERROR_JOB_202, detail);
  }
  if (is_output_video_yaml_field(yaml_path, "codec_options.pix_fmt")) {
    if (codec == "mjpeg") {
      return with_job_error(USER_ERROR_JOB_187, detail);
    }
    if (codec == "dnxhd") {
      return with_job_error(USER_ERROR_JOB_193, detail);
    }
    if (codec == "prores") {
      return with_job_error(USER_ERROR_JOB_200, detail);
    }
    return with_job_error(USER_ERROR_JOB_178, detail);
  }
  if (is_output_video_yaml_field(yaml_path, "codec_options.profile")) {
    if (codec == "dnxhd") {
      return with_job_error(USER_ERROR_JOB_191, detail);
    }
    if (codec == "prores") {
      return with_job_error(USER_ERROR_JOB_198, detail);
    }
    return with_job_error(USER_ERROR_JOB_182, detail);
  }
  return {};
}

/*!
 * \brief Message for an \c output.videos enum violation, or empty.
 */
std::string output_video_enum_error(const std::string& yaml_path,
                                    const std::string& detail,
                                    const std::string& codec) {
  static constexpr std::pair<std::string_view, std::string_view> kFields[] = {
      {"signal.range", USER_ERROR_JOB_155},
      {"signal.matrix", USER_ERROR_JOB_158},
      {"signal.primaries", USER_ERROR_JOB_161},
      {"signal.transfer", USER_ERROR_JOB_164},
      {"codec", USER_ERROR_JOB_171},
      {"codec_options.preset", USER_ERROR_JOB_173},
      {"codec_options.tune", USER_ERROR_JOB_180},
      {"codec_options.level", USER_ERROR_JOB_184}};
  for (const auto& field : kFields) {
    if (is_output_video_yaml_field(yaml_path, field.first)) {
      return with_job_error(field.second, detail);
    }
  }
  return codec_options_enum_error(yaml_path, codec, detail);
}

/*!
 * \brief Message for a value outside its schema \c enum, or empty.
 */
std::string enum_violation_error(const std::string& yaml_path,
                                 const nlohmann::json& instance,
                                 const nlohmann::json& root) {
  const std::string got = instance.is_string() ? instance.get<std::string>()
                                               : describe_json_value(instance);
  const std::string detail = "'" + got + "'.";
  if (yaml_path == "layout.image.fit") {
    return with_job_error(USER_ERROR_JOB_22, detail);
  }
  if (yaml_path == "layout.image.filter") {
    return with_job_error(USER_ERROR_JOB_23, detail);
  }
  if (is_text_overlay_yaml_field(yaml_path, "position.mode")) {
    return with_job_error(USER_ERROR_JOB_20, detail);
  }
  if (is_text_overlay_yaml_field(yaml_path, "position.anchor")) {
    return with_job_error(USER_ERROR_JOB_19, detail);
  }
  if (is_burn_in_yaml_field(yaml_path, "box.mode")) {
    return with_job_error(USER_ERROR_JOB_21, detail);
  }
  return output_video_enum_error(yaml_path, detail,
                                 output_video_codec_at(root, yaml_path));
}

/*!
 * \brief Message for a value matching none of its \c oneOf / \c anyOf branches.
 */
/*!
 * \brief Message for burn-in \c box.margin when no \c oneOf branch matches.
 *
 * Unknown map keys are reported as JOB_40 (same as parse / min_margin_px);
 * other mismatches stay JOB_109.
 */
std::string burn_in_box_margin_subschema_error(const std::string& yaml_path,
                                               const nlohmann::json& instance) {
  if (instance.is_object()) {
    for (auto it = instance.begin(); it != instance.end(); ++it) {
      if (it.key() != "top" && it.key() != "right" && it.key() != "bottom" &&
          it.key() != "left") {
        return with_job_error(USER_ERROR_JOB_40,
                              yaml_path + "." + it.key() + ".");
      }
    }
  }
  return with_job_error(USER_ERROR_JOB_109, is_detail(yaml_path, instance));
}

/*!
 * \brief Message for H.264 \c level when no \c anyOf branch matches.
 */
std::string h264_level_subschema_error(const std::string& yaml_path,
                                       const nlohmann::json& instance) {
  if (!is_output_video_yaml_field(yaml_path, "codec_options.level")) {
    return {};
  }
  if (!instance.is_string() && !instance.is_number()) {
    return with_job_error(USER_ERROR_JOB_28, yaml_path + ".");
  }
  const std::string got = instance.is_string() ? instance.get<std::string>()
                                               : describe_json_value(instance);
  return with_job_error(USER_ERROR_JOB_184, "'" + got + "'.");
}

/*!
 * \brief Message for a value matching none of its \c oneOf / \c anyOf branches.
 */
std::string subschema_error(const std::string& yaml_path,
                            const nlohmann::json& instance) {
  if (is_burn_in_yaml_field(yaml_path, "box.margin")) {
    return burn_in_box_margin_subschema_error(yaml_path, instance);
  }
  if (is_plan_yaml_field(yaml_path, "timecode.start")) {
    return with_job_error(USER_ERROR_JOB_101, yaml_path + ".");
  }
  const std::string level = h264_level_subschema_error(yaml_path, instance);
  if (!level.empty()) {
    return level;
  }
  return with_job_error(USER_ERROR_JOB_42, yaml_path + ".");
}

/*!
 * \brief Message for an empty \c plans string field, or empty.
 */
std::string plan_min_length_error(const std::string& yaml_path,
                                  const std::string& detail) {
  static constexpr std::pair<std::string_view, std::string_view> kFields[] = {
      {"id", USER_ERROR_JOB_119},
      {"input_colorspace", USER_ERROR_JOB_121},
      {"sequence.path", USER_ERROR_JOB_124},
      {"audio.path", USER_ERROR_JOB_130}};
  for (const auto& field : kFields) {
    if (is_plan_yaml_field(yaml_path, field.first)) {
      return with_job_error(field.second, detail);
    }
  }
  return {};
}

/*!
 * \brief Message for an empty \c output.videos string field, or empty.
 */
std::string output_video_min_length_error(const std::string& yaml_path,
                                          const std::string& detail) {
  static constexpr std::pair<std::string_view, std::string_view> kFields[] = {
      {"id", USER_ERROR_JOB_144},
      {"display_view.display", USER_ERROR_JOB_149},
      {"display_view.view", USER_ERROR_JOB_151},
      {"path", USER_ERROR_JOB_166},
      {"codec", USER_ERROR_JOB_170}};
  for (const auto& field : kFields) {
    if (is_output_video_yaml_field(yaml_path, field.first)) {
      return with_job_error(field.second, detail);
    }
  }
  return {};
}

/*!
 * \brief Message for an empty \c output.image_sequences string field, or empty.
 */
std::string output_image_sequence_min_length_error(const std::string& yaml_path,
                                                   const std::string& detail) {
  static constexpr std::pair<std::string_view, std::string_view> kFields[] = {
      {"id", USER_ERROR_JOB_205},
      {"display_view.display", USER_ERROR_JOB_210},
      {"display_view.view", USER_ERROR_JOB_212},
      {"path_pattern", USER_ERROR_JOB_214}};
  for (const auto& field : kFields) {
    if (is_output_image_sequence_yaml_field(yaml_path, field.first)) {
      return with_job_error(field.second, detail);
    }
  }
  return {};
}

/*!
 * \brief Message for a string field that must not be empty (\c minLength).
 */
std::string min_length_error(const std::string& yaml_path) {
  const std::string detail = yaml_path + " is empty.";
  if (yaml_path == "color.ocio_config") {
    return with_job_error(USER_ERROR_JOB_48, detail);
  }
  if (yaml_path == "color.working_colorspace") {
    return with_job_error(USER_ERROR_JOB_49, detail);
  }
  const std::string video = output_video_min_length_error(yaml_path, detail);
  if (!video.empty()) {
    return video;
  }
  const std::string sequence =
      output_image_sequence_min_length_error(yaml_path, detail);
  if (!sequence.empty()) {
    return sequence;
  }
  if (is_text_overlay_yaml_field(yaml_path, "font.path")) {
    return with_job_error(USER_ERROR_JOB_87, detail);
  }
  if (is_slate_line_yaml_field(yaml_path, "text")) {
    return with_job_error(USER_ERROR_JOB_117, detail);
  }
  if (yaml_path.rfind("color.context.", 0) == 0) {
    return with_job_error(USER_ERROR_JOB_135, detail);
  }
  const std::string plan = plan_min_length_error(yaml_path, detail);
  return plan.empty() ? with_job_error(USER_ERROR_JOB_43, yaml_path + ".")
                      : plan;
}

/*!
 * \brief Message for ProRes \c profile when no \c anyOf branch matches.
 */
std::string prores_profile_subschema_error(const std::string& yaml_path,
                                           const nlohmann::json& instance) {
  if (!is_output_video_yaml_field(yaml_path, "codec_options.profile")) {
    return {};
  }
  if (instance.is_number_integer() && instance.get<int>() == 4444) {
    return {};
  }
  if (!instance.is_string() && !instance.is_number()) {
    return with_job_error(USER_ERROR_JOB_197, yaml_path + ".");
  }
  const std::string got = instance.is_string() ? instance.get<std::string>()
                                               : describe_json_value(instance);
  return with_job_error(USER_ERROR_JOB_198, "'" + got + "'.");
}

/*!
 * \brief Message for a type, enum, subschema, or length violation, or empty.
 */
std::string structural_constraint_error(const std::string& yaml_path,
                                        const nlohmann::json& instance,
                                        const std::string& message,
                                        const nlohmann::json& root) {
  const bool no_subschema =
      message.find("no subschema has succeeded") != std::string::npos ||
      message.find("is not valid under any of the given schemas") !=
          std::string::npos;
  if (yaml_path == "output" && no_subschema) {
    return with_job_error(USER_ERROR_JOB_39);
  }
  if (message == "instance invalid as per false-schema") {
    if (is_output_video_yaml_field(yaml_path, "codec_options.bitrate_kbps")) {
      return with_job_error(USER_ERROR_JOB_30, yaml_path + ".");
    }
    return with_job_error(USER_ERROR_JOB_40, yaml_path + ".");
  }
  // allOf / if-then may prefix the leaf reason (e.g. "…required to validate -
  // unexpected instance type").
  if (message.find("unexpected instance type") != std::string::npos) {
    return wrong_type_error(yaml_path, instance, root);
  }
  if (message.find("instance not found in required enum") !=
      std::string::npos) {
    return enum_violation_error(yaml_path, instance, root);
  }
  if (no_subschema) {
    const std::string prores =
        prores_profile_subschema_error(yaml_path, instance);
    if (!prores.empty()) {
      return prores;
    }
    return subschema_error(yaml_path, instance);
  }
  if (message.find("too short as per minLength") != std::string::npos) {
    if (is_output_video_yaml_field(yaml_path, "codec_options.vendor")) {
      return with_job_error(USER_ERROR_JOB_96, yaml_path + ".");
    }
    return min_length_error(yaml_path);
  }
  if (message.find("too long as per maxLength") != std::string::npos &&
      is_output_video_yaml_field(yaml_path, "codec_options.vendor")) {
    return with_job_error(USER_ERROR_JOB_96, yaml_path + ".");
  }
  return {};
}

std::string format_job_schema_error(const nlohmann::json::json_pointer& ptr,
                                    const nlohmann::json& instance,
                                    const std::string& message,
                                    const nlohmann::json& root);

/*!
 * \brief Re-reports an \c additionalProperties failure on the offending child.
 * \return Child message, or empty when \a message is not such a failure.
 */
std::string format_additional_property_error(
    const nlohmann::json::json_pointer& ptr, const nlohmann::json& instance,
    const std::string& message, const nlohmann::json& root) {
  const std::string prefix = "validation failed for additional property '";
  if (message.compare(0, prefix.size(), prefix) != 0) {
    return {};
  }
  const std::size_t end = message.find('\'', prefix.size());
  if (end == std::string::npos) {
    return {};
  }
  const std::string key = message.substr(prefix.size(), end - prefix.size());
  const std::string rest_mark = "': ";
  std::string inner;
  if (message.compare(end, rest_mark.size(), rest_mark) == 0) {
    inner = message.substr(end + rest_mark.size());
  }
  nlohmann::json child;
  if (instance.is_object()) {
    const auto it = instance.find(key);
    if (it != instance.end()) {
      child = *it;
    }
  }
  nlohmann::json::json_pointer child_ptr = ptr;
  child_ptr /= key;
  return format_job_schema_error(child_ptr, child, inner, root);
}

std::string format_job_schema_error(const nlohmann::json::json_pointer& ptr,
                                    const nlohmann::json& instance,
                                    const std::string& message,
                                    const nlohmann::json& root) {
  const std::string child =
      format_additional_property_error(ptr, instance, message, root);
  if (!child.empty()) {
    return child;
  }
  const std::string yaml_path = json_pointer_to_yaml_path(ptr);
  using Resolver = std::string (*)(const std::string&, const nlohmann::json&,
                                   const std::string&);
  static constexpr Resolver kResolvers[] = {
      &substitution_schema_error, &required_key_error,
      &array_size_error,          &property_count_error,
      &version_constraint_error,  &numeric_constraint_error};
  for (Resolver resolve : kResolvers) {
    const std::string resolved = resolve(yaml_path, instance, message);
    if (!resolved.empty()) {
      return resolved;
    }
  }
  const std::string structural =
      structural_constraint_error(yaml_path, instance, message, root);
  if (!structural.empty()) {
    return structural;
  }
  return with_job_error(USER_ERROR_JOB_44, yaml_path + ".");
}

class JobSchemaErrorHandler
    : public nlohmann::json_schema::basic_error_handler {
 public:
  explicit JobSchemaErrorHandler(const nlohmann::json& root) : root_(root) {}

  void error(const nlohmann::json::json_pointer& ptr,
             const nlohmann::json& instance,
             const std::string& message) override {
    nlohmann::json_schema::basic_error_handler::error(ptr, instance, message);
    if (first_message_.empty()) {
      first_message_ = format_job_schema_error(ptr, instance, message, root_);
    }
  }

  const std::string& message() const { return first_message_; }

 private:
  const nlohmann::json& root_;
  std::string first_message_;
};

}  // namespace

Status validate_job_schema(const std::filesystem::path& job_path) {
  if (job_path.empty()) {
    return Status::Internal(with_job_error(INTERNAL_ERROR_JOB_1));
  }

  YmlLoader loader;
  DAILYBOY_RETURN_IF_ERROR(loader.load(job_path));
  DAILYBOY_ASSIGN_OR_RETURN(const YAML::Node job_root,
                            expect_map(loader.root(), "job"));
  const nlohmann::json instance = yaml_to_json(job_root);

  std::ifstream schema_file(DAILYBOY_JOB_SCHEMA_PATH);
  if (!schema_file.is_open()) {
    return Status::Internal(
        with_job_error(INTERNAL_ERROR_JOB_2, DAILYBOY_JOB_SCHEMA_PATH));
  }

  nlohmann::json schema;
  try {
    schema_file >> schema;
  } catch (const std::exception& ex) {
    return Status::Internal(with_job_error(INTERNAL_ERROR_JOB_3, ex.what()));
  }

  nlohmann::json_schema::json_validator validator;
  try {
    validator.set_root_schema(schema);
  } catch (const std::exception& ex) {
    return Status::Internal(with_job_error(INTERNAL_ERROR_JOB_4, ex.what()));
  }
  JobSchemaErrorHandler err(instance);
  validator.validate(instance, err);
  if (err) {
    return Status::User(err.message());
  }

  const YAML::Node metadata = job_root["metadata"];
  std::map<std::string, SubstitutionValue> substitutions;
  if (metadata && metadata.IsMap()) {
    DAILYBOY_ASSIGN_OR_RETURN(substitutions,
                              read_substitutions(metadata["substitutions"]));
    if (substitutions.empty()) {
      return Status::User(with_job_error(USER_ERROR_JOB_6));
    }
    DAILYBOY_RETURN_IF_ERROR(
        validate_substitution_usage(job_root, substitutions));
  }
  DAILYBOY_RETURN_IF_ERROR(validate_fileseq_patterns(job_root, substitutions));
  DAILYBOY_RETURN_IF_ERROR(validate_output_contract(job_root));
  DAILYBOY_RETURN_IF_ERROR(validate_timecode_contract(job_root));
  if (job_root["color"] && job_root["color"].IsMap()) {
    DAILYBOY_RETURN_IF_ERROR(parse_colorimetry(job_root["color"]).status());
  }
  if (job_root["layout"] && job_root["layout"].IsMap()) {
    DAILYBOY_RETURN_IF_ERROR(parse_layout(job_root["layout"]).status());
  }
  if (job_root["plans"] && job_root["plans"].IsSequence()) {
    DAILYBOY_RETURN_IF_ERROR(parse_plans(job_root["plans"]).status());
  }
  if (job_root["output"] && job_root["output"].IsMap()) {
    DAILYBOY_RETURN_IF_ERROR(parse_output(job_root["output"]).status());
  }
  return Status::Ok();
}

}  // namespace dailyboy
