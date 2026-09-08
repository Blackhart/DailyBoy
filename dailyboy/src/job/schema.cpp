/*!
 * \file schema.cpp
 * \brief JSON Schema plus DailyBoy contract checks for a job YAML file.
 */

#include "job/schema.hpp"

#include <fstream>
#include <nlohmann/json-schema.hpp>

#include "error/job.hpp"
#include "job/parse_colorimetry.hpp"
#include "job/parse_layout.hpp"
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
 * \brief Reads one \c layout.background channel: unquoted number in [0, 1].
 */
Status validate_output_contract(const YAML::Node& job_root) {
  const YAML::Node output = job_root["output"];
  if (!output || !output.IsMap()) {
    return Status::Ok();
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

std::string missing_required_key_error(const std::string& path,
                                       const std::string& key) {
  if (key == "plans") {
    return with_job_error(USER_ERROR_JOB_12);
  }
  if (key == "color") {
    return with_job_error(USER_ERROR_JOB_13);
  }
  if (key == "layout") {
    return with_job_error(USER_ERROR_JOB_14);
  }
  if (key == "output") {
    return with_job_error(USER_ERROR_JOB_15);
  }
  if (key == "dailyboy_version") {
    return with_job_error(USER_ERROR_JOB_16);
  }
  if (key == "substitutions") {
    return with_job_error(USER_ERROR_JOB_5);
  }
  if (key == "ocio_config") {
    return with_job_error(USER_ERROR_JOB_17);
  }
  if (key == "canvas") {
    return with_job_error(USER_ERROR_JOB_54);
  }
  if (key == "image") {
    return with_job_error(USER_ERROR_JOB_62);
  }
  if (key == "fit" && path == "layout.image") {
    return with_job_error(USER_ERROR_JOB_64);
  }
  if (key == "template" && is_layout_burn_in_item_path(path)) {
    return with_job_error(USER_ERROR_JOB_73);
  }
  if (key == "position" && is_layout_burn_in_item_path(path)) {
    return with_job_error(USER_ERROR_JOB_74);
  }
  if (key == "font" && is_layout_burn_in_item_path(path)) {
    return with_job_error(USER_ERROR_JOB_75);
  }
  if (key == "width" && path == "layout.canvas") {
    return with_job_error(USER_ERROR_JOB_55);
  }
  if (key == "height" && path == "layout.canvas") {
    return with_job_error(USER_ERROR_JOB_57);
  }
  return with_job_error(USER_ERROR_JOB_11, path + " '" + key + "'.");
}

std::string format_job_schema_error(const nlohmann::json::json_pointer& ptr,
                                    const nlohmann::json& instance,
                                    const std::string& message) {
  const std::string prefix = "validation failed for additional property '";
  if (message.compare(0, prefix.size(), prefix) == 0) {
    const std::size_t start = prefix.size();
    const std::size_t end = message.find('\'', start);
    if (end != std::string::npos) {
      const std::string key = message.substr(start, end - start);
      std::string inner;
      const std::string rest_mark = "': ";
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
      return format_job_schema_error(child_ptr, child, inner);
    }
  }

  const std::string yaml_path = json_pointer_to_yaml_path(ptr);
  const std::string sub_key = substitution_value_key_from_yaml_path(yaml_path);
  if (!sub_key.empty()) {
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

  std::string frame_sub;
  std::string frame_key;
  if (parse_frame_map_entry_yaml_path(yaml_path, frame_sub, frame_key)) {
    nlohmann::json got = instance;
    if (instance.is_object()) {
      const auto it = instance.find(frame_key);
      if (it != instance.end()) {
        got = *it;
      }
    }
    return frame_map_value_error(frame_sub, frame_key,
                                 describe_json_value(got));
  }

  const std::string required_mark = "required property '";
  const std::size_t req = message.find(required_mark);
  if (req != std::string::npos) {
    const std::size_t start = req + required_mark.size();
    const std::size_t end = message.find('\'', start);
    if (end != std::string::npos) {
      return missing_required_key_error(yaml_path,
                                        message.substr(start, end - start));
    }
  }

  if (yaml_path == "plans" && (message == "array has too few items" ||
                               message == "array has too many items")) {
    const std::size_t count = instance.is_array() ? instance.size() : 0;
    return with_job_error(USER_ERROR_JOB_37,
                          "(got " + std::to_string(count) + ").");
  }

  if (yaml_path == "layout.burn_ins" && message == "array has too few items") {
    const std::size_t count = instance.is_array() ? instance.size() : 0;
    return with_job_error(USER_ERROR_JOB_72,
                          "(got " + std::to_string(count) + ").");
  }

  if (yaml_path == "metadata.substitutions" &&
      message == "too few properties") {
    return with_job_error(USER_ERROR_JOB_6);
  }

  if (yaml_path == "color.context" && message == "too few properties") {
    return with_job_error(USER_ERROR_JOB_51);
  }

  if (yaml_path == "dailyboy_version" &&
      (message == "instance not const" ||
       message == "unexpected instance type")) {
    return with_job_error(USER_ERROR_JOB_38,
                          "(got " + describe_json_value(instance) + ").");
  }

  if (message.find("multiple of 2") != std::string::npos) {
    return with_job_error(USER_ERROR_JOB_1, yaml_path + ".");
  }

  if (message.find("below minimum") != std::string::npos) {
    if (yaml_path == "layout.canvas.width") {
      return with_job_error(USER_ERROR_JOB_60,
                            "(got " + describe_json_value(instance) + ").");
    }
    if (yaml_path == "layout.canvas.height") {
      return with_job_error(USER_ERROR_JOB_61,
                            "(got " + describe_json_value(instance) + ").");
    }
    if (yaml_path == "layout.image.min_margin_px.top" ||
        yaml_path == "layout.image.min_margin_px.right" ||
        yaml_path == "layout.image.min_margin_px.bottom" ||
        yaml_path == "layout.image.min_margin_px.left") {
      return with_job_error(
          USER_ERROR_JOB_69,
          yaml_path + " (got " + describe_json_value(instance) + ").");
    }
    if (is_layout_background_channel(yaml_path)) {
      return with_job_error(
          USER_ERROR_JOB_71,
          yaml_path + " (got " + describe_json_value(instance) + ").");
    }
    if (is_burn_in_yaml_field(yaml_path, "font.size_px")) {
      return with_job_error(
          USER_ERROR_JOB_89,
          yaml_path + " (got " + describe_json_value(instance) + ").");
    }
    if (is_burn_in_font_color_channel(yaml_path)) {
      return with_job_error(
          USER_ERROR_JOB_91,
          yaml_path + " (got " + describe_json_value(instance) + ").");
    }
  }

  if (message.find("exceeds maximum") != std::string::npos) {
    if (is_layout_background_channel(yaml_path)) {
      return with_job_error(
          USER_ERROR_JOB_71,
          yaml_path + " (got " + describe_json_value(instance) + ").");
    }
    if (is_burn_in_font_color_channel(yaml_path)) {
      return with_job_error(
          USER_ERROR_JOB_91,
          yaml_path + " (got " + describe_json_value(instance) + ").");
    }
  }

  if (yaml_path == "output" &&
      message.find("no subschema has succeeded") != std::string::npos) {
    return with_job_error(USER_ERROR_JOB_39);
  }

  if (message == "instance invalid as per false-schema") {
    return with_job_error(USER_ERROR_JOB_40, yaml_path + ".");
  }

  if (message == "unexpected instance type") {
    if (yaml_path == "layout.canvas.width") {
      return with_job_error(
          USER_ERROR_JOB_56,
          "layout.canvas.width is " + describe_json_value(instance) + ".");
    }
    if (yaml_path == "layout.canvas.height") {
      return with_job_error(
          USER_ERROR_JOB_58,
          "layout.canvas.height is " + describe_json_value(instance) + ".");
    }
    if (yaml_path == "layout.pixel_aspect") {
      return with_job_error(
          USER_ERROR_JOB_59,
          "layout.pixel_aspect is " + describe_json_value(instance) + ".");
    }
    if (yaml_path == "layout.image.fit") {
      return with_job_error(
          USER_ERROR_JOB_65,
          "layout.image.fit is " + describe_json_value(instance) + ".");
    }
    if (yaml_path == "layout.image.filter") {
      return with_job_error(
          USER_ERROR_JOB_66,
          "layout.image.filter is " + describe_json_value(instance) + ".");
    }
    if (yaml_path == "layout.image.min_margin_px") {
      return with_job_error(USER_ERROR_JOB_67,
                            "layout.image.min_margin_px is " +
                                describe_json_value(instance) + ".");
    }
    if (yaml_path == "layout.image.min_margin_px.top" ||
        yaml_path == "layout.image.min_margin_px.right" ||
        yaml_path == "layout.image.min_margin_px.bottom" ||
        yaml_path == "layout.image.min_margin_px.left") {
      return with_job_error(
          USER_ERROR_JOB_68,
          yaml_path + " is " + describe_json_value(instance) + ".");
    }
    if (yaml_path == "layout.background") {
      return with_job_error(
          USER_ERROR_JOB_70,
          "layout.background is " + describe_json_value(instance) + ".");
    }
    if (is_layout_background_channel(yaml_path)) {
      return with_job_error(
          USER_ERROR_JOB_71,
          yaml_path + " is " + describe_json_value(instance) + ".");
    }
    if (is_burn_in_yaml_field(yaml_path, "template")) {
      return with_job_error(
          USER_ERROR_JOB_76,
          yaml_path + " is " + describe_json_value(instance) + ".");
    }
    if (is_burn_in_yaml_field(yaml_path, "position.mode")) {
      return with_job_error(
          USER_ERROR_JOB_77,
          yaml_path + " is " + describe_json_value(instance) + ".");
    }
    if (is_burn_in_yaml_field(yaml_path, "position.anchor")) {
      return with_job_error(
          USER_ERROR_JOB_78,
          yaml_path + " is " + describe_json_value(instance) + ".");
    }
    if (is_burn_in_yaml_field(yaml_path, "position.x")) {
      return with_job_error(
          USER_ERROR_JOB_81,
          yaml_path + " is " + describe_json_value(instance) + ".");
    }
    if (is_burn_in_yaml_field(yaml_path, "position.y")) {
      return with_job_error(
          USER_ERROR_JOB_82,
          yaml_path + " is " + describe_json_value(instance) + ".");
    }
    if (is_burn_in_yaml_field(yaml_path, "font.path")) {
      return with_job_error(
          USER_ERROR_JOB_87,
          yaml_path + " is " + describe_json_value(instance) + ".");
    }
    if (is_burn_in_yaml_field(yaml_path, "font.size_px")) {
      return with_job_error(
          USER_ERROR_JOB_88,
          yaml_path + " is " + describe_json_value(instance) + ".");
    }
    if (is_burn_in_yaml_field(yaml_path, "font.color")) {
      return with_job_error(
          USER_ERROR_JOB_90,
          yaml_path + " is " + describe_json_value(instance) + ".");
    }
    if (is_burn_in_font_color_channel(yaml_path)) {
      return with_job_error(
          USER_ERROR_JOB_91,
          yaml_path + " is " + describe_json_value(instance) + ".");
    }
    return with_job_error(
        USER_ERROR_JOB_41,
        yaml_path + " (got " + describe_json_value(instance) + ").");
  }

  if (message == "instance not found in required enum") {
    const std::string got = instance.is_string()
                                ? instance.get<std::string>()
                                : describe_json_value(instance);
    if (yaml_path == "layout.image.fit") {
      return with_job_error(USER_ERROR_JOB_22, "'" + got + "'.");
    }
    if (yaml_path == "layout.image.filter") {
      return with_job_error(USER_ERROR_JOB_23, "'" + got + "'.");
    }
    if (is_burn_in_yaml_field(yaml_path, "position.mode")) {
      return with_job_error(USER_ERROR_JOB_20, "'" + got + "'.");
    }
    if (is_burn_in_yaml_field(yaml_path, "position.anchor")) {
      return with_job_error(USER_ERROR_JOB_19, "'" + got + "'.");
    }
  }

  if (message.find("no subschema has succeeded") != std::string::npos) {
    return with_job_error(USER_ERROR_JOB_42, yaml_path + ".");
  }

  if (message.find("too short as per minLength") != std::string::npos) {
    if (yaml_path == "color.ocio_config") {
      return with_job_error(USER_ERROR_JOB_48, "color.ocio_config is empty.");
    }
    if (yaml_path == "color.working_colorspace") {
      return with_job_error(USER_ERROR_JOB_49,
                            "color.working_colorspace is empty.");
    }
    if (yaml_path.find("display_view.display") != std::string::npos ||
        yaml_path.find("display_view.view") != std::string::npos) {
      return with_job_error(USER_ERROR_JOB_50, yaml_path + " is empty.");
    }
    if (is_burn_in_yaml_field(yaml_path, "font.path")) {
      return with_job_error(USER_ERROR_JOB_87, yaml_path + " is empty.");
    }
    return with_job_error(USER_ERROR_JOB_43, yaml_path + ".");
  }

  return with_job_error(USER_ERROR_JOB_44, yaml_path + ".");
}

class JobSchemaErrorHandler
    : public nlohmann::json_schema::basic_error_handler {
 public:
  void error(const nlohmann::json::json_pointer& ptr,
             const nlohmann::json& instance,
             const std::string& message) override {
    nlohmann::json_schema::basic_error_handler::error(ptr, instance, message);
    if (first_message_.empty()) {
      first_message_ = format_job_schema_error(ptr, instance, message);
    }
  }

  const std::string& message() const { return first_message_; }

 private:
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
  JobSchemaErrorHandler err;
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
  if (job_root["color"] && job_root["color"].IsMap()) {
    DAILYBOY_RETURN_IF_ERROR(parse_colorimetry(job_root["color"]).status());
  }
  if (job_root["layout"] && job_root["layout"].IsMap()) {
    DAILYBOY_RETURN_IF_ERROR(parse_layout(job_root["layout"]).status());
  }
  return Status::Ok();
}

}  // namespace dailyboy
