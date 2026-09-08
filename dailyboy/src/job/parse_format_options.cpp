/*!
 * \file parse_format_options.cpp
 * \brief Parse image sequence format_options from the job YAML.
 */

#include "job/parse_format_options.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>

#include "error/job.hpp"
#include "job/yaml_read.hpp"

namespace dailyboy {

namespace {

std::string lower_ext(const std::string& path_pattern) {
  const std::filesystem::path path(path_pattern);
  std::string ext = path.extension().string();
  std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return ext;
}

bool is_png_compression(const std::string& value) {
  return value == "default" || value == "filtered" || value == "huffman" ||
         value == "rle" || value == "fixed" || value == "pngfast" ||
         value == "none";
}

bool is_tiff_compression(const std::string& value) {
  return value == "none" || value == "lzw" || value == "zip" ||
         value == "ccittrle" || value == "packbits";
}

bool is_exr_compression(const std::string& value) {
  return value == "none" || value == "rle" || value == "zips" ||
         value == "zip" || value == "piz" || value == "pxr24" ||
         value == "b44" || value == "b44a" || value == "dwaa" ||
         value == "dwab";
}

bool is_jpeg_subsampling(const std::string& value) {
  return value == "4:4:4" || value == "4:2:2" || value == "4:2:0" ||
         value == "4:1:1";
}

bool is_tiff_bit_depth(const std::string& value) {
  static const char* kOk[] = {"1",  "2",  "4",  "6",  "8",  "10",  "12",
                              "14", "16", "24", "32", "64", "h16", "f32"};
  for (const char* ok : kOk) {
    if (value == ok) {
      return true;
    }
  }
  return false;
}

std::string tiff_bit_depth_string(const YAML::Node& node) {
  if (node.IsScalar() && node.Tag() != "!" && !node.IsNull()) {
    int as_int = 0;
    if (try_yaml_as(node, as_int)) {
      return std::to_string(as_int);
    }
  }
  return node.as<std::string>();
}

Status reject_unknown_keys(const YAML::Node& map, const std::string& field,
                           const char* const* keys, std::size_t count) {
  for (const auto& it : map) {
    const std::string key = it.first.as<std::string>();
    bool known = false;
    for (std::size_t i = 0; i < count; ++i) {
      if (key == keys[i]) {
        known = true;
        break;
      }
    }
    if (!known) {
      std::string detail = field;
      detail += ".";
      detail += key;
      return Status::User(with_job_error(USER_ERROR_JOB_40, detail));
    }
  }
  return Status::Ok();
}

StatusOr<JobOutputImageSequencePng> parse_png_options(
    const YAML::Node& map, const std::string& field) {
  static const char* kKeys[] = {"bit_depth", "compression", "compression_level",
                                "filter"};
  DAILYBOY_RETURN_IF_ERROR(
      reject_unknown_keys(map, field, kKeys, std::size(kKeys)));
  JobOutputImageSequencePng options;
  DAILYBOY_ASSIGN_OR_RETURN(
      int bit_depth,
      as_optional<int>(map, "bit_depth",
                       JobOutputImageSequencePng::kDefaultBitDepth, field));
  if (bit_depth != 8 && bit_depth != 16) {
    return Status::User(with_job_error(
        USER_ERROR_JOB_25,
        field + ".bit_depth '" + std::to_string(bit_depth) + "'."));
  }
  options.set_bit_depth(bit_depth);
  DAILYBOY_ASSIGN_OR_RETURN(
      const std::string compression,
      as_optional<std::string>(map, "compression", "default", field));
  if (!is_png_compression(compression)) {
    return Status::User(with_job_error(
        USER_ERROR_JOB_25, field + ".compression '" + compression + "'."));
  }
  options.set_compression(compression);
  DAILYBOY_ASSIGN_OR_RETURN(
      int level,
      as_optional<int>(map, "compression_level",
                       JobOutputImageSequencePng::kDefaultCompressionLevel,
                       field));
  if (level < 0 || level > 6) {
    return Status::User(
        with_job_error(USER_ERROR_JOB_25, field + ".compression_level."));
  }
  options.set_compression_level(level);
  DAILYBOY_ASSIGN_OR_RETURN(
      int filter,
      as_optional<int>(map, "filter", JobOutputImageSequencePng::kDefaultFilter,
                       field));
  if (filter < 0 || filter > 4) {
    return Status::User(with_job_error(USER_ERROR_JOB_25, field + ".filter."));
  }
  options.set_filter(filter);
  return options;
}

StatusOr<JobOutputImageSequenceJpeg> parse_jpeg_options(
    const YAML::Node& map, const std::string& field) {
  static const char* kKeys[] = {"bit_depth", "compression_level", "subsampling",
                                "progressive"};
  DAILYBOY_RETURN_IF_ERROR(
      reject_unknown_keys(map, field, kKeys, std::size(kKeys)));
  JobOutputImageSequenceJpeg options;
  DAILYBOY_ASSIGN_OR_RETURN(
      int bit_depth,
      as_optional<int>(map, "bit_depth",
                       JobOutputImageSequenceJpeg::kDefaultBitDepth, field));
  if (bit_depth != 8) {
    return Status::User(
        with_job_error(USER_ERROR_JOB_25, field + ".bit_depth (must be 8)."));
  }
  options.set_bit_depth(bit_depth);
  DAILYBOY_ASSIGN_OR_RETURN(
      int level,
      as_optional<int>(map, "compression_level",
                       JobOutputImageSequenceJpeg::kDefaultCompressionLevel,
                       field));
  if (level < 1 || level > 100) {
    return Status::User(
        with_job_error(USER_ERROR_JOB_25, field + ".compression_level."));
  }
  options.set_compression_level(level);
  DAILYBOY_ASSIGN_OR_RETURN(
      const std::string subsampling,
      as_optional<std::string>(map, "subsampling", "", field));
  if (!subsampling.empty() && !is_jpeg_subsampling(subsampling)) {
    return Status::User(with_job_error(
        USER_ERROR_JOB_25, field + ".subsampling '" + subsampling + "'."));
  }
  options.set_subsampling(subsampling);
  DAILYBOY_ASSIGN_OR_RETURN(
      bool progressive, as_optional<bool>(map, "progressive", false, field));
  options.set_progressive(progressive);
  return options;
}

StatusOr<JobOutputImageSequenceTiff> parse_tiff_options(
    const YAML::Node& map, const std::string& field) {
  static const char* kKeys[] = {"bit_depth", "bigtiff", "compression",
                                "compression_level"};
  DAILYBOY_RETURN_IF_ERROR(
      reject_unknown_keys(map, field, kKeys, std::size(kKeys)));
  JobOutputImageSequenceTiff options;
  if (map["bit_depth"]) {
    const std::string depth = tiff_bit_depth_string(map["bit_depth"]);
    if (!is_tiff_bit_depth(depth)) {
      return Status::User(with_job_error(
          USER_ERROR_JOB_25, field + ".bit_depth '" + depth + "'."));
    }
    options.set_bit_depth(depth);
  }
  DAILYBOY_ASSIGN_OR_RETURN(bool bigtiff,
                            as_optional<bool>(map, "bigtiff", false, field));
  options.set_bigtiff(bigtiff);
  DAILYBOY_ASSIGN_OR_RETURN(
      const std::string compression,
      as_optional<std::string>(map, "compression", "zip", field));
  if (!is_tiff_compression(compression)) {
    return Status::User(with_job_error(
        USER_ERROR_JOB_25, field + ".compression '" + compression + "'."));
  }
  options.set_compression(compression);
  DAILYBOY_ASSIGN_OR_RETURN(
      int level,
      as_optional<int>(map, "compression_level",
                       JobOutputImageSequenceTiff::kDefaultCompressionLevel,
                       field));
  if (level < 1 || level > 9) {
    return Status::User(
        with_job_error(USER_ERROR_JOB_25, field + ".compression_level."));
  }
  options.set_compression_level(level);
  return options;
}

bool exr_level_allowed(const std::string& compression) {
  return compression == "zip" || compression == "zips" ||
         compression == "dwaa" || compression == "dwab";
}

StatusOr<JobOutputImageSequenceExr> parse_exr_options(
    const YAML::Node& map, const std::string& field) {
  static const char* kKeys[] = {"bit_depth", "compression",
                                "compression_level"};
  DAILYBOY_RETURN_IF_ERROR(
      reject_unknown_keys(map, field, kKeys, std::size(kKeys)));
  JobOutputImageSequenceExr options;
  DAILYBOY_ASSIGN_OR_RETURN(
      const std::string bit_depth,
      as_optional<std::string>(map, "bit_depth", "h16", field));
  if (bit_depth != "h16" && bit_depth != "f32") {
    return Status::User(with_job_error(
        USER_ERROR_JOB_25, field + ".bit_depth '" + bit_depth + "'."));
  }
  options.set_bit_depth(bit_depth);
  DAILYBOY_ASSIGN_OR_RETURN(
      const std::string compression,
      as_optional<std::string>(map, "compression", "zip", field));
  if (!is_exr_compression(compression)) {
    return Status::User(with_job_error(
        USER_ERROR_JOB_25, field + ".compression '" + compression + "'."));
  }
  options.set_compression(compression);
  if (map["compression_level"]) {
    if (!exr_level_allowed(compression)) {
      return Status::User(with_job_error(USER_ERROR_JOB_94, field + "."));
    }
    DAILYBOY_ASSIGN_OR_RETURN(
        int level, as_required<int>(map, "compression_level", field));
    if ((compression == "zip" || compression == "zips") &&
        (level < 1 || level > 9)) {
      return Status::User(
          with_job_error(USER_ERROR_JOB_25, field + ".compression_level."));
    }
    if (level < 0) {
      return Status::User(
          with_job_error(USER_ERROR_JOB_25, field + ".compression_level."));
    }
    options.set_compression_level(level);
  }
  return options;
}

StatusOr<JobOutputImageSequenceHeif> parse_heif_options(
    const YAML::Node& map, const std::string& field, const std::string& ext) {
  static const char* kKeys[] = {"bit_depth", "compression",
                                "compression_level"};
  DAILYBOY_RETURN_IF_ERROR(
      reject_unknown_keys(map, field, kKeys, std::size(kKeys)));
  JobOutputImageSequenceHeif options;
  DAILYBOY_ASSIGN_OR_RETURN(
      int bit_depth,
      as_optional<int>(map, "bit_depth",
                       JobOutputImageSequenceHeif::kDefaultBitDepth, field));
  if (bit_depth != 8) {
    return Status::User(
        with_job_error(USER_ERROR_JOB_25, field + ".bit_depth (must be 8)."));
  }
  options.set_bit_depth(bit_depth);
  const std::string default_comp = (ext == ".avif") ? "avif" : "heic";
  DAILYBOY_ASSIGN_OR_RETURN(
      std::string compression,
      as_optional<std::string>(map, "compression", default_comp, field));
  if (compression == "hevc") {
    compression = "heic";
  }
  if (compression != "heic" && compression != "avif") {
    return Status::User(with_job_error(
        USER_ERROR_JOB_25, field + ".compression '" + compression + "'."));
  }
  options.set_compression(std::move(compression));
  DAILYBOY_ASSIGN_OR_RETURN(
      int level,
      as_optional<int>(map, "compression_level",
                       JobOutputImageSequenceHeif::kDefaultCompressionLevel,
                       field));
  if (level < 0 || level > 100) {
    return Status::User(
        with_job_error(USER_ERROR_JOB_25, field + ".compression_level."));
  }
  options.set_compression_level(level);
  return options;
}

}  // namespace

std::string image_sequence_format_from_path(const std::string& path_pattern) {
  const std::string ext = lower_ext(path_pattern);
  if (ext == ".png") {
    return "png";
  }
  if (ext == ".jpg" || ext == ".jpeg") {
    return "jpeg";
  }
  if (ext == ".tif" || ext == ".tiff") {
    return "tiff";
  }
  if (ext == ".exr") {
    return "exr";
  }
  if (ext == ".heif" || ext == ".heic" || ext == ".avif") {
    return "heif";
  }
  return {};
}

StatusOr<std::optional<JobOutputImageSequenceFormatOptions>>
parse_image_sequence_format_options(const YAML::Node& node,
                                    const std::string& path_pattern,
                                    const std::string& field) {
  if (!node) {
    return std::optional<JobOutputImageSequenceFormatOptions>{};
  }
  const std::string format = image_sequence_format_from_path(path_pattern);
  if (format.empty()) {
    return Status::User(with_job_error(USER_ERROR_JOB_93, field + "."));
  }
  DAILYBOY_ASSIGN_OR_RETURN(const YAML::Node map, expect_map(node, field));
  if (format == "png") {
    DAILYBOY_ASSIGN_OR_RETURN(JobOutputImageSequencePng options,
                              parse_png_options(map, field));
    return std::optional<JobOutputImageSequenceFormatOptions>{
        std::move(options)};
  }
  if (format == "jpeg") {
    DAILYBOY_ASSIGN_OR_RETURN(JobOutputImageSequenceJpeg options,
                              parse_jpeg_options(map, field));
    return std::optional<JobOutputImageSequenceFormatOptions>{
        std::move(options)};
  }
  if (format == "tiff") {
    DAILYBOY_ASSIGN_OR_RETURN(JobOutputImageSequenceTiff options,
                              parse_tiff_options(map, field));
    return std::optional<JobOutputImageSequenceFormatOptions>{
        std::move(options)};
  }
  if (format == "exr") {
    DAILYBOY_ASSIGN_OR_RETURN(JobOutputImageSequenceExr options,
                              parse_exr_options(map, field));
    return std::optional<JobOutputImageSequenceFormatOptions>{
        std::move(options)};
  }
  DAILYBOY_ASSIGN_OR_RETURN(
      JobOutputImageSequenceHeif options,
      parse_heif_options(map, field, lower_ext(path_pattern)));
  return std::optional<JobOutputImageSequenceFormatOptions>{std::move(options)};
}

}  // namespace dailyboy
