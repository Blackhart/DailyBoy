/*!
 * \file run_perf.cpp
 * \brief Google Benchmark timings for the full render loop (run_job).
 */

#include <benchmark/benchmark.h>

#include <cstdint>
#include <dailyboy/log.hpp>
#include <filesystem>
#include <fstream>
#include <string>

#include "job/loader.hpp"
#include "process/run.hpp"
#include "support/perf_frame.hpp"
#include "support/test_fonts.hpp"

namespace {


enum class PipelineOutput { Png, H264 };

bool write_pipeline_yaml(const std::filesystem::path& yaml_path,
                         const std::filesystem::path& plate_dir,
                         const std::filesystem::path& out_dir,
                         PipelineOutput output, int frame_count) {
  std::ofstream out(yaml_path);
  if (!out) {
    return false;
  }
  const int frame_end = dailyboy::test::kPerfPlateFrameStart + frame_count - 1;
  out << "dailyboy_version: 1\n"
      << "metadata:\n"
      << "  substitutions:\n"
      << "    project: \"DEMO\"\n"
      << "    shot: \"sh010\"\n"
      << "color:\n"
      << "  ocio_config: \"/unused/config.ocio\"\n"
      << "layout:\n"
      << "  canvas:\n"
      << "    width: " << dailyboy::test::kHdWidth << "\n"
      << "    height: " << dailyboy::test::kHdHeight << "\n"
      << "  image:\n"
      << "    fit: \"contain\"\n"
      << "    filter: \"lanczos3\"\n"
      << "    min_margin_px:\n"
      << "      top: 50\n"
      << "      bottom: 50\n"
      << "      left: 0\n"
      << "      right: 0\n"
      << "  background: {r: 0, g: 0, b: 0}\n"
      << "  burn_ins:\n"
      << "    - template: \"{project}  {shot}\"\n"
      << "      position: {mode: \"layout\", anchor: \"top_left\"}\n"
      << "      font: {path: \"" << dailyboy::test::kDejaVuSans << "\", size_px: 18}\n"
      << "      box:\n"
      << "        mode: fill\n"
      << "        color: {r: 0, g: 0, b: 0}\n"
      << "        opacity: 0.45\n"
      << "        margin: 6\n"
      << "    - template: \"{shot}  {frame}\"\n"
      << "      position: {mode: \"layout\", anchor: \"bottom_left\"}\n"
      << "      font: {path: \"" << dailyboy::test::kDejaVuSans << "\", size_px: 18}\n"
      << "      box:\n"
      << "        mode: fill\n"
      << "        color: {r: 0, g: 0, b: 0}\n"
      << "        opacity: 0.45\n"
      << "        margin: 6\n"
      << "    - template: \"{frame_start}-{frame_end}\"\n"
      << "      position: {mode: \"layout\", anchor: \"bottom_right\"}\n"
      << "      font: {path: \"" << dailyboy::test::kDejaVuSans << "\", size_px: 18}\n"
      << "      box:\n"
      << "        mode: outline\n"
      << "        color: {r: 1, g: 1, b: 1}\n"
      << "        opacity: 0.85\n"
      << "        margin: 6\n"
      << "    - template: \"DEMO\"\n"
      << "      position: {mode: \"layout\", anchor: \"top_right\"}\n"
      << "      font: {path: \"" << dailyboy::test::kDejaVuSans << "\", size_px: 18}\n"
      << "    - template: \"note\"\n"
      << "      position: {mode: \"layout\", anchor: \"top_center\"}\n"
      << "      font: {path: \"" << dailyboy::test::kDejaVuSans << "\", size_px: 16}\n"
      << "    - template: \"{plan_id}\"\n"
      << "      position: {mode: \"layout\", anchor: \"bottom_center\"}\n"
      << "      font: {path: \"" << dailyboy::test::kDejaVuSans << "\", size_px: 16}\n"
      << "  slate:\n"
      << "    duration_frames: 0\n"
      << "    lines: []\n"
      << "output:\n";
  if (output == PipelineOutput::Png) {
    out << "  image_sequences:\n"
        << "    - id: archive\n"
        << "      enabled: true\n"
        << "      display_view:\n"
           "        display: \"passthrough\"\n"
           "        view: \"passthrough\"\n"
        << "      path_pattern: \"" << (out_dir / "f.%04d.png").string()
        << "\"\n";
  } else {
    out << "  videos:\n"
        << "    - id: review_h264\n"
        << "      enabled: true\n"
        << "      display_view:\n"
           "        display: \"passthrough\"\n"
           "        view: \"passthrough\"\n"
           "      signal:\n"
           "        range: tv\n"
           "        matrix: bt709\n"
           "        primaries: bt709\n"
           "        transfer: bt709\n"
        << "      path: \"" << (out_dir / "out.mov").string() << "\"\n"
        << "      fps: 24\n"
        << "      codec: h264\n"
        << "      codec_options:\n"
        << "        preset: ultrafast\n"
        << "        faststart: false\n";
  }
  out << "plans:\n"
      << "  - id: sh010_bg\n"
      << "    input_colorspace: ACES - ACEScg\n"
      << "    sequence:\n"
      << "      path: \"" << (plate_dir / "plate.%04d.png").string() << "\"\n"
      << "      frame_start: " << dailyboy::test::kPerfPlateFrameStart << "\n"
      << "      frame_end: " << frame_end << "\n";
  return static_cast<bool>(out);
}

void run_pipeline(benchmark::State& state, PipelineOutput output,
                  const char* dir_name) {
  dailyboy::init_logging();
  dailyboy::set_log_level(dailyboy::LogLevel::Error);

  const int frame_count = static_cast<int>(state.range(0));
  const std::string dir =
      std::string(dir_name) + "_" + std::to_string(frame_count);
  const std::filesystem::path root =
      dailyboy::test::unique_perf_dir(dir.c_str());
  const std::filesystem::path plates = root / "plates";
  const std::filesystem::path out_dir = root / "out";
  if (!dailyboy::test::write_example_plate_pngs(plates, frame_count)) {
    state.SkipWithError("cannot write plate PNGs");
    return;
  }
  const std::filesystem::path yaml = root / "job.yaml";
  if (!write_pipeline_yaml(yaml, plates, out_dir, output, frame_count)) {
    state.SkipWithError("cannot write job YAML");
    return;
  }

  const dailyboy::Status schema = dailyboy::validate_job_schema(yaml);
  if (!schema.ok()) {
    state.SkipWithError(schema.message().c_str());
    return;
  }
  dailyboy::StatusOr<dailyboy::Job> job = dailyboy::load_job(yaml);
  if (!job.ok()) {
    state.SkipWithError(job.status().message().c_str());
    return;
  }

  for (auto _ : state) {
    const dailyboy::Status status = dailyboy::run_job(job.value());
    if (!status.ok()) {
      state.SkipWithError(status.message().c_str());
      return;
    }
    benchmark::ClobberMemory();
  }
  state.SetItemsProcessed(state.iterations() *
                          static_cast<int64_t>(frame_count));
  std::filesystem::remove_all(root);
}

}  // namespace

/*!
 * \brief Times run_job PNG output for 10 / 100 / 1000 HD frames.
 */
static void RunJob_PngAndBurnIns(benchmark::State& state) {
  run_pipeline(state, PipelineOutput::Png, "perf_run_png");
}
BENCHMARK(RunJob_PngAndBurnIns)
    ->Arg(10)
    ->Arg(100)
    ->Arg(1000)
    ->Unit(benchmark::kMillisecond)
    ->MinTime(0.05)
    ->UseRealTime();

/*!
 * \brief Times run_job H.264 ultrafast for 10 / 100 / 1000 HD frames.
 */
static void RunJob_H264Ultrafast(benchmark::State& state) {
  run_pipeline(state, PipelineOutput::H264, "perf_run_h264");
}
BENCHMARK(RunJob_H264Ultrafast)
    ->Arg(10)
    ->Arg(100)
    ->Arg(1000)
    ->Unit(benchmark::kMillisecond)
    ->MinTime(0.05)
    ->UseRealTime();
