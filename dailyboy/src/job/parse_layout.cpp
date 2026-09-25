/*!
 * \file parse_layout.cpp
 * \brief Parse job YAML layout (canvas, image, burn-ins, slate).
 */

#include "job/parse_layout.hpp"

#include <filesystem>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "job/text.hpp"
#include "job/yaml_read.hpp"

namespace dailyboy {

StatusOr<TextPositionModeLayout::Anchor> parse_anchor(
    const std::string& value, const std::string& field) {
  if (value == "top_left") {
    return TextPositionModeLayout::Anchor::TopLeft;
  }
  if (value == "top_center") {
    return TextPositionModeLayout::Anchor::TopCenter;
  }
  if (value == "top_right") {
    return TextPositionModeLayout::Anchor::TopRight;
  }
  if (value == "center_left") {
    return TextPositionModeLayout::Anchor::CenterLeft;
  }
  if (value == "center_center") {
    return TextPositionModeLayout::Anchor::CenterCenter;
  }
  if (value == "center_right") {
    return TextPositionModeLayout::Anchor::CenterRight;
  }
  if (value == "bottom_left") {
    return TextPositionModeLayout::Anchor::BottomLeft;
  }
  if (value == "bottom_center") {
    return TextPositionModeLayout::Anchor::BottomCenter;
  }
  if (value == "bottom_right") {
    return TextPositionModeLayout::Anchor::BottomRight;
  }
  return Status::User(
      with_job_error(USER_ERROR_JOB_19, field + " '" + value + "'."));
}

StatusOr<TextPosition> parse_text_position(const YAML::Node& node,
                                           const std::string& field,
                                           int canvas_width,
                                           int canvas_height) {
  DAILYBOY_ASSIGN_OR_RETURN(const YAML::Node map, expect_map(node, field));
  if (!map["mode"] || map["mode"].IsNull()) {
    return Status::User(with_job_error(USER_ERROR_JOB_136, field + ".mode."));
  }
  DAILYBOY_ASSIGN_OR_RETURN(
      const std::string mode,
      read_quoted_nonempty_string(map["mode"], USER_ERROR_JOB_77,
                                  field + ".mode"));
  for (const auto& kv : map) {
    const std::string key = kv.first.as<std::string>();
    if (key != "mode" && key != "anchor" && key != "x" && key != "y") {
      std::string detail = field;
      detail += ".";
      detail += key;
      detail += ".";
      return Status::User(with_job_error(USER_ERROR_JOB_40, detail));
    }
  }
  if (mode != "layout" && mode != "pixel" && mode != "percent") {
    return Status::User(
        with_job_error(USER_ERROR_JOB_20, field + " '" + mode + "'."));
  }

  const bool has_anchor = map["anchor"] && !map["anchor"].IsNull();
  const bool has_x = map["x"] && !map["x"].IsNull();
  const bool has_y = map["y"] && !map["y"].IsNull();
  if (mode != "layout" && has_anchor) {
    return Status::User(with_job_error(USER_ERROR_JOB_79));
  }
  if (mode == "layout" && (has_x || has_y)) {
    return Status::User(with_job_error(USER_ERROR_JOB_80));
  }

  TextPosition out;
  if (mode == "layout") {
    if (!has_anchor) {
      return Status::User(
          with_job_error(USER_ERROR_JOB_11, field + " 'anchor'."));
    }
    DAILYBOY_ASSIGN_OR_RETURN(
        const std::string anchor,
        read_quoted_nonempty_string(map["anchor"], USER_ERROR_JOB_78,
                                    field + ".anchor"));
    DAILYBOY_ASSIGN_OR_RETURN(TextPositionModeLayout::Anchor parsed,
                              parse_anchor(anchor, field));
    TextPositionModeLayout v;
    v.set_anchor(parsed);
    out.set_mode(TextPosition::Mode::Layout);
    out.set_value(v);
    return out;
  }

  if (!has_x) {
    return Status::User(with_job_error(USER_ERROR_JOB_11, field + " 'x'."));
  }
  if (!has_y) {
    return Status::User(with_job_error(USER_ERROR_JOB_11, field + " 'y'."));
  }
  DAILYBOY_ASSIGN_OR_RETURN(
      int x, read_yaml_integer(map["x"], USER_ERROR_JOB_81, field + ".x"));
  DAILYBOY_ASSIGN_OR_RETURN(
      int y, read_yaml_integer(map["y"], USER_ERROR_JOB_82, field + ".y"));
  if (mode == "pixel") {
    if (x < 0 || x > canvas_width) {
      return Status::User(with_job_error(
          USER_ERROR_JOB_83,
          field + ".x (got integer " + std::to_string(x) + ")."));
    }
    if (y < 0 || y > canvas_height) {
      return Status::User(with_job_error(
          USER_ERROR_JOB_84,
          field + ".y (got integer " + std::to_string(y) + ")."));
    }
    TextPositionModePixel v;
    v.set_x(x);
    v.set_y(y);
    out.set_mode(TextPosition::Mode::Pixel);
    out.set_value(v);
    return out;
  }

  if (x < 0 || x > 100) {
    return Status::User(
        with_job_error(USER_ERROR_JOB_85,
                       field + ".x (got integer " + std::to_string(x) + ")."));
  }
  if (y < 0 || y > 100) {
    return Status::User(
        with_job_error(USER_ERROR_JOB_86,
                       field + ".y (got integer " + std::to_string(y) + ")."));
  }
  TextPositionModePercent v;
  v.set_x(static_cast<double>(x));
  v.set_y(static_cast<double>(y));
  out.set_mode(TextPosition::Mode::Percent);
  out.set_value(v);
  return out;
}

/*!
 * \brief Reads an unquoted YAML number in [0, 1].
 */
StatusOr<double> read_unit_interval_channel(const YAML::Node& node,
                                            std::string_view error_text,
                                            const std::string& loc) {
  DAILYBOY_ASSIGN_OR_RETURN(double value,
                            read_yaml_number(node, error_text, loc));
  if (value < 0.0 || value > 1.0) {
    return Status::User(with_job_error(
        error_text, loc + " (got number " + node.as<std::string>() + ")."));
  }
  return value;
}

/*!
 * \brief Rejects color keys other than \c r, \c g, and \c b.
 */
Status require_rgb_keys_only(const YAML::Node& map, const std::string& field) {
  for (const auto& kv : map) {
    const std::string key = kv.first.as<std::string>();
    if (key != "r" && key != "g" && key != "b") {
      return Status::User(
          with_job_error(USER_ERROR_JOB_40, field + "." + key + "."));
    }
  }
  return Status::Ok();
}

/*!
 * \brief Reads one color channel, or \a missing_channel when the key is absent.
 */
StatusOr<double> read_rgb_channel(const YAML::Node& map, const char* key,
                                  const std::string& field,
                                  std::string_view channel_error,
                                  double missing_channel) {
  const YAML::Node node = map[key];
  if (!node) {
    return missing_channel;
  }
  return read_unit_interval_channel(node, channel_error, field + "." + key);
}

/*!
 * \brief Reads \c r, \c g, and \c b from a map whose keys are already checked.
 */
StatusOr<RGBColor> read_rgb_channels(const YAML::Node& map,
                                     const std::string& field,
                                     std::string_view channel_error,
                                     double missing_channel) {
  DAILYBOY_ASSIGN_OR_RETURN(
      const double r,
      read_rgb_channel(map, "r", field, channel_error, missing_channel));
  DAILYBOY_ASSIGN_OR_RETURN(
      const double g,
      read_rgb_channel(map, "g", field, channel_error, missing_channel));
  DAILYBOY_ASSIGN_OR_RETURN(
      const double b,
      read_rgb_channel(map, "b", field, channel_error, missing_channel));
  return RGBColor(r, g, b);
}

/*!
 * \brief Parses an optional \c r/\c g/\c b map with channels in [0, 1].
 * \param map_error Error text when the node is present but not a map.
 * \param channel_error Error text when a channel is not a number in [0, 1].
 * \param missing_channel Value used for an absent block or channel.
 */
StatusOr<RGBColor> parse_unit_rgb_color(const YAML::Node& node,
                                        const std::string& field,
                                        std::string_view map_error,
                                        std::string_view channel_error,
                                        double missing_channel) {
  if (!node || node.IsNull()) {
    return RGBColor(missing_channel, missing_channel, missing_channel);
  }
  if (!node.IsMap()) {
    return Status::User(with_job_error(
        map_error, field + " is " + describe_yaml_value(node) + "."));
  }
  DAILYBOY_RETURN_IF_ERROR(require_rgb_keys_only(node, field));
  return read_rgb_channels(node, field, channel_error, missing_channel);
}

StatusOr<RGBColor> parse_rgb_color(const YAML::Node& node,
                                   const std::string& field,
                                   double missing_channel) {
  return parse_unit_rgb_color(node, field, USER_ERROR_JOB_70, USER_ERROR_JOB_71,
                              missing_channel);
}

/*!
 * \brief Parses optional text \c font.color (channels default to 1).
 */
StatusOr<RGBColor> parse_font_color(const YAML::Node& node,
                                    const std::string& field) {
  return parse_unit_rgb_color(node, field, USER_ERROR_JOB_90, USER_ERROR_JOB_91,
                              1.0);
}

/*!
 * \brief Reads \c font.path and checks the file exists when it has no tokens.
 *
 * Paths holding \c {token} or \c ${VAR} are only resolved at render time, so
 * they cannot be checked here.
 */
StatusOr<std::string> read_font_path(const YAML::Node& node,
                                     const std::string& field) {
  DAILYBOY_ASSIGN_OR_RETURN(
      std::string path,
      read_quoted_nonempty_string(node, USER_ERROR_JOB_87, field + ".path"));
  const bool has_token = path.find('{') != std::string::npos ||
                         path.find('$') != std::string::npos;
  if (!has_token && !std::filesystem::exists(path)) {
    return Status::User(
        with_job_error(USER_ERROR_JOB_139, field + ".path '" + path + "'."));
  }
  return path;
}

/*!
 * \brief Reads \c font.size_px: an unquoted integer \c >= 4.
 */
StatusOr<int> read_font_size_px(const YAML::Node& node,
                                const std::string& field) {
  DAILYBOY_ASSIGN_OR_RETURN(
      const int size_px,
      read_yaml_integer(node, USER_ERROR_JOB_88, field + ".size_px"));
  if (size_px < 4) {
    return Status::User(with_job_error(
        USER_ERROR_JOB_89,
        field + ".size_px (got integer " + std::to_string(size_px) + ")."));
  }
  return size_px;
}

StatusOr<TextFont> parse_text_font(const YAML::Node& node,
                                   const std::string& field) {
  DAILYBOY_ASSIGN_OR_RETURN(const YAML::Node map, expect_map(node, field));
  if (!map["path"] || map["path"].IsNull()) {
    return Status::User(with_job_error(USER_ERROR_JOB_137, field + ".path."));
  }
  if (!map["size_px"] || map["size_px"].IsNull()) {
    return Status::User(
        with_job_error(USER_ERROR_JOB_138, field + ".size_px."));
  }
  DAILYBOY_ASSIGN_OR_RETURN(std::string font_path,
                            read_font_path(map["path"], field));
  DAILYBOY_ASSIGN_OR_RETURN(const int size_px,
                            read_font_size_px(map["size_px"], field));
  DAILYBOY_ASSIGN_OR_RETURN(const RGBColor color,
                            parse_font_color(map["color"], field + ".color"));
  TextFont out;
  out.set_path(std::move(font_path));
  out.set_size_px(size_px);
  out.set_color(color);
  return out;
}

/*!
 * \brief Rejects margin keys other than the four sides.
 */
Status require_margin_keys_only(const YAML::Node& map,
                                const std::string& field) {
  for (const auto& kv : map) {
    const std::string key = kv.first.as<std::string>();
    if (key != "top" && key != "right" && key != "bottom" && key != "left") {
      return Status::User(
          with_job_error(USER_ERROR_JOB_40, field + "." + key + "."));
    }
  }
  return Status::Ok();
}

/*!
 * \brief Reads one margin side, or \c 0 when the key is absent.
 * \param type_error Error text when the side is not an unquoted integer.
 * \param range_error Error text when the side is negative.
 */
StatusOr<int> read_margin_side(const YAML::Node& map, const char* key,
                               const std::string& field,
                               std::string_view type_error,
                               std::string_view range_error) {
  const YAML::Node node = map[key];
  if (!node) {
    return 0;
  }
  const std::string loc = field + "." + key;
  DAILYBOY_ASSIGN_OR_RETURN(const int value,
                            read_yaml_integer(node, type_error, loc));
  if (value < 0) {
    return Status::User(with_job_error(
        range_error, loc + " (got integer " + std::to_string(value) + ")."));
  }
  return value;
}

/*!
 * \brief Reads a per-side margin map; omitted sides stay 0.
 */
StatusOr<Margin> parse_margin_sides(const YAML::Node& map,
                                    const std::string& field,
                                    std::string_view type_error,
                                    std::string_view range_error) {
  DAILYBOY_RETURN_IF_ERROR(require_margin_keys_only(map, field));
  Margin out;
  DAILYBOY_ASSIGN_OR_RETURN(
      const int top,
      read_margin_side(map, "top", field, type_error, range_error));
  DAILYBOY_ASSIGN_OR_RETURN(
      const int right,
      read_margin_side(map, "right", field, type_error, range_error));
  DAILYBOY_ASSIGN_OR_RETURN(
      const int bottom,
      read_margin_side(map, "bottom", field, type_error, range_error));
  DAILYBOY_ASSIGN_OR_RETURN(
      const int left,
      read_margin_side(map, "left", field, type_error, range_error));
  out.set_top(top);
  out.set_right(right);
  out.set_bottom(bottom);
  out.set_left(left);
  return out;
}

/*!
 * \brief Reads a scalar \c box.margin applied to all four sides.
 */
StatusOr<JobLayoutBurnInBoxMargin> read_uniform_box_margin(
    const YAML::Node& node, const std::string& field) {
  DAILYBOY_ASSIGN_OR_RETURN(const int all,
                            read_yaml_integer(node, USER_ERROR_JOB_109, field));
  if (all < 0) {
    return Status::User(
        with_job_error(USER_ERROR_JOB_109,
                       field + " (got integer " + std::to_string(all) + ")."));
  }
  JobLayoutBurnInBoxMargin out;
  out.set_top(all);
  out.set_right(all);
  out.set_bottom(all);
  out.set_left(all);
  return out;
}

StatusOr<JobLayoutBurnInBoxMargin> parse_box_margin(const YAML::Node& node,
                                                    const std::string& field) {
  if (!node || node.IsNull()) {
    return JobLayoutBurnInBoxMargin();
  }
  if (node.IsScalar()) {
    return read_uniform_box_margin(node, field);
  }
  if (!node.IsMap()) {
    return Status::User(with_job_error(
        USER_ERROR_JOB_109, field + " is " + describe_yaml_value(node) + "."));
  }
  return parse_margin_sides(node, field, USER_ERROR_JOB_109,
                            USER_ERROR_JOB_109);
}

/*!
 * \brief Parses optional \c layout.image.min_margin_px (sides default to 0).
 */
StatusOr<Margin> parse_side_insets_px(const YAML::Node& node,
                                      const std::string& field) {
  if (!node || node.IsNull()) {
    return Margin();
  }
  if (!node.IsMap()) {
    return Status::User(with_job_error(
        USER_ERROR_JOB_67, field + " is " + describe_yaml_value(node) + "."));
  }
  return parse_margin_sides(node, field, USER_ERROR_JOB_68, USER_ERROR_JOB_69);
}

/*!
 * \brief Parses optional \c layout.background (channels default to 0).
 */
StatusOr<JobLayoutBackground> parse_layout_background(const YAML::Node& node) {
  return parse_rgb_color(node, "layout.background", 0.0);
}

/*!
 * \brief Parses optional \c box.mode; defaults to fill when the key is absent.
 */
StatusOr<JobLayoutBurnInBox::JobLayoutBurnInBoxModeValue> parse_box_mode(
    const YAML::Node& node, const std::string& field) {
  if (!node || node.IsNull()) {
    return JobLayoutBurnInBox::JobLayoutBurnInBoxModeValue::Fill;
  }
  DAILYBOY_ASSIGN_OR_RETURN(
      const std::string mode,
      read_quoted_nonempty_string(node, USER_ERROR_JOB_105, field + ".mode"));
  if (mode == "fill") {
    return JobLayoutBurnInBox::JobLayoutBurnInBoxModeValue::Fill;
  }
  if (mode == "outline") {
    return JobLayoutBurnInBox::JobLayoutBurnInBoxModeValue::Outline;
  }
  return Status::User(
      with_job_error(USER_ERROR_JOB_21, field + ".mode '" + mode + "'."));
}

/*!
 * \brief Reads optional \c box.opacity: a number in [0, 1], default 1.
 */
StatusOr<double> read_box_opacity(const YAML::Node& map,
                                  const std::string& field) {
  const YAML::Node node = map["opacity"];
  if (!node) {
    return 1.0;
  }
  return read_unit_interval_channel(node, USER_ERROR_JOB_108,
                                    field + ".opacity");
}

StatusOr<JobLayoutBurnInBox> parse_burnin_box(const YAML::Node& node,
                                              const std::string& field) {
  DAILYBOY_ASSIGN_OR_RETURN(const YAML::Node map, expect_map(node, field));
  DAILYBOY_ASSIGN_OR_RETURN(
      const JobLayoutBurnInBox::JobLayoutBurnInBoxModeValue mode,
      parse_box_mode(map["mode"], field));
  DAILYBOY_ASSIGN_OR_RETURN(
      const JobLayoutBurnInBoxColor color,
      parse_unit_rgb_color(map["color"], field + ".color", USER_ERROR_JOB_106,
                           USER_ERROR_JOB_107, 0.0));
  DAILYBOY_ASSIGN_OR_RETURN(const double opacity, read_box_opacity(map, field));
  DAILYBOY_ASSIGN_OR_RETURN(const JobLayoutBurnInBoxMargin margin,
                            parse_box_margin(map["margin"], field + ".margin"));
  JobLayoutBurnInBox out;
  out.set_mode(mode);
  out.set_color(color);
  out.set_opacity(opacity);
  out.set_margin(margin);
  return out;
}

StatusOr<JobLayoutBurnIn> parse_burnin(const YAML::Node& node,
                                       const std::string& field,
                                       int canvas_width, int canvas_height) {
  DAILYBOY_ASSIGN_OR_RETURN(const YAML::Node map, expect_map(node, field));
  JobLayoutBurnIn out;
  if (!map["template"] || map["template"].IsNull()) {
    return Status::User(with_job_error(USER_ERROR_JOB_73));
  }
  if (!map["position"] || map["position"].IsNull()) {
    return Status::User(with_job_error(USER_ERROR_JOB_74));
  }
  if (!map["font"] || map["font"].IsNull()) {
    return Status::User(with_job_error(USER_ERROR_JOB_75));
  }
  DAILYBOY_ASSIGN_OR_RETURN(
      std::string template_text,
      read_quoted_nonempty_string(map["template"], USER_ERROR_JOB_76,
                                  field + ".template"));
  DAILYBOY_ASSIGN_OR_RETURN(
      TextPosition position,
      parse_text_position(map["position"], field + ".position", canvas_width,
                          canvas_height));
  DAILYBOY_ASSIGN_OR_RETURN(TextFont font,
                            parse_text_font(map["font"], field + ".font"));
  out.set_template_text(std::move(template_text));
  out.set_position(position);
  out.set_font(std::move(font));
  if (map["box"] && !map["box"].IsNull()) {
    DAILYBOY_ASSIGN_OR_RETURN(JobLayoutBurnInBox box,
                              parse_burnin_box(map["box"], field + ".box"));
    out.set_box(box);
  }
  return out;
}

/*!
 * \brief Rejects a slate line missing \c text, \c position, or \c font.
 */
Status require_slate_line_keys(const YAML::Node& map,
                               const std::string& field) {
  if (!map["text"] || map["text"].IsNull()) {
    return Status::User(with_job_error(USER_ERROR_JOB_114, field + ".text."));
  }
  if (!map["position"] || map["position"].IsNull()) {
    return Status::User(
        with_job_error(USER_ERROR_JOB_115, field + ".position."));
  }
  if (!map["font"] || map["font"].IsNull()) {
    return Status::User(with_job_error(USER_ERROR_JOB_116, field + ".font."));
  }
  return Status::Ok();
}

StatusOr<JobLayoutSlateLine> parse_slate_line(const YAML::Node& node,
                                              const std::string& field,
                                              int canvas_width,
                                              int canvas_height) {
  DAILYBOY_ASSIGN_OR_RETURN(const YAML::Node map, expect_map(node, field));
  DAILYBOY_RETURN_IF_ERROR(require_slate_line_keys(map, field));
  DAILYBOY_ASSIGN_OR_RETURN(
      std::string text, read_quoted_nonempty_string(
                            map["text"], USER_ERROR_JOB_117, field + ".text"));
  DAILYBOY_ASSIGN_OR_RETURN(
      const TextPosition position,
      parse_text_position(map["position"], field + ".position", canvas_width,
                          canvas_height));
  DAILYBOY_ASSIGN_OR_RETURN(TextFont font,
                            parse_text_font(map["font"], field + ".font"));
  JobLayoutSlateLine out;
  out.set_text(std::move(text));
  out.set_position(position);
  out.set_font(std::move(font));
  return out;
}

/*!
 * \brief Parses \c layout.slate.lines; the list must hold at least one line.
 */
StatusOr<std::vector<JobLayoutSlateLine>> parse_slate_lines(
    const YAML::Node& node, int canvas_width, int canvas_height) {
  DAILYBOY_ASSIGN_OR_RETURN(const YAML::Node seq,
                            expect_sequence(node, "layout.slate.lines"));
  if (seq.size() == 0) {
    return Status::User(with_job_error(USER_ERROR_JOB_113, "(got 0)."));
  }
  std::vector<JobLayoutSlateLine> lines;
  lines.reserve(seq.size());
  for (std::size_t i = 0; i < seq.size(); ++i) {
    DAILYBOY_ASSIGN_OR_RETURN(
        JobLayoutSlateLine line,
        parse_slate_line(seq[i],
                         "layout.slate.lines[" + std::to_string(i) + "]",
                         canvas_width, canvas_height));
    lines.push_back(std::move(line));
  }
  return lines;
}

/*!
 * \brief Reads \c layout.slate.duration_frames: an unquoted integer \c >= 0.
 */
StatusOr<int> read_slate_duration_frames(const YAML::Node& node) {
  DAILYBOY_ASSIGN_OR_RETURN(const int duration_frames,
                            read_yaml_integer(node, USER_ERROR_JOB_111,
                                              "layout.slate.duration_frames"));
  if (duration_frames < 0) {
    return Status::User(with_job_error(
        USER_ERROR_JOB_112,
        "(got integer " + std::to_string(duration_frames) + ")."));
  }
  return duration_frames;
}

/*!
 * \brief Parses the optional \c layout.slate block.
 */
StatusOr<JobLayoutSlate> parse_slate(const YAML::Node& node, int canvas_width,
                                     int canvas_height) {
  DAILYBOY_ASSIGN_OR_RETURN(const YAML::Node map,
                            expect_map(node, "layout.slate"));
  if (!map["duration_frames"] || map["duration_frames"].IsNull()) {
    return Status::User(with_job_error(USER_ERROR_JOB_110));
  }
  if (!map["lines"] || map["lines"].IsNull()) {
    return Status::User(with_job_error(USER_ERROR_JOB_113));
  }
  DAILYBOY_ASSIGN_OR_RETURN(const int duration_frames,
                            read_slate_duration_frames(map["duration_frames"]));
  DAILYBOY_ASSIGN_OR_RETURN(
      std::vector<JobLayoutSlateLine> lines,
      parse_slate_lines(map["lines"], canvas_width, canvas_height));
  JobLayoutSlate out;
  out.set_duration_frames(duration_frames);
  out.set_lines(std::move(lines));
  return out;
}

StatusOr<JobLayout> parse_layout(const YAML::Node& node) {
  DAILYBOY_ASSIGN_OR_RETURN(const YAML::Node map, expect_map(node, "layout"));
  JobLayout out;

  {
    if (!map["canvas"]) {
      return Status::User(with_job_error(USER_ERROR_JOB_54));
    }
    DAILYBOY_ASSIGN_OR_RETURN(const YAML::Node canvas_map,
                              expect_map(map["canvas"], "layout.canvas"));
    JobLayoutCanvas canvas;
    if (!canvas_map["width"]) {
      return Status::User(with_job_error(USER_ERROR_JOB_55));
    }
    if (!canvas_map["height"]) {
      return Status::User(with_job_error(USER_ERROR_JOB_57));
    }
    DAILYBOY_ASSIGN_OR_RETURN(
        int width, read_yaml_integer(canvas_map["width"], USER_ERROR_JOB_56,
                                     "layout.canvas.width"));
    DAILYBOY_ASSIGN_OR_RETURN(
        int height, read_yaml_integer(canvas_map["height"], USER_ERROR_JOB_58,
                                      "layout.canvas.height"));
    if (width < 2) {
      return Status::User(with_job_error(
          USER_ERROR_JOB_60, "(got integer " + std::to_string(width) + ")."));
    }
    if (height < 2) {
      return Status::User(with_job_error(
          USER_ERROR_JOB_61, "(got integer " + std::to_string(height) + ")."));
    }
    if ((width % 2) != 0) {
      return Status::User(
          with_job_error(USER_ERROR_JOB_1, "layout.canvas.width."));
    }
    if ((height % 2) != 0) {
      return Status::User(
          with_job_error(USER_ERROR_JOB_1, "layout.canvas.height."));
    }
    canvas.set_width(width);
    canvas.set_height(height);
    out.set_canvas(canvas);
  }

  {
    JobLayoutPixelAspect pixel_aspect;
    if (map["pixel_aspect"]) {
      DAILYBOY_ASSIGN_OR_RETURN(
          const double aspect,
          read_yaml_number(map["pixel_aspect"], USER_ERROR_JOB_59,
                           "layout.pixel_aspect"));
      if (aspect <= 0.0) {
        return Status::User(with_job_error(
            USER_ERROR_JOB_132, "layout.pixel_aspect (got number " +
                                    map["pixel_aspect"].as<std::string>() +
                                    ")."));
      }
      pixel_aspect.set_aspect(aspect);
    } else {
      pixel_aspect.set_aspect(1.0);
    }
    out.set_pixel_aspect(pixel_aspect);
  }

  {
    if (!map["image"]) {
      return Status::User(with_job_error(USER_ERROR_JOB_62));
    }
    DAILYBOY_ASSIGN_OR_RETURN(const YAML::Node image_map,
                              expect_map(map["image"], "layout.image"));
    JobLayoutImage image;
    if (!image_map["fit"]) {
      return Status::User(with_job_error(USER_ERROR_JOB_64));
    }
    DAILYBOY_ASSIGN_OR_RETURN(
        const std::string fit,
        read_quoted_nonempty_string(image_map["fit"], USER_ERROR_JOB_65,
                                    "layout.image.fit"));
    if (fit == "contain") {
      image.set_fit(JobLayoutImage::JobLayoutImageFitValue::Contain);
    } else if (fit == "cover") {
      image.set_fit(JobLayoutImage::JobLayoutImageFitValue::Cover);
    } else {
      return Status::User(with_job_error(USER_ERROR_JOB_22, "'" + fit + "'."));
    }

    if (image_map["filter"]) {
      DAILYBOY_ASSIGN_OR_RETURN(
          const std::string filter,
          read_quoted_nonempty_string(image_map["filter"], USER_ERROR_JOB_66,
                                      "layout.image.filter"));
      if (filter == "bilinear") {
        image.set_filter(JobLayoutImage::JobLayoutImageFilterValue::Bilinear);
      } else if (filter == "lanczos3") {
        image.set_filter(JobLayoutImage::JobLayoutImageFilterValue::Lanczos3);
      } else {
        return Status::User(
            with_job_error(USER_ERROR_JOB_23, "'" + filter + "'."));
      }
    } else {
      image.set_filter(JobLayoutImage::JobLayoutImageFilterValue::Lanczos3);
    }

    DAILYBOY_ASSIGN_OR_RETURN(
        Margin min_margin, parse_side_insets_px(image_map["min_margin_px"],
                                                "layout.image.min_margin_px"));
    image.set_min_margin_px(min_margin);
    out.set_image(image);
  }

  {
    DAILYBOY_ASSIGN_OR_RETURN(JobLayoutBackground bg,
                              parse_layout_background(map["background"]));
    out.set_background(bg);
  }

  {
    JobLayoutBurnIns burn_ins;
    std::vector<JobLayoutBurnIn> values;
    if (map["burn_ins"] && !map["burn_ins"].IsNull()) {
      DAILYBOY_ASSIGN_OR_RETURN(
          const YAML::Node seq,
          expect_sequence(map["burn_ins"], "layout.burn_ins"));
      if (seq.size() == 0) {
        return Status::User(with_job_error(USER_ERROR_JOB_72, "(got 0)."));
      }
      values.reserve(seq.size());
      for (std::size_t i = 0; i < seq.size(); ++i) {
        DAILYBOY_ASSIGN_OR_RETURN(
            JobLayoutBurnIn burnin,
            parse_burnin(seq[i], "layout.burn_ins[" + std::to_string(i) + "]",
                         out.canvas().width(), out.canvas().height()));
        values.push_back(std::move(burnin));
      }
    }
    burn_ins.set_burn_ins(std::move(values));
    out.set_burn_ins(std::move(burn_ins));
  }

  if (map["slate"] && !map["slate"].IsNull()) {
    DAILYBOY_ASSIGN_OR_RETURN(
        JobLayoutSlate slate,
        parse_slate(map["slate"], out.canvas().width(), out.canvas().height()));
    out.set_slate(std::move(slate));
  }

  return out;
}

}  // namespace dailyboy
