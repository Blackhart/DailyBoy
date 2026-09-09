/*!
 * \file writers_perf.cpp
 * \brief Google Benchmark write timings: Fast vs HighQuality per deliverable.
 *
 * Image formats: png, jpeg, tiff, exr, heic, avif.
 * Video formats: h264, mjpeg, dnxhd, prores.
 *
 * Config matrix (job options exercised by each bench):
 *
 * | Format | Fast | HighQuality |
 * | ------ | ---- | ----------- |
 * | png | bit_depth 8, pngfast, level 1, filter 0 | bit_depth 16, default, level 6, filter 4 |
 * | jpeg | quality 60, 4:2:0, progressive false | quality 98, 4:4:4, progressive true |
 * | tiff | bit_depth 8, compression none | bit_depth h16, zip, level 9 |
 * | exr | bit_depth h16, compression rle | bit_depth f32, zip, level 9 |
 * | heic | compression heic, level 30 | compression heic, level 90 |
 * | avif | compression avif, level 30 | compression avif, level 90 |
 * | h264 | preset ultrafast, crf 28, yuv420p | preset slow, crf 18, yuv422p, high422 |
 * | mjpeg | qscale 8, yuv420p, huffman default | qscale 1, yuv444p, huffman optimal |
 * | dnxhd | profile dnxhr_lb, yuv422p | profile dnxhr_444, yuv444p10 |
 * | prores | profile proxy, yuv422p10 | profile hq, yuv422p10 |
 */

#include <OpenImageIO/imagebuf.h>
#include <benchmark/benchmark.h>

#include <filesystem>
#include <memory>
#include <string>
#include <utility>

#include "image/frame.hpp"
#include "image/sequence_writer.hpp"
#include "job/output.hpp"
#include "job/plans.hpp"
#include "status.hpp"
#include "support/perf_frame.hpp"
#include "video/video_writer.hpp"

