/*!
 * \file burn_ins_perf.cpp
 * \brief Google Benchmark timings for HD burn-in box drawing.
 */

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>
#include <OpenImageIO/imageio.h>
#include <benchmark/benchmark.h>

#include <optional>
#include <string>
#include <vector>

#include "image/frame.hpp"
#include "job/job.hpp"
#include "job/layout.hpp"
#include "job/primitives.hpp"
#include "job/text.hpp"
#include "process/burn_ins.hpp"
#include "process/colorimetry.hpp"
#include "process/compositing.hpp"
#include "process/tokens.hpp"
#include "support/test_fonts.hpp"

namespace {

constexpr int kHdWidth = 1920;
constexpr int kHdHeight = 1080;
constexpr float kGreen[3] = {0.0f, 1.0f, 0.0f};

OIIO::ImageBuf make_hd_green() {
  OIIO::ImageSpec spec(kHdWidth, kHdHeight, 3, OIIO::TypeDesc::FLOAT);
  OIIO::ImageBuf buf(spec);
  OIIO::ImageBufAlgo::fill(buf, OIIO::cspan<float>(kGreen, 3));
  return buf;
}

dailyboy::TextPosition pixel_position(int x, int y) {
  dailyboy::TextPositionModePixel pixel;
  pixel.set_x(x);
  pixel.set_y(y);
  dailyboy::TextPosition position;
  position.set_mode(dailyboy::TextPosition::Mode::Pixel);
  position.set_value(pixel);
  return position;
}

dailyboy::TextPosition layout_position(
    dailyboy::TextPositionModeLayout::Anchor anchor) {
  dailyboy::TextPositionModeLayout layout;
  layout.set_anchor(anchor);
  dailyboy::TextPosition position;
  position.set_mode(dailyboy::TextPosition::Mode::Layout);
  position.set_value(layout);
  return position;
}

dailyboy::JobLayoutBurnInBox fill_box(double opacity) {
  dailyboy::JobLayoutBurnInBox box;
  box.set_mode(dailyboy::JobLayoutBurnInBox::JobLayoutBurnInBoxModeValue::Fill);
  box.set_color(dailyboy::RGBColor(0.0, 0.0, 0.0));
  box.set_opacity(opacity);
  dailyboy::Margin pad;
  pad.set_top(6);
  pad.set_right(6);
  pad.set_bottom(6);
  pad.set_left(6);
  box.set_margin(std::move(pad));
  return box;
}

dailyboy::JobLayoutBurnInBox outline_box(double opacity) {
  dailyboy::JobLayoutBurnInBox box = fill_box(opacity);
  box.set_mode(
      dailyboy::JobLayoutBurnInBox::JobLayoutBurnInBoxModeValue::Outline);
  box.set_color(dailyboy::RGBColor(1.0, 1.0, 1.0));
  return box;
}

dailyboy::JobLayoutBurnIn make_burn_in(
    const std::string& text, dailyboy::TextPosition position,
    std::optional<dailyboy::JobLayoutBurnInBox> box) {
  dailyboy::JobLayoutBurnIn burn_in;
  burn_in.set_template_text(text);
  burn_in.set_position(std::move(position));
  dailyboy::TextFont font;
  font.set_path(dailyboy::test::kDejaVuSans);
  font.set_size_px(18);
  font.set_color(dailyboy::RGBColor(1.0, 1.0, 1.0));
  burn_in.set_font(std::move(font));
  burn_in.set_box(std::move(box));
  return burn_in;
}

dailyboy::Job make_job(std::vector<dailyboy::JobLayoutBurnIn> burn_ins) {
  dailyboy::Job job;
  dailyboy::JobLayoutBurnIns list;
  list.set_burn_ins(std::move(burn_ins));
  dailyboy::JobLayout layout;
  layout.set_burn_ins(std::move(list));
  job.set_layout(std::move(layout));
  return job;
}

dailyboy::OverlayTokenContext make_tokens() {
  dailyboy::OverlayTokenContext tokens;
  tokens.frame = 1001;
  tokens.frame_start = 1001;
  tokens.frame_end = 1048;
  tokens.source_file = "examples/frames/2048x1080/plate.1001.png";
  tokens.plan_id = "sh010_bg";
  return tokens;
}

void run_draw(benchmark::State& state, const dailyboy::Job& job) {
  dailyboy::StatusOr<dailyboy::ColorPipeline> prepared =
      dailyboy::ColorPipeline::prepare(job);
  if (!prepared.ok()) {
    state.SkipWithError(prepared.status().message().c_str());
    return;
  }
  const dailyboy::ColorPipeline& color_pipeline = *prepared;
  dailyboy::Frame frame(make_hd_green());
  const dailyboy::OverlayTokenContext tokens = make_tokens();
  for (auto _ : state) {
    OIIO::ImageBufAlgo::fill(frame.buf(), OIIO::cspan<float>(kGreen, 3));
    const dailyboy::Status status = dailyboy::draw_hud(frame, job, tokens);
    if (!status.ok()) {
      state.SkipWithError(status.message().c_str());
      return;
    }
    benchmark::DoNotOptimize(frame.buf().localpixels());
    benchmark::ClobberMemory();
  }
}

}  // namespace

static void DrawBurnIns_HdCanvas_FillBox(benchmark::State& state) {
  const dailyboy::Job job =
      make_job({make_burn_in("H", pixel_position(20, 12), fill_box(0.45))});
  run_draw(state, job);
}
BENCHMARK(DrawBurnIns_HdCanvas_FillBox)
    ->Unit(benchmark::kMillisecond)
    ->MinTime(0.05);

static void DrawBurnIns_HdCanvas_OutlineBox(benchmark::State& state) {
  const dailyboy::Job job =
      make_job({make_burn_in("H", pixel_position(20, 12), outline_box(0.85))});
  run_draw(state, job);
}
BENCHMARK(DrawBurnIns_HdCanvas_OutlineBox)
    ->Unit(benchmark::kMillisecond)
    ->MinTime(0.05);

static void DrawBurnIns_HdCanvas_SixBurnIns(benchmark::State& state) {
  using Anchor = dailyboy::TextPositionModeLayout::Anchor;
  const dailyboy::Job job = make_job({
      make_burn_in("DEMO_PROJECT  sh010  v003",
                   layout_position(Anchor::TopLeft), fill_box(0.45)),
      make_burn_in("Plan start", layout_position(Anchor::TopCenter),
                   fill_box(0.45)),
      make_burn_in("jdoe", layout_position(Anchor::TopRight),
                   outline_box(0.85)),
      make_burn_in("sh010  1001  plate.1001.png",
                   layout_position(Anchor::BottomLeft), fill_box(0.45)),
      make_burn_in("WIP lighting — not for distribution",
                   layout_position(Anchor::BottomCenter), std::nullopt),
      make_burn_in("1001-1048  sh010_bg", layout_position(Anchor::BottomRight),
                   fill_box(0.45)),
  });
  run_draw(state, job);
}
BENCHMARK(DrawBurnIns_HdCanvas_SixBurnIns)
    ->Unit(benchmark::kMillisecond)
    ->MinTime(0.05);
