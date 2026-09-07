/*!
 * \file sequence_writer.cpp
 * \brief Writes Frame pixels to a fileseq output path via OpenImageIO.
 */

#include "image/sequence_writer.hpp"

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>
#include <OpenImageIO/imageio.h>
#include <fileseq/error.h>
#include <fileseq/sequence.h>

#include <dailyboy/log.hpp>
#include <exception>
#include <filesystem>
#include <string>
#include <system_error>
#include <type_traits>
#include <utility>
#include <variant>

#include "error/image.hpp"
#include "process/path_tokens.hpp"
#include "status.hpp"

namespace dailyboy {

namespace {

Status ensure_parent_directory(const std::filesystem::path& path) {
  const std::filesystem::path parent = path.parent_path();
  if (parent.empty()) {
    return Status::Ok();
  }
  std::error_code ec;
  std::filesystem::create_directories(parent, ec);
  if (ec) {
    return Status::User(std::string(USER_ERROR_IO_8) + " " + parent.string() +
                        ": " + ec.message());
  }
  return Status::Ok();
}

StatusOr<std::string> validated_pattern(const std::string& pattern) {
  if (pattern.empty()) {
    return Status::User(std::string(USER_ERROR_IO_2));
  }
  fileseq::Status ok;
  const fileseq::FileSequence seq(pattern, &ok);
  if (!ok || !seq.isValid()) {
    return Status::User(std::string(USER_ERROR_IO_3) + " " + pattern);
  }
  return pattern;
}

std::filesystem::path path_for_frame(const std::string& pattern, int frame) {
  fileseq::Status ok;
  const fileseq::FileSequence seq(pattern, &ok);
  return std::filesystem::path(seq.frame(static_cast<fileseq::Frame>(frame)));
}

OIIO::TypeDesc type_for_int_depth(int bit_depth) {
  if (bit_depth <= 8) {
    return OIIO::TypeDesc::UINT8;
  }
  if (bit_depth <= 16) {
    return OIIO::TypeDesc::UINT16;
  }
  if (bit_depth <= 32) {
    return OIIO::TypeDesc::UINT32;
  }
  return OIIO::TypeDesc::DOUBLE;
}

OIIO::TypeDesc type_for_tiff_depth(const std::string& bit_depth) {
  if (bit_depth == "h16") {
    return OIIO::TypeDesc::HALF;
  }
  if (bit_depth == "f32") {
    return OIIO::TypeDesc::FLOAT;
  }
  return type_for_int_depth(std::stoi(bit_depth));
}

StatusOr<OIIO::ImageBuf> convert_buffer(const OIIO::ImageBuf& src,
                                        OIIO::TypeDesc type) {
  if (src.spec().format == type) {
    return src.copy(OIIO::TypeDesc::UNKNOWN);
  }
  OIIO::ImageBuf dst;
  if (!OIIO::ImageBufAlgo::copy(dst, src, type)) {
    return Status::User(std::string(USER_ERROR_IO_8) + " " + dst.geterror());
  }
  return dst;
}

void apply_png_attrs(OIIO::ImageSpec& spec,
                     const JobOutputImageSequencePng& options) {
  spec.attribute("compression", options.compression());
  spec.attribute("png:compressionLevel", options.compression_level());
  spec.attribute("png:filter", options.filter());
}

void apply_jpeg_attrs(OIIO::ImageSpec& spec,
                      const JobOutputImageSequenceJpeg& options) {
  spec.attribute("Compression", "jpeg");
  spec.attribute("CompressionQuality", options.compression_level());
  if (!options.subsampling().empty()) {
    spec.attribute("jpeg:subsampling", options.subsampling());
  }
  if (options.progressive()) {
    spec.attribute("jpeg:progressive", 1);
  }
}

void apply_tiff_attrs(OIIO::ImageSpec& spec,
                      const JobOutputImageSequenceTiff& options) {
  spec.attribute("Compression", options.compression());
  spec.attribute("CompressionQuality", options.compression_level());
  spec.attribute("tiff:zipquality", options.compression_level());
  if (options.bigtiff()) {
    spec.attribute("tiff:bigtiff", 1);
  }
  const std::string& depth = options.bit_depth();
  if (depth == "h16") {
    spec.attribute("tiff:half", 1);
  } else if (depth != "f32" && depth != "8" && depth != "16" && depth != "32" &&
             depth != "64") {
    spec.attribute("oiio:BitsPerSample", std::stoi(depth));
  }
}

void apply_exr_attrs(OIIO::ImageSpec& spec,
                     const JobOutputImageSequenceExr& options) {
  spec.attribute("Compression", options.compression());
  if (options.has_compression_level()) {
    spec.attribute("CompressionQuality", options.compression_level());
  } else if (options.compression() == "zip" ||
             options.compression() == "zips") {
    spec.attribute("CompressionQuality",
                   JobOutputImageSequenceExr::kDefaultZipLevel);
  }
}

void apply_heif_attrs(OIIO::ImageSpec& spec,
                      const JobOutputImageSequenceHeif& options) {
  spec.attribute("Compression", options.compression());
  spec.attribute("CompressionQuality", options.compression_level());
}

StatusOr<OIIO::ImageBuf> prepare_png(const Frame& frame,
                                     const JobOutputImageSequencePng& options) {
  DAILYBOY_ASSIGN_OR_RETURN(
      OIIO::ImageBuf buf,
      convert_buffer(frame.buf(), type_for_int_depth(options.bit_depth())));
  apply_png_attrs(buf.specmod(), options);
  return buf;
}

StatusOr<OIIO::ImageBuf> prepare_jpeg(
    const Frame& frame, const JobOutputImageSequenceJpeg& options) {
  DAILYBOY_ASSIGN_OR_RETURN(OIIO::ImageBuf buf,
                            convert_buffer(frame.buf(), OIIO::TypeDesc::UINT8));
  apply_jpeg_attrs(buf.specmod(), options);
  return buf;
}

StatusOr<OIIO::ImageBuf> prepare_tiff(
    const Frame& frame, const JobOutputImageSequenceTiff& options) {
  DAILYBOY_ASSIGN_OR_RETURN(
      OIIO::ImageBuf buf,
      convert_buffer(frame.buf(), type_for_tiff_depth(options.bit_depth())));
  apply_tiff_attrs(buf.specmod(), options);
  return buf;
}

StatusOr<OIIO::ImageBuf> prepare_exr(const Frame& frame,
                                     const JobOutputImageSequenceExr& options) {
  const OIIO::TypeDesc type = options.bit_depth() == "f32"
                                  ? OIIO::TypeDesc::FLOAT
                                  : OIIO::TypeDesc::HALF;
  DAILYBOY_ASSIGN_OR_RETURN(OIIO::ImageBuf buf,
                            convert_buffer(frame.buf(), type));
  apply_exr_attrs(buf.specmod(), options);
  return buf;
}

StatusOr<OIIO::ImageBuf> prepare_heif(
    const Frame& frame, const JobOutputImageSequenceHeif& options) {
  DAILYBOY_ASSIGN_OR_RETURN(OIIO::ImageBuf buf,
                            convert_buffer(frame.buf(), OIIO::TypeDesc::UINT8));
  apply_heif_attrs(buf.specmod(), options);
  return buf;
}

StatusOr<OIIO::ImageBuf> prepare_write_buffer(
    const Frame& frame,
    const std::optional<JobOutputImageSequenceFormatOptions>& format_options) {
  if (!format_options) {
    return frame.buf().copy(OIIO::TypeDesc::UNKNOWN);
  }
  return std::visit(
      [&](const auto& options) -> StatusOr<OIIO::ImageBuf> {
        using T = std::decay_t<decltype(options)>;
        if constexpr (std::is_same_v<T, JobOutputImageSequencePng>) {
          return prepare_png(frame, options);
        } else if constexpr (std::is_same_v<T, JobOutputImageSequenceJpeg>) {
          return prepare_jpeg(frame, options);
        } else if constexpr (std::is_same_v<T, JobOutputImageSequenceTiff>) {
          return prepare_tiff(frame, options);
        } else if constexpr (std::is_same_v<T, JobOutputImageSequenceExr>) {
          return prepare_exr(frame, options);
        } else {
          return prepare_heif(frame, options);
        }
      },
      *format_options);
}

Status write_buffer(const OIIO::ImageBuf& buf,
                    const std::filesystem::path& path) {
  if (!buf.write(path.string())) {
    return Status::User(std::string(USER_ERROR_IO_8) + " " + path.string() +
                        ": " + buf.geterror());
  }
  return Status::Ok();
}

void append_spec_attr(std::string& text, const OIIO::ParamValue& attr) {
  text += " ";
  text += attr.name().string();
  text += "=";
  text += attr.get_string();
}

void log_write_spec(const OIIO::ImageSpec& spec,
                    const std::filesystem::path& path) {
  std::string text = "render: write " + path.string() +
                     " TypeDesc=" + std::string(spec.format.c_str());
  for (const OIIO::ParamValue& attr : spec.extra_attribs) {
    append_spec_attr(text, attr);
  }
  log_debug(text);
}

}  // namespace

StatusOr<SequenceWriter> SequenceWriter::open(
    const JobSequence& sequence,
    const std::map<std::string, JobMetadataSubstitutionValue>& substitutions,
    std::optional<JobOutputImageSequenceFormatOptions> format_options) {
  DAILYBOY_ASSIGN_OR_RETURN(
      std::string expanded,
      expand_path_tokens(sequence.path(), substitutions, USER_ERROR_IO_6));
  SequenceWriter writer;
  DAILYBOY_ASSIGN_OR_RETURN(writer.pattern_, validated_pattern(expanded));
  writer.format_options_ = std::move(format_options);
  return writer;
}

Status SequenceWriter::write(const Frame& frame, int frame_number) {
  const std::filesystem::path path = path_for_frame(pattern_, frame_number);
  DAILYBOY_RETURN_IF_ERROR(ensure_parent_directory(path));
  try {
    DAILYBOY_ASSIGN_OR_RETURN(OIIO::ImageBuf buf,
                              prepare_write_buffer(frame, format_options_));
    log_write_spec(buf.spec(), path);
    return write_buffer(buf, path);
  } catch (const std::exception& ex) {
    return Status::User(std::string(USER_ERROR_IO_8) + " " + path.string() +
                        ": " + ex.what());
  }
}

}  // namespace dailyboy