namespace {

using H264 = dailyboy::JobOutputVideoH264;
using Mjpeg = dailyboy::JobOutputVideoMjpeg;
using Dnxhd = dailyboy::JobOutputVideoDnxhd;
using Prores = dailyboy::JobOutputVideoProres;

dailyboy::JobOutputVideoSignal tv_signal() {
  dailyboy::JobOutputVideoSignal signal;
  signal.set_range(dailyboy::JobOutputVideoSignal::RangeValue::Tv);
  return signal;
}

dailyboy::JobOutputVideo make_video(
    dailyboy::JobOutputVideo::JobOutputVideoCodecValue codec,
    dailyboy::JobOutputVideoCodecOptions options) {
  dailyboy::JobOutputVideo video;
  video.set_codec(codec);
  video.set_signal(tv_signal());
  video.set_codec_options(std::move(options));
  return video;
}

dailyboy::JobOutputVideo h264_fast() {
  H264 options;
  options.set_preset(H264::JobOutputVideoH264PresetValue::Ultrafast);
  options.set_crf(28);
  options.set_pix_fmt(H264::JobOutputVideoH264PixFmtValue::Yuv420p);
  options.set_faststart(false);
  return make_video(dailyboy::JobOutputVideo::JobOutputVideoCodecValue::H264,
                    std::move(options));
}

dailyboy::JobOutputVideo h264_high_quality() {
  H264 options;
  options.set_preset(H264::JobOutputVideoH264PresetValue::Slow);
  options.set_crf(18);
  options.set_pix_fmt(H264::JobOutputVideoH264PixFmtValue::Yuv422p);
  options.set_profile(H264::JobOutputVideoH264ProfileValue::High422);
  options.set_faststart(false);
  return make_video(dailyboy::JobOutputVideo::JobOutputVideoCodecValue::H264,
                    std::move(options));
}

dailyboy::JobOutputVideo mjpeg_fast() {
  Mjpeg options;
  options.set_qscale(8);
  options.set_pix_fmt(Mjpeg::JobOutputVideoMjpegPixFmtValue::Yuv420p);
  options.set_huffman(Mjpeg::JobOutputVideoMjpegHuffmanValue::Default);
  options.set_faststart(false);
  return make_video(dailyboy::JobOutputVideo::JobOutputVideoCodecValue::Mjpeg,
                    std::move(options));
}

dailyboy::JobOutputVideo mjpeg_high_quality() {
  Mjpeg options;
  options.set_qscale(1);
  options.set_pix_fmt(Mjpeg::JobOutputVideoMjpegPixFmtValue::Yuv444p);
  options.set_huffman(Mjpeg::JobOutputVideoMjpegHuffmanValue::Optimal);
  options.set_faststart(false);
  return make_video(dailyboy::JobOutputVideo::JobOutputVideoCodecValue::Mjpeg,
                    std::move(options));
}

dailyboy::JobOutputVideo dnxhd_fast() {
  Dnxhd options;
  options.set_profile(Dnxhd::JobOutputVideoDnxhdProfileValue::DnxhrLb);
  options.set_pix_fmt(Dnxhd::JobOutputVideoDnxhdPixFmtValue::Yuv422p);
  options.set_faststart(false);
  return make_video(dailyboy::JobOutputVideo::JobOutputVideoCodecValue::Dnxhd,
                    std::move(options));
}

dailyboy::JobOutputVideo dnxhd_high_quality() {
  Dnxhd options;
  options.set_profile(Dnxhd::JobOutputVideoDnxhdProfileValue::Dnxhr444);
  options.set_pix_fmt(Dnxhd::JobOutputVideoDnxhdPixFmtValue::Yuv444p10);
  options.set_faststart(false);
  return make_video(dailyboy::JobOutputVideo::JobOutputVideoCodecValue::Dnxhd,
                    std::move(options));
}

dailyboy::JobOutputVideo prores_fast() {
  Prores options;
  options.set_profile(Prores::JobOutputVideoProresProfileValue::Proxy);
  options.set_pix_fmt(Prores::JobOutputVideoProresPixFmtValue::Yuv422p10);
  options.set_faststart(false);
  return make_video(dailyboy::JobOutputVideo::JobOutputVideoCodecValue::Prores,
                    std::move(options));
}

dailyboy::JobOutputVideo prores_high_quality() {
  Prores options;
  options.set_profile(Prores::JobOutputVideoProresProfileValue::Hq);
  options.set_pix_fmt(Prores::JobOutputVideoProresPixFmtValue::Yuv422p10);
  options.set_faststart(false);
  return make_video(dailyboy::JobOutputVideo::JobOutputVideoCodecValue::Prores,
                    std::move(options));
}

dailyboy::JobOutputImageSequencePng png_fast() {
  dailyboy::JobOutputImageSequencePng options;
  options.set_bit_depth(8);
  options.set_compression("pngfast");
  options.set_compression_level(1);
  options.set_filter(0);
  return options;
}

dailyboy::JobOutputImageSequencePng png_high_quality() {
  dailyboy::JobOutputImageSequencePng options;
  options.set_bit_depth(16);
  options.set_compression("default");
  options.set_compression_level(6);
  options.set_filter(4);
  return options;
}

dailyboy::JobOutputImageSequenceJpeg jpeg_fast() {
  dailyboy::JobOutputImageSequenceJpeg options;
  options.set_bit_depth(8);
  options.set_compression_level(60);
  options.set_subsampling("4:2:0");
  options.set_progressive(false);
  return options;
}

dailyboy::JobOutputImageSequenceJpeg jpeg_high_quality() {
  dailyboy::JobOutputImageSequenceJpeg options;
  options.set_bit_depth(8);
  options.set_compression_level(98);
  options.set_subsampling("4:4:4");
  options.set_progressive(true);
  return options;
}

dailyboy::JobOutputImageSequenceTiff tiff_fast() {
  dailyboy::JobOutputImageSequenceTiff options;
  options.set_bit_depth("8");
  options.set_compression("none");
  options.set_compression_level(1);
  return options;
}

dailyboy::JobOutputImageSequenceTiff tiff_high_quality() {
  dailyboy::JobOutputImageSequenceTiff options;
  options.set_bit_depth("h16");
  options.set_compression("zip");
  options.set_compression_level(9);
  return options;
}

dailyboy::JobOutputImageSequenceExr exr_fast() {
  dailyboy::JobOutputImageSequenceExr options;
  options.set_bit_depth("h16");
  options.set_compression("rle");
  return options;
}

dailyboy::JobOutputImageSequenceExr exr_high_quality() {
  dailyboy::JobOutputImageSequenceExr options;
  options.set_bit_depth("f32");
  options.set_compression("zip");
  options.set_compression_level(9);
  return options;
}

dailyboy::JobOutputImageSequenceHeif heic_fast() {
  dailyboy::JobOutputImageSequenceHeif options;
  options.set_bit_depth(8);
  options.set_compression("heic");
  options.set_compression_level(30);
  return options;
}

dailyboy::JobOutputImageSequenceHeif heic_high_quality() {
  dailyboy::JobOutputImageSequenceHeif options;
  options.set_bit_depth(8);
  options.set_compression("heic");
  options.set_compression_level(90);
  return options;
}

dailyboy::JobOutputImageSequenceHeif avif_fast() {
  dailyboy::JobOutputImageSequenceHeif options;
  options.set_bit_depth(8);
  options.set_compression("avif");
  options.set_compression_level(30);
  return options;
}

dailyboy::JobOutputImageSequenceHeif avif_high_quality() {
  dailyboy::JobOutputImageSequenceHeif options;
  options.set_bit_depth(8);
  options.set_compression("avif");
  options.set_compression_level(90);
  return options;
}

void run_video_write(benchmark::State& state, const char* name,
                     const dailyboy::JobOutputVideo& video) {
  const std::filesystem::path dir = dailyboy::test::unique_perf_dir(name);
  const std::filesystem::path mov = dir / "out.mov";
  std::unique_ptr<dailyboy::VideoWriter> writer =
      dailyboy::make_video_writer(video.codec());
  const dailyboy::Frame frame = dailyboy::test::hd_rgb_uint8();
  const dailyboy::Status open =
      writer->open(mov, dailyboy::test::kHdWidth, dailyboy::test::kHdHeight,
                   dailyboy::VideoWriter::kDefaultFps, video);
  if (!open.ok()) {
    state.SkipWithError(open.message().c_str());
    return;
  }
  for (auto _ : state) {
    const dailyboy::Status status = writer->write(frame);
    if (!status.ok()) {
      state.SkipWithError(status.message().c_str());
      break;
    }
    benchmark::DoNotOptimize(frame.buf().localpixels());
    benchmark::ClobberMemory();
  }
  writer->close();
  std::filesystem::remove_all(dir);
}

void run_sequence_write(benchmark::State& state, const char* name,
                        const std::string& pattern_leaf,
                        dailyboy::JobOutputImageSequenceFormatOptions options) {
  const std::filesystem::path dir = dailyboy::test::unique_perf_dir(name);
  dailyboy::JobSequence sequence;
  sequence.set_path((dir / pattern_leaf).string());
  dailyboy::StatusOr<dailyboy::SequenceWriter> opened =
      dailyboy::SequenceWriter::open(sequence, {}, std::move(options));
  if (!opened.ok()) {
    state.SkipWithError(opened.status().message().c_str());
    return;
  }
  dailyboy::SequenceWriter writer = std::move(opened).value();
  const dailyboy::Frame frame = dailyboy::test::hd_rgb_float();
  int frame_number = 1001;
  for (auto _ : state) {
    const dailyboy::Status status = writer.write(frame, frame_number);
    ++frame_number;
    if (!status.ok()) {
      state.SkipWithError(status.message().c_str());
      break;
    }
    benchmark::DoNotOptimize(frame.buf().localpixels());
    benchmark::ClobberMemory();
  }
  std::filesystem::remove_all(dir);
}

}  // namespace

