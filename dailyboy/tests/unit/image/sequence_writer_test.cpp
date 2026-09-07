#include "image/sequence_writer.hpp"

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>
#include <OpenImageIO/imageio.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <optional>
#include <string>
#include <variant>

#include "image/frame.hpp"
#include "job/output.hpp"
#include "job/plans.hpp"
#include "status.hpp"
#include "support/test_fixtures.hpp"

namespace {

dailyboy::Frame solid_frame() {
  OIIO::ImageSpec spec(dailyboy::test::kPlateWidth,
                       dailyboy::test::kPlateHeight, 3, OIIO::TypeDesc::FLOAT);
  OIIO::ImageBuf buf(spec);
  const float rgb[3] = {0.2f, 0.4f, 0.6f};
  EXPECT_TRUE(OIIO::ImageBufAlgo::fill(buf, OIIO::cspan<float>(rgb, 3)));
  return dailyboy::Frame(std::move(buf));
}

std::filesystem::path writer_dir(const std::string& name) {
  const std::filesystem::path dir =
      std::filesystem::path(DAILYBOY_TEST_BINARY_DIR) / name;
  std::error_code ec;
  std::filesystem::create_directories(dir, ec);
  return dir;
}

void expect_written_file(const std::filesystem::path& path,
                         OIIO::TypeDesc expected_format) {
  ASSERT_TRUE(std::filesystem::is_regular_file(path)) << path;
  auto in = OIIO::ImageInput::open(path.string());
  ASSERT_NE(in, nullptr) << path << ": " << OIIO::geterror();
  EXPECT_EQ(in->spec().format, expected_format) << path;
  EXPECT_EQ(in->spec().nchannels, 3);
  EXPECT_GT(in->spec().width, 0);
  EXPECT_GT(in->spec().height, 0);
  in->close();
}

dailyboy::Status write_one(
    const std::filesystem::path& pattern,
    dailyboy::JobOutputImageSequenceFormatOptions options) {
  dailyboy::JobSequence sequence;
  sequence.set_path(pattern.string());
  dailyboy::StatusOr<dailyboy::SequenceWriter> writer =
      dailyboy::SequenceWriter::open(sequence, {}, std::move(options));
  if (!writer.ok()) {
    return writer.status();
  }
  return writer.value().write(solid_frame(), 1001);
}

}  // namespace

/*!
 * \brief Writes one PNG at the requested frame number.
 */
TEST(SequenceWriter, Write_OneFrame_CreatesFile) {
  // Prepare
  const std::filesystem::path dir = writer_dir("sequence_writer");
  dailyboy::JobSequence sequence;
  sequence.set_path((dir / "out.%04d.png").string());
  sequence.set_frame_start(1001);
  sequence.set_frame_end(1001);
  dailyboy::Frame frame = solid_frame();

  // Test
  dailyboy::StatusOr<dailyboy::SequenceWriter> writer =
      dailyboy::SequenceWriter::open(sequence);
  ASSERT_TRUE(writer.ok()) << writer.status().message();
  const dailyboy::Status status = writer.value().write(frame, 1001);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  EXPECT_TRUE(std::filesystem::is_regular_file(dir / "out.1001.png"));
}

/*!
 * \brief Writes UINT8 PNG when format_options set bit_depth 8.
 */
TEST(SequenceWriter, Write_PngFormatOptions_CreatesUint8File) {
  // Prepare
  const std::filesystem::path dir = writer_dir("sequence_writer_png");
  dailyboy::JobOutputImageSequencePng options;
  options.set_bit_depth(8);
  options.set_compression("default");
  options.set_compression_level(6);
  options.set_filter(0);

  // Test
  const dailyboy::Status status =
      write_one(dir / "out.%04d.png", std::move(options));

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  expect_written_file(dir / "out.1001.png", OIIO::TypeDesc::UINT8);
}

/*!
 * \brief Writes JPEG with quality and reopens the file.
 */
TEST(SequenceWriter, Write_JpegFormatOptions_CreatesUint8File) {
  // Prepare
  const std::filesystem::path dir = writer_dir("sequence_writer_jpeg");
  dailyboy::JobOutputImageSequenceJpeg options;
  options.set_bit_depth(8);
  options.set_compression_level(90);
  options.set_subsampling("4:2:0");
  options.set_progressive(false);

  // Test
  const dailyboy::Status status =
      write_one(dir / "out.%04d.jpg", std::move(options));

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  expect_written_file(dir / "out.1001.jpg", OIIO::TypeDesc::UINT8);
}

/*!
 * \brief Writes 8-bit ZIP TIFF and reopens the file.
 */
