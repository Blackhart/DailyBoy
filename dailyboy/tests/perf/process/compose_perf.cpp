/*!
 * \file compose_perf.cpp
 * \brief Google Benchmark timings for HD compose (example job geometry).
 */

#include <OpenImageIO/imagebuf.h>
#include <benchmark/benchmark.h>

#include "image/frame.hpp"
#include "job/job.hpp"
#include "job/layout.hpp"
#include "process/colorimetry.hpp"
#include "process/compositing.hpp"
#include "process/reformat.hpp"
#include "process/tokens.hpp"
#include "support/perf_frame.hpp"

namespace {

dailyboy::JobOutputDisplayView passthrough_display_view() {
  dailyboy::JobOutputDisplayView display_view;
  display_view.set_display("passthrough");
  display_view.set_view("passthrough");
  return display_view;
}

dailyboy::Job example_job(
    dailyboy::JobLayoutImage::JobLayoutImageFilterValue filter) {
  dailyboy::JobLayout layout;
  dailyboy::JobLayoutCanvas canvas;
  canvas.set_width(dailyboy::test::kHdWidth);
  canvas.set_height(dailyboy::test::kHdHeight);
  layout.set_canvas(std::move(canvas));
  dailyboy::JobLayoutImage image;
  image.set_fit(dailyboy::JobLayoutImage::JobLayoutImageFitValue::Contain);
  image.set_filter(filter);
  dailyboy::Margin margin;
  margin.set_top(50);
  margin.set_bottom(50);
  image.set_min_margin_px(std::move(margin));
  layout.set_image(std::move(image));
  layout.set_background(dailyboy::RGBColor(0.0, 0.0, 0.0));
  dailyboy::Job job;
  job.set_layout(std::move(layout));
  return job;
}

dailyboy::StatusOr<dailyboy::Frame> compose_once(
    const dailyboy::Frame& src, const dailyboy::Job& job,
    const dailyboy::ColorPipeline& color_pipeline) {
  dailyboy::Frame plate = dailyboy::test::copy_frame(src);
  dailyboy::Status status = dailyboy::reformat(plate);
  if (!status.ok()) {
    return status;
  }
  status = dailyboy::convert_color_from_input_to_working(plate, color_pipeline);
  if (!status.ok()) {
    return status;
  }
  dailyboy::StatusOr<dailyboy::FittedPlate> fitted =
      dailyboy::resize_plate(plate, job);
  if (!fitted.ok()) {
    return fitted.status();
  }
  status = dailyboy::convert_color_from_working_to_display(
      fitted.value().frame, color_pipeline, passthrough_display_view());
  if (!status.ok()) {
    return status;
  }
  dailyboy::StatusOr<dailyboy::Frame> canvas =
      dailyboy::make_filled_canvas(job);
  if (!canvas.ok()) {
    return canvas.status();
  }
  return dailyboy::compose(std::move(canvas).value(), fitted.value(), job,
                           dailyboy::OverlayTokenContext{});
}

void run_compose(benchmark::State& state, const dailyboy::Job& job,
                 const dailyboy::ColorPipeline& color_pipeline,
                 const dailyboy::Frame& src) {
  for (auto _ : state) {
    dailyboy::StatusOr<dailyboy::Frame> result =
        compose_once(src, job, color_pipeline);
    if (!result.ok()) {
      state.SkipWithError(result.status().message().c_str());
      return;
    }
    benchmark::DoNotOptimize(result.value().buf().localpixels());
    benchmark::ClobberMemory();
  }
}

}  // namespace

static void Compose_MatchingCanvas_IsNoOp(benchmark::State& state) {
  dailyboy::Job job = example_job(
      dailyboy::JobLayoutImage::JobLayoutImageFilterValue::Lanczos3);
  job.layout().image().min_margin_px() = dailyboy::Margin{};
  dailyboy::StatusOr<dailyboy::ColorPipeline> prepared =
      dailyboy::ColorPipeline::prepare(job);
  if (!prepared.ok()) {
    state.SkipWithError(prepared.status().message().c_str());
    return;
  }
  const dailyboy::ColorPipeline& color_pipeline = *prepared;
  dailyboy::Frame frame = dailyboy::test::hd_rgb_float();
  for (auto _ : state) {
    dailyboy::StatusOr<dailyboy::Frame> result =
        compose_once(frame, job, color_pipeline);
    if (!result.ok()) {
      state.SkipWithError(result.status().message().c_str());
      return;
    }
    benchmark::DoNotOptimize(result.value().buf().localpixels());
    benchmark::ClobberMemory();
  }
}
BENCHMARK(Compose_MatchingCanvas_IsNoOp)
    ->Unit(benchmark::kMillisecond)
    ->MinTime(0.05);

static void Compose_ExamplePlate_Lanczos3Contain(benchmark::State& state) {
  dailyboy::Job job = example_job(
      dailyboy::JobLayoutImage::JobLayoutImageFilterValue::Lanczos3);
  dailyboy::StatusOr<dailyboy::ColorPipeline> prepared =
      dailyboy::ColorPipeline::prepare(job);
  if (!prepared.ok()) {
    state.SkipWithError(prepared.status().message().c_str());
    return;
  }
  run_compose(state, job, *prepared, dailyboy::test::example_plate_float());
}
BENCHMARK(Compose_ExamplePlate_Lanczos3Contain)
    ->Unit(benchmark::kMillisecond)
    ->MinTime(0.05);

static void Compose_ExamplePlate_BilinearContain(benchmark::State& state) {
  dailyboy::Job job = example_job(
      dailyboy::JobLayoutImage::JobLayoutImageFilterValue::Bilinear);
  dailyboy::StatusOr<dailyboy::ColorPipeline> prepared =
      dailyboy::ColorPipeline::prepare(job);
  if (!prepared.ok()) {
    state.SkipWithError(prepared.status().message().c_str());
    return;
  }
  run_compose(state, job, *prepared, dailyboy::test::example_plate_float());
}
BENCHMARK(Compose_ExamplePlate_BilinearContain)
    ->Unit(benchmark::kMillisecond)
    ->MinTime(0.05);