static void Write_H264_Fast(benchmark::State& state) {
  run_video_write(state, "perf_h264_fast", h264_fast());
}
BENCHMARK(Write_H264_Fast)->Unit(benchmark::kMillisecond)->MinTime(0.05);

static void Write_H264_HighQuality(benchmark::State& state) {
  run_video_write(state, "perf_h264_hq", h264_high_quality());
}
BENCHMARK(Write_H264_HighQuality)->Unit(benchmark::kMillisecond)->MinTime(0.05);

static void Write_Mjpeg_Fast(benchmark::State& state) {
  run_video_write(state, "perf_mjpeg_fast", mjpeg_fast());
}
BENCHMARK(Write_Mjpeg_Fast)->Unit(benchmark::kMillisecond)->MinTime(0.05);

static void Write_Mjpeg_HighQuality(benchmark::State& state) {
  run_video_write(state, "perf_mjpeg_hq", mjpeg_high_quality());
}
BENCHMARK(Write_Mjpeg_HighQuality)
    ->Unit(benchmark::kMillisecond)
    ->MinTime(0.05);

static void Write_Dnxhd_Fast(benchmark::State& state) {
  run_video_write(state, "perf_dnxhd_fast", dnxhd_fast());
}
BENCHMARK(Write_Dnxhd_Fast)->Unit(benchmark::kMillisecond)->MinTime(0.05);

