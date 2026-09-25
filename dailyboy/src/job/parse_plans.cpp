/*!
 * \file parse_plans.cpp
 * \brief Parse job YAML plans and sequences.
 */

#include "job/parse_plans.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "error/job.hpp"
#include "job/yaml_read.hpp"

namespace dailyboy {

namespace {

/*!
 * \brief Reads \c handles.head or \c handles.tail: unquoted integer \c >= 0.
 * \param error_text Side-specific error (\c USER_ERROR_JOB_140 or \c _141).
 */
StatusOr<int> read_handle_count(const YAML::Node& node,
                                std::string_view error_text,
                                const std::string& loc) {
  DAILYBOY_ASSIGN_OR_RETURN(const int value,
                            read_yaml_integer(node, error_text, loc));
  if (value < 0) {
    return Status::User(with_job_error(
        error_text, loc + " (got integer " + std::to_string(value) + ")."));
  }
  return value;
}

/*!
 * \brief Parses optional \c sequence.handles (object; head/tail default to 0).
 */
Status parse_sequence_handles(const YAML::Node& node, const std::string& field,
                              JobSequence& sequence) {
  if (!node || node.IsNull()) {
    return Status::Ok();
  }
  if (!node.IsMap()) {
    return Status::User(with_job_error(
        USER_ERROR_JOB_100, field + " is " + describe_yaml_value(node) + "."));
  }
  for (const auto& kv : node) {
    const std::string key = kv.first.as<std::string>();
    if (key != "head" && key != "tail") {
      return Status::User(
          with_job_error(USER_ERROR_JOB_40, field + "." + key + "."));
    }
  }
  if (node["head"]) {
    DAILYBOY_ASSIGN_OR_RETURN(
        const int head,
        read_handle_count(node["head"], USER_ERROR_JOB_140, field + ".head"));
    sequence.set_handle_head(head);
  }
  if (node["tail"]) {
    DAILYBOY_ASSIGN_OR_RETURN(
        const int tail,
        read_handle_count(node["tail"], USER_ERROR_JOB_141, field + ".tail"));
    sequence.set_handle_tail(tail);
  }
  return Status::Ok();
}

StatusOr<std::optional<JobPlanAudio>> parse_plan_audio(
    const YAML::Node& node, const std::string& field) {
  if (!node) {
    return std::optional<JobPlanAudio>{};
  }
  DAILYBOY_ASSIGN_OR_RETURN(const YAML::Node map, expect_map(node, field));
  if (!map["path"] || map["path"].IsNull()) {
    return Status::User(with_job_error(USER_ERROR_JOB_129, field + ".path."));
  }
  DAILYBOY_ASSIGN_OR_RETURN(
      std::string path, read_quoted_nonempty_string(
                            map["path"], USER_ERROR_JOB_130, field + ".path"));
  JobPlanAudio audio;
  audio.set_path(std::move(path));
  return std::optional<JobPlanAudio>{std::move(audio)};
}

StatusOr<JobPlanTimecode::Start> parse_timecode_start(const YAML::Node& node,
                                                      const std::string& loc) {
  if (!node || !node.IsScalar()) {
    return Status::User(with_job_error(USER_ERROR_JOB_101, loc + "."));
  }
  const std::string scalar = node.Scalar();
  if (scalar.find(':') != std::string::npos ||
      scalar.find(';') != std::string::npos) {
    return JobPlanTimecode::Start{scalar};
  }
  int frames = 0;
  try {
    frames = node.as<int>();
  } catch (const YAML::Exception&) {
    return Status::User(with_job_error(USER_ERROR_JOB_101, loc + "."));
  }
  if (frames < 0) {
    return Status::User(with_job_error(USER_ERROR_JOB_101, loc + "."));
  }
  return JobPlanTimecode::Start{frames};
}

/*!
 * \brief Reads optional \c timecode.drop_frame; defaults to \c false.
 *
 * Only unquoted YAML booleans are accepted, so \c "true" is an error.
 */
StatusOr<bool> read_drop_frame(const YAML::Node& map,
                               const std::string& field) {
  const YAML::Node node = map["drop_frame"];
  if (!node || node.IsNull()) {
    return false;
  }
  bool value = false;
  if (!node.IsScalar() || is_yaml_quoted_string(node) ||
      !parse_bool_scalar(node.as<std::string>(), value)) {
    return Status::User(with_job_error(
        USER_ERROR_JOB_142,
        field + ".drop_frame is " + describe_yaml_value(node) + "."));
  }
  return value;
}

StatusOr<std::optional<JobPlanTimecode>> parse_plan_timecode(
    const YAML::Node& node, const std::string& field) {
  if (!node) {
    return std::optional<JobPlanTimecode>{};
  }
  DAILYBOY_ASSIGN_OR_RETURN(const YAML::Node map, expect_map(node, field));
  for (const auto& kv : map) {
    const std::string key = kv.first.as<std::string>();
    if (key != "start" && key != "drop_frame") {
      return Status::User(
          with_job_error(USER_ERROR_JOB_40, field + "." + key + "."));
    }
  }
  if (!map["start"]) {
    return Status::User(with_job_error(USER_ERROR_JOB_101, field + ".start."));
  }
  JobPlanTimecode timecode;
  DAILYBOY_ASSIGN_OR_RETURN(
      JobPlanTimecode::Start start,
      parse_timecode_start(map["start"], field + ".start"));
  timecode.set_start(std::move(start));
  DAILYBOY_ASSIGN_OR_RETURN(const bool drop_frame, read_drop_frame(map, field));
  timecode.set_drop_frame(drop_frame);
  if (std::holds_alternative<std::string>(timecode.start())) {
    const std::string& text = std::get<std::string>(timecode.start());
    const bool has_semicolon = text.find(';') != std::string::npos;
    if (has_semicolon && !drop_frame) {
      return Status::User(
          with_job_error(USER_ERROR_JOB_102, field + ".start."));
    }
  }
  return std::optional<JobPlanTimecode>{std::move(timecode)};
}

/*!
 * \brief Rejects a sequence missing \c path, \c frame_start, or \c frame_end.
 */
Status require_sequence_keys(const YAML::Node& map, const std::string& field) {
  if (!map["path"] || map["path"].IsNull()) {
    return Status::User(with_job_error(USER_ERROR_JOB_123, field + ".path."));
  }
  if (!map["frame_start"] || map["frame_start"].IsNull()) {
    return Status::User(
        with_job_error(USER_ERROR_JOB_125, field + ".frame_start."));
  }
  if (!map["frame_end"] || map["frame_end"].IsNull()) {
    return Status::User(
        with_job_error(USER_ERROR_JOB_127, field + ".frame_end."));
  }
  return Status::Ok();
}

/*!
 * \brief Parses \c plans[].sequence (pattern, hero range, optional handles).
 */
StatusOr<JobSequence> parse_plan_sequence(const YAML::Node& node,
                                          const std::string& field) {
  DAILYBOY_ASSIGN_OR_RETURN(const YAML::Node map, expect_map(node, field));
  DAILYBOY_RETURN_IF_ERROR(require_sequence_keys(map, field));
  DAILYBOY_ASSIGN_OR_RETURN(
      std::string path, read_quoted_nonempty_string(
                            map["path"], USER_ERROR_JOB_124, field + ".path"));
  DAILYBOY_ASSIGN_OR_RETURN(
      const int frame_start,
      read_yaml_integer(map["frame_start"], USER_ERROR_JOB_126,
                        field + ".frame_start"));
  DAILYBOY_ASSIGN_OR_RETURN(
      const int frame_end,
      read_yaml_integer(map["frame_end"], USER_ERROR_JOB_128,
                        field + ".frame_end"));
  JobSequence out;
  out.set_path(std::move(path));
  out.set_frame_start(frame_start);
  out.set_frame_end(frame_end);
  DAILYBOY_RETURN_IF_ERROR(
      parse_sequence_handles(map["handles"], field + ".handles", out));
  return out;
}

/*!
 * \brief Rejects a plan missing \c id, \c input_colorspace, or \c sequence.
 */
Status require_plan_keys(const YAML::Node& map, const std::string& field) {
  if (!map["id"] || map["id"].IsNull()) {
    return Status::User(with_job_error(USER_ERROR_JOB_118, field + ".id."));
  }
  if (!map["input_colorspace"] || map["input_colorspace"].IsNull()) {
    return Status::User(
        with_job_error(USER_ERROR_JOB_120, field + ".input_colorspace."));
  }
  if (!map["sequence"] || map["sequence"].IsNull()) {
    return Status::User(
        with_job_error(USER_ERROR_JOB_122, field + ".sequence."));
  }
  return Status::Ok();
}

StatusOr<JobPlan> parse_plan(const YAML::Node& node, const std::string& base) {
  DAILYBOY_ASSIGN_OR_RETURN(const YAML::Node map, expect_map(node, base));
  DAILYBOY_RETURN_IF_ERROR(require_plan_keys(map, base));
  DAILYBOY_ASSIGN_OR_RETURN(
      std::string id,
      read_quoted_nonempty_string(map["id"], USER_ERROR_JOB_119, base + ".id"));
  DAILYBOY_ASSIGN_OR_RETURN(
      std::string input_colorspace,
      read_quoted_nonempty_string(map["input_colorspace"], USER_ERROR_JOB_121,
                                  base + ".input_colorspace"));
  DAILYBOY_ASSIGN_OR_RETURN(
      JobSequence sequence,
      parse_plan_sequence(map["sequence"], base + ".sequence"));
  DAILYBOY_ASSIGN_OR_RETURN(std::optional<JobPlanAudio> audio,
                            parse_plan_audio(map["audio"], base + ".audio"));
  DAILYBOY_ASSIGN_OR_RETURN(
      std::optional<JobPlanTimecode> timecode,
      parse_plan_timecode(map["timecode"], base + ".timecode"));
  JobPlan out;
  out.set_id(std::move(id));
  out.set_input_colorspace(std::move(input_colorspace));
  out.set_sequence(std::move(sequence));
  out.set_audio(std::move(audio));
  out.set_timecode(std::move(timecode));
  return out;
}

}  // namespace

StatusOr<JobPlans> parse_plans(const YAML::Node& node) {
  DAILYBOY_ASSIGN_OR_RETURN(const YAML::Node seq,
                            expect_sequence(node, "plans"));
  std::vector<JobPlan> plans;
  plans.reserve(seq.size());
  for (std::size_t i = 0; i < seq.size(); ++i) {
    DAILYBOY_ASSIGN_OR_RETURN(
        JobPlan plan, parse_plan(seq[i], "plans[" + std::to_string(i) + "]"));
    plans.push_back(std::move(plan));
  }
  JobPlans out;
  out.set_plans(std::move(plans));
  return out;
}

}  // namespace dailyboy
