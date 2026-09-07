/*!
 * \file colorimetry.cpp
 * \brief OCIO transforms via OpenImageIO ColorConfig and prepared processors.
 */

#include "process/colorimetry.hpp"

#include <OpenImageIO/color.h>
#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>

#include <cstdlib>
#include <dailyboy/log.hpp>
#include <exception>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>

#include "error/color.hpp"
#include "process/path_tokens.hpp"
#include "status.hpp"

namespace dailyboy {

namespace {

constexpr const char* kPassthrough = "passthrough";

}  // namespace

struct ColorPipeline::Impl {
  std::unique_ptr<OIIO::ColorConfig> config;
  OIIO::ColorProcessorHandle input_to_working;
  std::map<std::string, OIIO::ColorProcessorHandle> working_to_display;
};

ColorPipeline::ColorPipeline() : impl_(std::make_unique<Impl>()) {}

ColorPipeline::ColorPipeline(ColorPipeline&&) noexcept = default;
ColorPipeline& ColorPipeline::operator=(ColorPipeline&&) noexcept = default;
ColorPipeline::~ColorPipeline() = default;

bool is_passthrough_display_view(const JobOutputDisplayView& display_view) {
  return display_view.display() == kPassthrough &&
         display_view.view() == kPassthrough;
}

std::string display_view_key(const JobOutputDisplayView& display_view) {
  return display_view.display() + "\n" + display_view.view();
}

namespace {

bool append_env_expansion(const std::string& path, std::size_t& i,
                          std::string& out) {
  if (path[i] != '$' || i + 1 >= path.size() || path[i + 1] != '{') {
    return false;
  }
  const std::size_t end = path.find('}', i + 2);
  if (end == std::string::npos) {
    return false;
  }
  const std::string var = path.substr(i + 2, end - (i + 2));
  const char* value = std::getenv(var.c_str());
  if (value != nullptr) {
    out += value;
  }
  i = end + 1;
  return true;
}

std::string expand_path_env(std::string path) {
  std::string out;
  out.reserve(path.size());
  for (std::size_t i = 0; i < path.size();) {
    if (!append_env_expansion(path, i, out)) {
      out += path[i];
      ++i;
    }
  }
  return out;
}

void append_context_pair(std::string& keys, std::string& values, bool& first,
                         const std::string& key, const std::string& value) {
  if (!first) {
    keys += ',';
    values += ',';
  }
  first = false;
  keys += key;
  values += value;
}

StatusOr<std::string> expand_color_string(const std::string& input,
                                          const Job& job) {
  if (input.empty()) {
    return input;
  }
  return expand_path_tokens(input, job.metadata().substitutions(),
                            USER_ERROR_COLOR_4);
}

Status context_csv(const std::map<std::string, std::string>& context,
                   const Job& job, std::string& keys, std::string& values) {
  keys.clear();
  values.clear();
  bool first = true;
  for (const auto& [key, value] : context) {
    DAILYBOY_ASSIGN_OR_RETURN(const std::string expanded,
                              expand_color_string(value, job));
    append_context_pair(keys, values, first, key, expanded);
  }
  return Status::Ok();
}

StatusOr<std::string> resolve_working(const Job& job) {
  std::string raw;
  if (!job.colorimetry().working_colorspace().empty()) {
    raw = job.colorimetry().working_colorspace();
  } else if (!job.plans().plans().empty()) {
    raw = job.plans().plans().front().input_colorspace();
  }
  return expand_color_string(raw, job);
}

StatusOr<std::string> resolve_input(const Job& job) {
  if (job.plans().plans().empty()) {
    return std::string{};
  }
  return expand_color_string(job.plans().plans().front().input_colorspace(),
                             job);
}

void collect_display_views(const Job& job, std::set<std::string>& keys,
                           std::map<std::string, JobOutputDisplayView>& views) {
  auto add = [&](const JobOutputDisplayView& display_view) {
    const std::string key = display_view_key(display_view);
    if (keys.insert(key).second) {
      views.emplace(key, display_view);
    }
  };
  for (const auto& video : job.output().videos().videos()) {
    if (video.enabled()) {
      add(video.display_view());
    }
  }
  for (const auto& item : job.output().image_sequences().image_sequences()) {
    if (item.enabled()) {
      add(item.display_view());
    }
  }
}

bool needs_transform(const std::string& from, const std::string& to) {
  return !from.empty() && !to.empty() && from != to;
}

bool any_real_display_view(
    const std::map<std::string, JobOutputDisplayView>& views) {
  for (const auto& [_, display_view] : views) {
    if (!is_passthrough_display_view(display_view)) {
      return true;
    }
  }
  return false;
}

bool any_transform_needed(
    const std::string& input, const std::string& working,
    const std::map<std::string, JobOutputDisplayView>& views) {
  return needs_transform(input, working) || any_real_display_view(views);
}

StatusOr<std::unique_ptr<OIIO::ColorConfig>> load_color_config(const Job& job) {
  const std::string path =
      expand_path_env(job.colorimetry().ocio_config().string());
  try {
    auto config = std::make_unique<OIIO::ColorConfig>(path);
    if (config->has_error()) {
      return Status::User(std::string(USER_ERROR_COLOR_1) + " " + path + ": " +
                          config->geterror());
    }
    return config;
  } catch (const std::exception& ex) {
    return Status::User(std::string(USER_ERROR_COLOR_1) + " " + path + ": " +
                        ex.what());
  }
}

Status processor_create_error(const std::string& from, const std::string& to,
                              const std::string& detail) {
  return Status::User(std::string(USER_ERROR_COLOR_2) + " " + from + " -> " +
                      to + ": " + detail);
}

StatusOr<OIIO::ColorProcessorHandle> make_processor(
    OIIO::ColorConfig& config, const std::string& from, const std::string& to,
    const std::string& context_keys, const std::string& context_values) {
  try {
    OIIO::ColorProcessorHandle processor =
        config.createColorProcessor(from, to, context_keys, context_values);
    if (!processor || config.has_error()) {
      return processor_create_error(from, to, config.geterror());
    }
    return processor;
  } catch (const std::exception& ex) {
    return processor_create_error(from, to, ex.what());
  }
}

StatusOr<OIIO::ColorProcessorHandle> make_display_processor(
    OIIO::ColorConfig& config, const std::string& working,
    const JobOutputDisplayView& display_view, const std::string& context_keys,
    const std::string& context_values) {
  try {
    OIIO::ColorProcessorHandle processor = config.createDisplayTransform(
        display_view.display(), display_view.view(), working, "", false,
        context_keys, context_values);
    if (!processor || config.has_error()) {
      return processor_create_error(
          working, display_view.display() + "/" + display_view.view(),
          config.geterror());
    }
    return processor;
  } catch (const std::exception& ex) {
    return processor_create_error(
        working, display_view.display() + "/" + display_view.view(), ex.what());
  }
}

Status apply_frame_convert(OIIO::ImageBuf& buf,
                           const OIIO::ColorProcessor* processor,
                           const char* label) {
  try {
    if (!OIIO::ImageBufAlgo::colorconvert(buf, buf, processor, false)) {
      return Status::User(std::string(USER_ERROR_COLOR_3) + " " + label + ": " +
                          buf.geterror());
    }
  } catch (const std::exception& ex) {
    return Status::User(std::string(USER_ERROR_COLOR_3) + " " + label + ": " +
                        ex.what());
  }
  return Status::Ok();
}

Status apply_frame(Frame& frame, const OIIO::ColorProcessorHandle& processor,
                   const char* label) {
  if (!processor) {
    log_debug(std::string("color: skip frame ") + label);
    return Status::Ok();
  }
  log_debug(std::string("color: frame ") + label);
  return apply_frame_convert(frame.buf(), processor.get(), label);
}

using DisplayProcessors = std::map<std::string, OIIO::ColorProcessorHandle>;

void emplace_passthrough_displays(
    DisplayProcessors& working_to_display,
    const std::map<std::string, JobOutputDisplayView>& views) {
  for (const auto& [key, _] : views) {
    working_to_display.emplace(key, OIIO::ColorProcessorHandle{});
  }
}

Status store_processor_if_needed(OIIO::ColorProcessorHandle& slot,
                                 OIIO::ColorConfig& config,
                                 const std::string& from, const std::string& to,
                                 const std::string& context_keys,
                                 const std::string& context_values,
                                 const char* label) {
  if (!needs_transform(from, to)) {
    return Status::Ok();
  }
  DAILYBOY_ASSIGN_OR_RETURN(
      slot, make_processor(config, from, to, context_keys, context_values));
  log_debug(std::string("color: prepare ") + label + " " + from + " -> " + to);
  return Status::Ok();
}

Status store_one_display_processor(DisplayProcessors& working_to_display,
                                   OIIO::ColorConfig& config,
                                   const std::string& working,
                                   const std::string& key,
                                   const JobOutputDisplayView& display_view,
                                   const std::string& context_keys,
                                   const std::string& context_values) {
  if (is_passthrough_display_view(display_view)) {
    working_to_display.emplace(key, OIIO::ColorProcessorHandle{});
    return Status::Ok();
  }
  DAILYBOY_ASSIGN_OR_RETURN(
      OIIO::ColorProcessorHandle processor,
      make_display_processor(config, working, display_view, context_keys,
                             context_values));
  log_debug("color: prepare W->display " + working + " -> " +
            display_view.display() + "/" + display_view.view());
  working_to_display.emplace(key, std::move(processor));
  return Status::Ok();
}

Status store_display_processors(
    DisplayProcessors& working_to_display, OIIO::ColorConfig& config,
    const std::string& working,
    const std::map<std::string, JobOutputDisplayView>& views,
    const std::string& context_keys, const std::string& context_values) {
  for (const auto& [key, display_view] : views) {
    DAILYBOY_RETURN_IF_ERROR(store_one_display_processor(
        working_to_display, config, working, key, display_view, context_keys,
        context_values));
  }
  return Status::Ok();
}

Status prepare_transforms(
    std::unique_ptr<OIIO::ColorConfig>& config_ptr,
    OIIO::ColorProcessorHandle& input_to_working,
    DisplayProcessors& working_to_display, const Job& job,
    const std::string& input, const std::string& working,
    const std::map<std::string, JobOutputDisplayView>& views) {
  DAILYBOY_ASSIGN_OR_RETURN(config_ptr, load_color_config(job));
  std::string context_keys;
  std::string context_values;
  DAILYBOY_RETURN_IF_ERROR(context_csv(job.colorimetry().context(), job,
                                       context_keys, context_values));
  OIIO::ColorConfig& config = *config_ptr;
  DAILYBOY_RETURN_IF_ERROR(
      store_processor_if_needed(input_to_working, config, input, working,
                                context_keys, context_values, "I->W"));
  return store_display_processors(working_to_display, config, working, views,
                                  context_keys, context_values);
}

}  // namespace

StatusOr<ColorPipeline> ColorPipeline::prepare(const Job& job) {
  DAILYBOY_ASSIGN_OR_RETURN(const std::string input, resolve_input(job));
  DAILYBOY_ASSIGN_OR_RETURN(const std::string working, resolve_working(job));
  std::set<std::string> keys;
  std::map<std::string, JobOutputDisplayView> views;
  collect_display_views(job, keys, views);
  ColorPipeline color_pipeline;
  if (!any_transform_needed(input, working, views)) {
    log_debug("color: prepare identity (no OCIO config load)");
    emplace_passthrough_displays(color_pipeline.impl_->working_to_display,
                                 views);
    return color_pipeline;
  }
  Impl& impl = *color_pipeline.impl_;
  DAILYBOY_RETURN_IF_ERROR(
      prepare_transforms(impl.config, impl.input_to_working,
                         impl.working_to_display, job, input, working, views));
  return color_pipeline;
}

Status convert_color_from_input_to_working(
    Frame& frame, const ColorPipeline& color_pipeline) {
  return apply_frame(frame, color_pipeline.impl_->input_to_working, "I->W");
}

Status convert_color_from_working_to_display(
    Frame& frame, const ColorPipeline& color_pipeline,
    const JobOutputDisplayView& display_view) {
  const std::string key = display_view_key(display_view);
  const auto found = color_pipeline.impl_->working_to_display.find(key);
  if (found == color_pipeline.impl_->working_to_display.end()) {
    if (color_pipeline.impl_->working_to_display.empty()) {
      log_debug("color: skip frame W->display (no prepared displays)");
      return Status::Ok();
    }
    return Status::User(std::string(USER_ERROR_COLOR_2) +
                        " unknown display_view: " + display_view.display() +
                        "/" + display_view.view());
  }
  return apply_frame(frame, found->second, "W->display");
}

}  // namespace dailyboy