static void Write_Dnxhd_HighQuality(benchmark::State& state) {
  run_video_write(state, "perf_dnxhd_hq", dnxhd_high_quality());
}
BENCHMARK(Write_Dnxhd_HighQuality)
    ->Unit(benchmark::kMillisecond)
    ->MinTime(0.05);

static void Write_Prores_Fast(benchmark::State& state) {
  run_video_write(state, "perf_prores_fast", prores_fast());
}
BENCHMARK(Write_Prores_Fast)->Unit(benchmark::kMillisecond)->MinTime(0.05);

static void Write_Prores_HighQuality(benchmark::State& state) {
  run_video_write(state, "perf_prores_hq", prores_high_quality());
}
BENCHMARK(Write_Prores_HighQuality)
    ->Unit(benchmark::kMillisecond)
    ->MinTime(0.05);

static void Write_Png_Fast(benchmark::State& state) {
  run_sequence_write(state, "perf_png_fast", "f.%04d.png", png_fast());
}
BENCHMARK(Write_Png_Fast)->Unit(benchmark::kMillisecond)->MinTime(0.05);

static void Write_Png_HighQuality(benchmark::State& state) {
  run_sequence_write(state, "perf_png_hq", "f.%04d.png", png_high_quality());
}
BENCHMARK(Write_Png_HighQuality)->Unit(benchmark::kMillisecond)->MinTime(0.05);

static void Write_Jpeg_Fast(benchmark::State& state) {
  run_sequence_write(state, "perf_jpeg_fast", "f.%04d.jpg", jpeg_fast());
}
BENCHMARK(Write_Jpeg_Fast)->Unit(benchmark::kMillisecond)->MinTime(0.05);

static void Write_Jpeg_HighQuality(benchmark::State& state) {
  run_sequence_write(state, "perf_jpeg_hq", "f.%04d.jpg", jpeg_high_quality());
}
BENCHMARK(Write_Jpeg_HighQuality)->Unit(benchmark::kMillisecond)->MinTime(0.05);

static void Write_Tiff_Fast(benchmark::State& state) {
  run_sequence_write(state, "perf_tiff_fast", "f.%04d.tiff", tiff_fast());
}
BENCHMARK(Write_Tiff_Fast)->Unit(benchmark::kMillisecond)->MinTime(0.05);

static void Write_Tiff_HighQuality(benchmark::State& state) {
  run_sequence_write(state, "perf_tiff_hq", "f.%04d.tiff", tiff_high_quality());
}
BENCHMARK(Write_Tiff_HighQuality)->Unit(benchmark::kMillisecond)->MinTime(0.05);

static void Write_Exr_Fast(benchmark::State& state) {
  run_sequence_write(state, "perf_exr_fast", "f.%04d.exr", exr_fast());
}
BENCHMARK(Write_Exr_Fast)->Unit(benchmark::kMillisecond)->MinTime(0.05);

static void Write_Exr_HighQuality(benchmark::State& state) {
  run_sequence_write(state, "perf_exr_hq", "f.%04d.exr", exr_high_quality());
}
BENCHMARK(Write_Exr_HighQuality)->Unit(benchmark::kMillisecond)->MinTime(0.05);

static void Write_Heic_Fast(benchmark::State& state) {
  run_sequence_write(state, "perf_heic_fast", "f.%04d.heic", heic_fast());
}
BENCHMARK(Write_Heic_Fast)->Unit(benchmark::kMillisecond)->MinTime(0.05);

static void Write_Heic_HighQuality(benchmark::State& state) {
  run_sequence_write(state, "perf_heic_hq", "f.%04d.heic", heic_high_quality());
}
BENCHMARK(Write_Heic_HighQuality)->Unit(benchmark::kMillisecond)->MinTime(0.05);

static void Write_Avif_Fast(benchmark::State& state) {
  run_sequence_write(state, "perf_avif_fast", "f.%04d.avif", avif_fast());
}
BENCHMARK(Write_Avif_Fast)->Unit(benchmark::kMillisecond)->MinTime(0.05);

static void Write_Avif_HighQuality(benchmark::State& state) {
  run_sequence_write(state, "perf_avif_hq", "f.%04d.avif", avif_high_quality());
}
BENCHMARK(Write_Avif_HighQuality)->Unit(benchmark::kMillisecond)->MinTime(0.05);
