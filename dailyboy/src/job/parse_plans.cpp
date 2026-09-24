/*!
 * \file parse_plans.cpp
 * \brief Parse job YAML plans and sequences.
 */

#include "job/parse_plans.hpp"

#include <optional>
#include <string>

#include "error/job.hpp"
#include "job/yaml_read.hpp"

namespace dailyboy {

namespace {

StatusOr<int> read_handle_count(const YAML::Node& node,
                                const std::string& loc) {
  DAILYBOY_ASSIGN_OR_RETURN(int value,
                            read_yaml_integer(node, USER_ERROR_JOB_100, loc));
  if (value < 0) {
    return Status::User(
        with_job_error(USER_ERROR_JOB_100,
                       loc + " (got integer " + std::to_string(value) + ")."));
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
    DAILYBOY_ASSIGN_OR_RETURN(int head,
                              read_handle_count(node["head"], field + ".head"));
    sequence.set_handle_head(head);
  }
  if (node["tail"]) {
    DAILYBOY_ASSIGN_OR_RETURN(int tail,
                              read_handle_count(node["tail"], field + ".tail"));
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
  JobPlanAudio audio;
  DAILYBOY_ASSIGN_OR_RETURN(std::string path,
                            as_required<std::string>(map, "path", field));
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
  DAILYBOY_ASSIGN_OR_RETURN(bool drop_frame,
                            as_optional<bool>(map, "drop_frame", false, field));
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

}  // namespace

StatusOr<JobPlans> parse_plans(const YAML::Node& node) {
  JobPlans out;
  std::vector<JobPlan> plans;
  DAILYBOY_ASSIGN_OR_RETURN(const YAML::Node seq,
                            expect_sequence(node, "plans"));
  plans.reserve(seq.size());
  for (std::size_t i = 0; i < seq.size(); ++i) {
    const std::string base = "plans[" + std::to_string(i) + "]";
    DAILYBOY_ASSIGN_OR_RETURN(const YAML::Node map, expect_map(seq[i], base));
    JobPlan plan;
    DAILYBOY_ASSIGN_OR_RETURN(std::string id,
                              as_required<std::string>(map, "id", base));
    DAILYBOY_ASSIGN_OR_RETURN(
        std::string input_colorspace,
        as_required<std::string>(map, "input_colorspace", base));
    DAILYBOY_ASSIGN_OR_RETURN(const YAML::Node sequence_map,
                              expect_map(map["sequence"], base + ".sequence"));
    JobSequence sequence;
    DAILYBOY_ASSIGN_OR_RETURN(
        std::string path,
        as_required<std::string>(sequence_map, "path", base + ".sequence"));
    DAILYBOY_ASSIGN_OR_RETURN(
        int frame_start,
        as_required<int>(sequence_map, "frame_start", base + ".sequence"));
    DAILYBOY_ASSIGN_OR_RETURN(
        int frame_end,
        as_required<int>(sequence_map, "frame_end", base + ".sequence"));
    sequence.set_path(std::move(path));
    sequence.set_frame_start(frame_start);
    sequence.set_frame_end(frame_end);
    DAILYBOY_RETURN_IF_ERROR(parse_sequence_handles(
        sequence_map["handles"], base + ".sequence.handles", sequence));
    DAILYBOY_ASSIGN_OR_RETURN(std::optional<JobPlanAudio> audio,
                              parse_plan_audio(map["audio"], base + ".audio"));
    DAILYBOY_ASSIGN_OR_RETURN(
        std::optional<JobPlanTimecode> timecode,
        parse_plan_timecode(map["timecode"], base + ".timecode"));
    plan.set_id(std::move(id));
    plan.set_input_colorspace(std::move(input_colorspace));
    plan.set_sequence(std::move(sequence));
    plan.set_audio(std::move(audio));
    plan.set_timecode(std::move(timecode));
    plans.push_back(std::move(plan));
  }
  out.set_plans(std::move(plans));
  return out;
}

}  // namespace dailyboy