TEST(SequenceWriter, Write_TiffUint8Zip_CreatesReadableFile) {
  // Prepare
  const std::filesystem::path dir = writer_dir("sequence_writer_tiff8");
  dailyboy::JobOutputImageSequenceTiff options;
  options.set_bit_depth("8");
  options.set_compression("zip");
  options.set_compression_level(6);
  options.set_bigtiff(false);

  // Test
  const dailyboy::Status status =
      write_one(dir / "out.%04d.tiff", std::move(options));

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  expect_written_file(dir / "out.1001.tiff", OIIO::TypeDesc::UINT8);
}

/*!
 * \brief Writes half ZIP TIFF (example share options) and reopens the file.
 */
TEST(SequenceWriter, Write_TiffHalfZip_CreatesReadableFile) {
  // Prepare
  const std::filesystem::path dir = writer_dir("sequence_writer_tiff_h16");
  dailyboy::JobOutputImageSequenceTiff options;
  options.set_bit_depth("h16");
  options.set_compression("zip");
  options.set_compression_level(6);
  options.set_bigtiff(false);

  // Test
  const dailyboy::Status status =
      write_one(dir / "out.%04d.tiff", std::move(options));

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  expect_written_file(dir / "out.1001.tiff", OIIO::TypeDesc::HALF);
}

/*!
 * \brief Writes float TIFF and reopens the file.
 */
TEST(SequenceWriter, Write_TiffFloat_CreatesReadableFile) {
  // Prepare
  const std::filesystem::path dir = writer_dir("sequence_writer_tiff_f32");
  dailyboy::JobOutputImageSequenceTiff options;
  options.set_bit_depth("f32");
  options.set_compression("lzw");
  options.set_compression_level(6);

  // Test
  const dailyboy::Status status =
      write_one(dir / "out.%04d.tif", std::move(options));

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  expect_written_file(dir / "out.1001.tif", OIIO::TypeDesc::FLOAT);
}

/*!
 * \brief Writes half OpenEXR and reopens the file.
 */
TEST(SequenceWriter, Write_ExrHalfZip_CreatesReadableFile) {
  // Prepare
  const std::filesystem::path dir = writer_dir("sequence_writer_exr");
  dailyboy::JobOutputImageSequenceExr options;
  options.set_bit_depth("h16");
  options.set_compression("zip");
  options.set_compression_level(4);

  // Test
  const dailyboy::Status status =
      write_one(dir / "out.%04d.exr", std::move(options));

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  expect_written_file(dir / "out.1001.exr", OIIO::TypeDesc::HALF);
}

/*!
 * \brief Writes HEIC and reopens the file.
 */
TEST(SequenceWriter, Write_HeifHeic_CreatesReadableFile) {
  // Prepare
  const std::filesystem::path dir = writer_dir("sequence_writer_heic");
  dailyboy::JobOutputImageSequenceHeif options;
  options.set_bit_depth(8);
  options.set_compression("heic");
  options.set_compression_level(75);

  // Test
  const dailyboy::Status status =
      write_one(dir / "out.%04d.heic", std::move(options));

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  expect_written_file(dir / "out.1001.heic", OIIO::TypeDesc::UINT8);
}

/*!
 * \brief Writes AVIF and reopens the file.
 */
TEST(SequenceWriter, Write_HeifAvif_CreatesReadableFile) {
  // Prepare
  const std::filesystem::path dir = writer_dir("sequence_writer_avif");
  dailyboy::JobOutputImageSequenceHeif options;
  options.set_bit_depth(8);
  options.set_compression("avif");
  options.set_compression_level(75);

  // Test
  const dailyboy::Status status =
      write_one(dir / "out.%04d.avif", std::move(options));

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  expect_written_file(dir / "out.1001.avif", OIIO::TypeDesc::UINT8);
}

/*!
 * \brief Writes a frame number that is not a source-plan range.
 */
TEST(SequenceWriter, Write_FrameOutsidePlanRange_CreatesFile) {
  // Prepare
  const std::filesystem::path dir = writer_dir("sequence_writer_slate");
  dailyboy::JobSequence sequence;
  sequence.set_path((dir / "out.%04d.png").string());
  dailyboy::Frame frame = solid_frame();

  // Test
  dailyboy::StatusOr<dailyboy::SequenceWriter> writer =
      dailyboy::SequenceWriter::open(sequence);
  ASSERT_TRUE(writer.ok()) << writer.status().message();
  const dailyboy::Status status = writer.value().write(frame, 999);

  // Assert
  ASSERT_TRUE(status.ok()) << status.message();
  EXPECT_TRUE(std::filesystem::is_regular_file(dir / "out.0999.png"));
}
