/*!
 * \file reformat_perf.cpp
 * \brief Google Benchmark timings for one-frame HD reformat from on-disk EXRs.
 */

#include <benchmark/benchmark.h>

#include <filesystem>
#include <string>

#include "image/frame.hpp"
#include "process/reformat.hpp"
#include "support/perf_frame.hpp"

namespace {

bool load_reformat_exr(benchmark::State& state, const char* name,
                       dailyboy::Frame& out) {
  const std::filesystem::path path =
      dailyboy::test::perf_data_file((std::string("reformat/") + name).c_str());
  if (!dailyboy::test::load_perf_frame(path, out)) {
    const std::string err = "cannot read " + path.string();
    state.SkipWithError(err.c_str());
    return false;
  }
  return true;
}

void run_reformat_copy(benchmark::State& state, const dailyboy::Frame& src) {
  for (auto _ : state) {
    state.PauseTiming();
    dailyboy::Frame frame = dailyboy::test::copy_frame(src);
    state.ResumeTiming();
    const dailyboy::Status status = dailyboy::reformat(frame);
    if (!status.ok()) {
      state.SkipWithError(status.message().c_str());
      return;
    }
    benchmark::DoNotOptimize(frame.buf().localpixels());
    benchmark::ClobberMemory();
  }
}

}  // namespace

/*!
 * \brief Leaves hd_odd_rgb.exr (1919x1080 RGB) at display size.
 */
static void Reformat_HdOddRgb_KeepsDisplaySize(benchmark::State& state) {
  dailyboy::Frame src;
  if (!load_reformat_exr(state, "hd_odd_rgb.exr", src)) {
    return;
  }
  run_reformat_copy(state, src);
}
BENCHMARK(Reformat_HdOddRgb_KeepsDisplaySize)
    ->Unit(benchmark::kMillisecond)
    ->MinTime(0.05);

/*!
 * \brief Drops alpha on hd_rgba.exr (1920x1080 RGBA).
 */
static void Reformat_HdRgba_DropsAlpha(benchmark::State& state) {
  dailyboy::Frame src;
  if (!load_reformat_exr(state, "hd_rgba.exr", src)) {
    return;
  }
  run_reformat_copy(state, src);
}
BENCHMARK(Reformat_HdRgba_DropsAlpha)
    ->Unit(benchmark::kMillisecond)
    ->MinTime(0.05);

/*!
 * \brief Restores hd_cropped_rgb.exr (180x180 data, 1920x1080 display).
 */
static void Reformat_HdCroppedRgb_RestoresDisplay(benchmark::State& state) {
  dailyboy::Frame src;
  if (!load_reformat_exr(state, "hd_cropped_rgb.exr", src)) {
    return;
  }
  run_reformat_copy(state, src);
}
BENCHMARK(Reformat_HdCroppedRgb_RestoresDisplay)
    ->Unit(benchmark::kMillisecond)
    ->MinTime(0.05);

/*!
 * \brief Drops alpha and restores the crop on hd_cropped_odd_rgba.exr.
 */
static void Reformat_HdCroppedOddRgba_DropsAlphaAndRestores(
    benchmark::State& state) {
  dailyboy::Frame src;
  if (!load_reformat_exr(state, "hd_cropped_odd_rgba.exr", src)) {
    return;
  }
  run_reformat_copy(state, src);
}
BENCHMARK(Reformat_HdCroppedOddRgba_DropsAlphaAndRestores)
    ->Unit(benchmark::kMillisecond)
    ->MinTime(0.05);
