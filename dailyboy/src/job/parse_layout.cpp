/*!
 * \file parse_layout.cpp
 * \brief Parse job YAML layout (canvas, image, burn-ins, slate).
 */

#include "job/parse_layout.hpp"

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
    return Status::User(with_job_error(USER_ERROR_JOB_11, field + " 'mode'."));
  }
  DAILYBOY_ASSIGN_OR_RETURN(
      const std::string mode,
      read_quoted_nonempty_string(map["mode"], USER_ERROR_JOB_77,
                                  field + ".mode"));
  for (const auto& kv : map) {
    const std::string key = kv.first.as<std::string>();
    if (key != "mode" && key != "anchor" && key != "x" && key != "y") {
      return Status::User(
          with_job_error(USER_ERROR_JOB_40, field + "." + key + "."));
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
 * \brief Parses optional \c r/\c g/\c b.
 *
 * Missing channels use \a missing_channel (0 for background/box, 1 for font).
 */
StatusOr<RGBColor> parse_rgb_color(const YAML::Node& node,
                                   const std::string& field,
                                   double missing_channel) {
  RGBColor color;
  color.set_r(missing_channel);
  color.set_g(missing_channel);
  color.set_b(missing_channel);
  if (!node || node.IsNull()) {
    return color;
  }
  DAILYBOY_ASSIGN_OR_RETURN(const YAML::Node map, expect_map(node, field));
  DAILYBOY_ASSIGN_OR_RETURN(
      double r, as_optional<double>(map, "r", missing_channel, field));
  DAILYBOY_ASSIGN_OR_RETURN(
      double g, as_optional<double>(map, "g", missing_channel, field));
  DAILYBOY_ASSIGN_OR_RETURN(
      double b, as_optional<double>(map, "b", missing_channel, field));
  color.set_r(r);
  color.set_g(g);
  color.set_b(b);
  return color;
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
 * \brief Parses optional burn-in \c font.color (channels default to 1).
 */
StatusOr<RGBColor> parse_font_color(const YAML::Node& node,
                                    const std::string& field) {
  RGBColor color;
  color.set_r(1.0);
  color.set_g(1.0);
  color.set_b(1.0);
  if (!node || node.IsNull()) {
    return color;
  }
  if (!node.IsMap()) {
    return Status::User(with_job_error(
        USER_ERROR_JOB_90, field + " is " + describe_yaml_value(node) + "."));
  }
  for (const auto& kv : node) {
    const std::string key = kv.first.as<std::string>();
    if (key != "r" && key != "g" && key != "b") {
      return Status::User(
          with_job_error(USER_ERROR_JOB_40, field + "." + key + "."));
    }
  }
  if (node["r"]) {
    DAILYBOY_ASSIGN_OR_RETURN(
        double r,
        read_unit_interval_channel(node["r"], USER_ERROR_JOB_91, field + ".r"));
    color.set_r(r);
  }
  if (node["g"]) {
    DAILYBOY_ASSIGN_OR_RETURN(
        double g,
        read_unit_interval_channel(node["g"], USER_ERROR_JOB_91, field + ".g"));
    color.set_g(g);
  }
  if (node["b"]) {
    DAILYBOY_ASSIGN_OR_RETURN(
        double b,
        read_unit_interval_channel(node["b"], USER_ERROR_JOB_91, field + ".b"));
    color.set_b(b);
  }
  return color;
}

StatusOr<TextFont> parse_text_font(const YAML::Node& node,
                                   const std::string& field) {
  DAILYBOY_ASSIGN_OR_RETURN(const YAML::Node map, expect_map(node, field));
  if (!map["path"] || map["path"].IsNull()) {
    return Status::User(with_job_error(USER_ERROR_JOB_11, field + " 'path'."));
  }
  if (!map["size_px"] || map["size_px"].IsNull()) {
    return Status::User(
        with_job_error(USER_ERROR_JOB_11, field + " 'size_px'."));
  }
  TextFont out;
  DAILYBOY_ASSIGN_OR_RETURN(
      std::string font_path,
      read_quoted_nonempty_string(map["path"], USER_ERROR_JOB_87,
                                  field + ".path"));
  DAILYBOY_ASSIGN_OR_RETURN(
      int size_px,
      read_yaml_integer(map["size_px"], USER_ERROR_JOB_88, field + ".size_px"));
  if (size_px < 4) {
    return Status::User(with_job_error(
        USER_ERROR_JOB_89,
        field + ".size_px (got integer " + std::to_string(size_px) + ")."));
  }
  out.set_path(std::move(font_path));
  out.set_size_px(size_px);
  DAILYBOY_ASSIGN_OR_RETURN(RGBColor color,
                            parse_font_color(map["color"], field + ".color"));
  out.set_color(std::move(color));
  return out;
}

StatusOr<JobLayoutBurnInBoxMargin> parse_box_margin(const YAML::Node& node,
                                                    const std::string& field) {
  JobLayoutBurnInBoxMargin out;
  if (!node || node.IsNull()) {
    return out;
  }
  if (node.IsScalar()) {
    int all = 0;
    try {
      all = node.as<int>();
    } catch (const YAML::Exception& ex) {
      return Status::User(
          with_job_error(USER_ERROR_JOB_18, field + " (" + ex.what() + ")."));
    }
    out.set_top(all);
    out.set_bottom(all);
    out.set_left(all);
    out.set_right(all);
    return out;
  }
  DAILYBOY_ASSIGN_OR_RETURN(const YAML::Node map, expect_map(node, field));
  DAILYBOY_ASSIGN_OR_RETURN(int top, as_optional<int>(map, "top", 0, field));
  DAILYBOY_ASSIGN_OR_RETURN(int bottom,
                            as_optional<int>(map, "bottom", 0, field));
  DAILYBOY_ASSIGN_OR_RETURN(int left, as_optional<int>(map, "left", 0, field));
  DAILYBOY_ASSIGN_OR_RETURN(int right,
                            as_optional<int>(map, "right", 0, field));
  out.set_top(top);
  out.set_bottom(bottom);
  out.set_left(left);
  out.set_right(right);
  return out;
}

/*!
 * \brief Reads one \c min_margin_px side: unquoted integer \c >= 0.
 */
StatusOr<int> read_min_margin_side(const YAML::Node& node,
                                   const std::string& loc) {
  DAILYBOY_ASSIGN_OR_RETURN(int value,
                            read_yaml_integer(node, USER_ERROR_JOB_68, loc));
  if (value < 0) {
    return Status::User(
        with_job_error(USER_ERROR_JOB_69,
                       loc + " (got integer " + std::to_string(value) + ")."));
  }
  return value;
}

/*!
 * \brief Parses optional \c layout.image.min_margin_px (object; sides default to 0).
 */
StatusOr<Margin> parse_side_insets_px(const YAML::Node& node,
                                      const std::string& field) {
  Margin out;
  if (!node || node.IsNull()) {
    return out;
  }
  if (!node.IsMap()) {
    return Status::User(with_job_error(
        USER_ERROR_JOB_67, field + " is " + describe_yaml_value(node) + "."));
  }
  for (const auto& kv : node) {
    const std::string key = kv.first.as<std::string>();
    if (key != "top" && key != "right" && key != "bottom" && key != "left") {
      return Status::User(
          with_job_error(USER_ERROR_JOB_40, field + "." + key + "."));
    }
  }
  if (node["top"]) {
    DAILYBOY_ASSIGN_OR_RETURN(
        int top, read_min_margin_side(node["top"], field + ".top"));
    out.set_top(top);
  }
  if (node["right"]) {
    DAILYBOY_ASSIGN_OR_RETURN(
        int right, read_min_margin_side(node["right"], field + ".right"));
    out.set_right(right);
  }
  if (node["bottom"]) {
    DAILYBOY_ASSIGN_OR_RETURN(
        int bottom, read_min_margin_side(node["bottom"], field + ".bottom"));
    out.set_bottom(bottom);
  }
  if (node["left"]) {
    DAILYBOY_ASSIGN_OR_RETURN(
        int left, read_min_margin_side(node["left"], field + ".left"));
    out.set_left(left);
  }
  return out;
}

/*!
 * \brief True when \a yaml_path is a \c layout.background channel.
 */
StatusOr<double> read_background_channel(const YAML::Node& node,
                                         const std::string& loc) {
  return read_unit_interval_channel(node, USER_ERROR_JOB_71, loc);
}

/*!
 * \brief Parses optional \c layout.background (object; channels default to 0).
 */
StatusOr<JobLayoutBackground> parse_layout_background(const YAML::Node& node) {
  JobLayoutBackground out;
  if (!node || node.IsNull()) {
    return out;
  }
  if (!node.IsMap()) {
    return Status::User(with_job_error(
        USER_ERROR_JOB_70,
        "layout.background is " + describe_yaml_value(node) + "."));
  }
  for (const auto& kv : node) {
    const std::string key = kv.first.as<std::string>();
    if (key != "r" && key != "g" && key != "b") {
      return Status::User(
          with_job_error(USER_ERROR_JOB_40, "layout.background." + key + "."));
    }
  }
  if (node["r"]) {
    DAILYBOY_ASSIGN_OR_RETURN(
        double r, read_background_channel(node["r"], "layout.background.r"));
    out.set_r(r);
  }
  if (node["g"]) {
    DAILYBOY_ASSIGN_OR_RETURN(
        double g, read_background_channel(node["g"], "layout.background.g"));
    out.set_g(g);
  }
  if (node["b"]) {
    DAILYBOY_ASSIGN_OR_RETURN(
        double b, read_background_channel(node["b"], "layout.background.b"));
    out.set_b(b);
  }
  return out;
}

StatusOr<JobLayoutBurnInBox> parse_burnin_box(const YAML::Node& node,
                                              const std::string& field) {
  JobLayoutBurnInBox out;
  DAILYBOY_ASSIGN_OR_RETURN(const YAML::Node map, expect_map(node, field));
  DAILYBOY_ASSIGN_OR_RETURN(
      const std::string mode,
      as_optional<std::string>(map, "mode", "fill", field));
  if (mode == "fill") {
    out.set_mode(JobLayoutBurnInBox::JobLayoutBurnInBoxModeValue::Fill);
  } else if (mode == "outline") {
    out.set_mode(JobLayoutBurnInBox::JobLayoutBurnInBoxModeValue::Outline);
  } else {
    return Status::User(
        with_job_error(USER_ERROR_JOB_21, field + " '" + mode + "'."));
  }

  JobLayoutBurnInBoxColor color;
  DAILYBOY_ASSIGN_OR_RETURN(
      color, parse_rgb_color(map["color"], field + ".color", 0.0));
  out.set_color(std::move(color));
  DAILYBOY_ASSIGN_OR_RETURN(double opacity,
                            as_optional<double>(map, "opacity", 1.0, field));
  out.set_opacity(opacity);
  DAILYBOY_ASSIGN_OR_RETURN(JobLayoutBurnInBoxMargin margin,
                            parse_box_margin(map["margin"], field + ".margin"));
  out.set_margin(std::move(margin));
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
  out.set_position(std::move(position));
  out.set_font(std::move(font));
  if (map["box"] && !map["box"].IsNull()) {
    DAILYBOY_ASSIGN_OR_RETURN(JobLayoutBurnInBox box,
                              parse_burnin_box(map["box"], field + ".box"));
    out.set_box(std::move(box));
  }
  return out;
}

StatusOr<JobLayoutSlateLine> parse_slate_line(const YAML::Node& node,
                                              const std::string& field,
                                              int canvas_width,
                                              int canvas_height) {
  DAILYBOY_ASSIGN_OR_RETURN(const YAML::Node map, expect_map(node, field));
  JobLayoutSlateLine out;
  DAILYBOY_ASSIGN_OR_RETURN(std::string text,
                            as_required<std::string>(map, "text", field));
  DAILYBOY_ASSIGN_OR_RETURN(
      TextPosition position,
      parse_text_position(map["position"], field + ".position", canvas_width,
                          canvas_height));
  DAILYBOY_ASSIGN_OR_RETURN(TextFont font,
                            parse_text_font(map["font"], field + ".font"));
  out.set_text(std::move(text));
  out.set_position(std::move(position));
  out.set_font(std::move(font));
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
    out.set_canvas(std::move(canvas));
  }

  {
    JobLayoutPixelAspect pixel_aspect;
    if (map["pixel_aspect"]) {
      DAILYBOY_ASSIGN_OR_RETURN(
          double aspect,
          read_yaml_number(map["pixel_aspect"], USER_ERROR_JOB_59,
                           "layout.pixel_aspect"));
      pixel_aspect.set_aspect(aspect);
    } else {
      pixel_aspect.set_aspect(1.0);
    }
    out.set_pixel_aspect(std::move(pixel_aspect));
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
    image.set_min_margin_px(std::move(min_margin));
    out.set_image(std::move(image));
  }

  {
    DAILYBOY_ASSIGN_OR_RETURN(JobLayoutBackground bg,
                              parse_layout_background(map["background"]));
    out.set_background(std::move(bg));
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
    DAILYBOY_ASSIGN_OR_RETURN(const YAML::Node slate_map,
                              expect_map(map["slate"], "layout.slate"));
    JobLayoutSlate slate;
    DAILYBOY_ASSIGN_OR_RETURN(
        int duration_frames,
        as_required<int>(slate_map, "duration_frames", "layout.slate"));
    slate.set_duration_frames(duration_frames);

    std::vector<JobLayoutSlateLine> lines;
    DAILYBOY_ASSIGN_OR_RETURN(
        const YAML::Node seq,
        expect_sequence(slate_map["lines"], "layout.slate.lines"));
    lines.reserve(seq.size());
    for (std::size_t i = 0; i < seq.size(); ++i) {
      DAILYBOY_ASSIGN_OR_RETURN(
          JobLayoutSlateLine line,
          parse_slate_line(seq[i],
                           "layout.slate.lines[" + std::to_string(i) + "]",
                           out.canvas().width(), out.canvas().height()));
      lines.push_back(std::move(line));
    }
    slate.set_lines(std::move(lines));
    out.set_slate(std::move(slate));
  }

  return out;
}

}  // namespace dailyboy
