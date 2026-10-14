/*!
 * \file parse_output.cpp
 * \brief Parse job YAML output videos and image sequences.
 */

#include "job/parse_output.hpp"

#include <cctype>
#include <optional>

#include "job/parse_format_options.hpp"
#include "job/yaml_read.hpp"

namespace dailyboy {

namespace {

using H264Preset = JobOutputVideoH264::JobOutputVideoH264PresetValue;
using H264PixFmt = JobOutputVideoH264::JobOutputVideoH264PixFmtValue;
using H264Tune = JobOutputVideoH264::JobOutputVideoH264TuneValue;
using H264Profile = JobOutputVideoH264::JobOutputVideoH264ProfileValue;

bool path_has_unresolved_token(const std::string& path) {
  return path.find('{') != std::string::npos ||
         path.find('$') != std::string::npos;
}

bool path_ends_with_mov(const std::string& path) {
  if (path.size() < 4) {
    return false;
  }
  const char* s = path.data() + path.size() - 4;
  return s[0] == '.' && std::tolower(static_cast<unsigned char>(s[1])) == 'm' &&
         std::tolower(static_cast<unsigned char>(s[2])) == 'o' &&
         std::tolower(static_cast<unsigned char>(s[3])) == 'v';
}

StatusOr<std::string> read_optional_quoted_string(const YAML::Node& map,
                                                  const char* key,
                                                  std::string_view error,
                                                  const std::string& loc,
                                                  const std::string& fallback) {
  const YAML::Node node = map[key];
  if (!node || node.IsNull()) {
    return fallback;
  }
  if (!is_yaml_quoted_string(node)) {
    return Status::User(
        with_job_error(error, loc + " is " + describe_yaml_value(node) + "."));
  }
  return node.as<std::string>();
}

StatusOr<bool> read_yaml_bool(const YAML::Node& node, std::string_view error,
                              const std::string& loc) {
  bool value = false;
  if (!node || !node.IsScalar() || is_yaml_quoted_string(node) ||
      !parse_bool_scalar(node.as<std::string>(), value)) {
    return Status::User(
        with_job_error(error, loc + " is " + describe_yaml_value(node) + "."));
  }
  return value;
}

StatusOr<bool> read_optional_bool(const YAML::Node& map, const char* key,
                                  bool fallback, std::string_view error,
                                  const std::string& loc) {
  const YAML::Node node = map[key];
  if (!node || node.IsNull()) {
    return fallback;
  }
  return read_yaml_bool(node, error, loc);
}

StatusOr<H264Preset> parse_h264_preset(const YAML::Node& map,
                                       const std::string& field) {
  DAILYBOY_ASSIGN_OR_RETURN(
      const std::string preset,
      read_optional_quoted_string(map, "preset", USER_ERROR_JOB_172,
                                  field + ".preset", "medium"));
  if (preset == "ultrafast") {
    return H264Preset::Ultrafast;
  }
  if (preset == "superfast") {
    return H264Preset::Superfast;
  }
  if (preset == "veryfast") {
    return H264Preset::Veryfast;
  }
  if (preset == "faster") {
    return H264Preset::Faster;
  }
  if (preset == "fast") {
    return H264Preset::Fast;
  }
  if (preset == "medium") {
    return H264Preset::Medium;
  }
  if (preset == "slow") {
    return H264Preset::Slow;
  }
  return Status::User(
      with_job_error(USER_ERROR_JOB_173, field + ".preset '" + preset + "'."));
}

StatusOr<int> parse_h264_crf(const YAML::Node& map, const std::string& field) {
  const YAML::Node node = map["crf"];
  if (!node || node.IsNull()) {
    return JobOutputVideoH264::kDefaultCrf;
  }
  DAILYBOY_ASSIGN_OR_RETURN(
      const double crf,
      read_yaml_number(node, USER_ERROR_JOB_174, field + ".crf"));
  if (crf < 0.0 || crf > 51.0) {
    return Status::User(with_job_error(
        USER_ERROR_JOB_174, field + ".crf (got " + std::to_string(crf) + ")."));
  }
  return static_cast<int>(crf);
}

StatusOr<int> parse_h264_bitrate(const YAML::Node& map,
                                 const std::string& field) {
  const YAML::Node node = map["bitrate_kbps"];
  if (!node || node.IsNull()) {
    return 0;
  }
  DAILYBOY_ASSIGN_OR_RETURN(
      const int bitrate,
      read_yaml_integer(node, USER_ERROR_JOB_175, field + ".bitrate_kbps"));
  if (bitrate < 1) {
    return Status::User(with_job_error(USER_ERROR_JOB_175,
                                       field + ".bitrate_kbps (got integer " +
                                           std::to_string(bitrate) + ")."));
  }
  return bitrate;
}

StatusOr<int> parse_h264_gop(const YAML::Node& map, const std::string& field) {
  const YAML::Node node = map["gop"];
  if (!node || node.IsNull()) {
    return JobOutputVideoH264::kDefaultGop;
  }
  DAILYBOY_ASSIGN_OR_RETURN(
      const int gop,
      read_yaml_integer(node, USER_ERROR_JOB_176, field + ".gop"));
  if (gop < 1) {
    return Status::User(with_job_error(
        USER_ERROR_JOB_176,
        field + ".gop (got integer " + std::to_string(gop) + ")."));
  }
  return gop;
}

StatusOr<H264PixFmt> parse_h264_pix_fmt(const YAML::Node& map,
                                        const std::string& field) {
  DAILYBOY_ASSIGN_OR_RETURN(
      const std::string pix_fmt,
      read_optional_quoted_string(map, "pix_fmt", USER_ERROR_JOB_177,
                                  field + ".pix_fmt", "yuv420p"));
  if (pix_fmt == "yuv420p") {
    return H264PixFmt::Yuv420p;
  }
  if (pix_fmt == "yuv422p") {
    return H264PixFmt::Yuv422p;
  }
  return Status::User(with_job_error(USER_ERROR_JOB_178,
                                     field + ".pix_fmt '" + pix_fmt + "'."));
}

StatusOr<H264Tune> parse_h264_tune(const YAML::Node& map,
                                   const std::string& field) {
  DAILYBOY_ASSIGN_OR_RETURN(
      const std::string tune,
      read_optional_quoted_string(map, "tune", USER_ERROR_JOB_179,
                                  field + ".tune", ""));
  if (tune.empty()) {
    return H264Tune::Unspecified;
  }
  if (tune == "film") {
    return H264Tune::Film;
  }
  if (tune == "animation") {
    return H264Tune::Animation;
  }
  if (tune == "grain") {
    return H264Tune::Grain;
  }
  if (tune == "stillimage") {
    return H264Tune::Stillimage;
  }
  if (tune == "fastdecode") {
    return H264Tune::Fastdecode;
  }
  if (tune == "zerolatency") {
    return H264Tune::Zerolatency;
  }
  return Status::User(
      with_job_error(USER_ERROR_JOB_180, field + ".tune '" + tune + "'."));
}

StatusOr<H264Profile> parse_h264_profile(const YAML::Node& map,
                                         const std::string& field) {
  DAILYBOY_ASSIGN_OR_RETURN(
      const std::string profile,
      read_optional_quoted_string(map, "profile", USER_ERROR_JOB_181,
                                  field + ".profile", ""));
  if (profile.empty()) {
    return H264Profile::Unspecified;
  }
  if (profile == "baseline") {
    return H264Profile::Baseline;
  }
  if (profile == "main") {
    return H264Profile::Main;
  }
  if (profile == "high") {
    return H264Profile::High;
  }
  if (profile == "high422") {
    return H264Profile::High422;
  }
  return Status::User(with_job_error(USER_ERROR_JOB_182,
                                     field + ".profile '" + profile + "'."));
}

Status check_h264_profile_pix_fmt(H264Profile profile, H264PixFmt pix_fmt,
                                  const std::string& field) {
  const bool is_422 = pix_fmt == H264PixFmt::Yuv422p;
  if (profile == H264Profile::High422 && !is_422) {
    return Status::User(with_job_error(USER_ERROR_JOB_26, field + "."));
  }
  if (is_422 &&
      (profile == H264Profile::Baseline || profile == H264Profile::Main ||
       profile == H264Profile::High)) {
    return Status::User(with_job_error(USER_ERROR_JOB_27, field + "."));
  }
  return Status::Ok();
}

}  // namespace

std::string canonicalize_h264_level(const std::string& raw) {
  if (raw == "3" || raw == "3.0") {
    return "3.0";
  }
  if (raw == "3.1") {
    return "3.1";
  }
  if (raw == "4" || raw == "4.0") {
    return "4.0";
  }
  if (raw == "4.1") {
    return "4.1";
  }
  if (raw == "4.2") {
    return "4.2";
  }
  if (raw == "5" || raw == "5.0") {
    return "5.0";
  }
  if (raw == "5.1") {
    return "5.1";
  }
  if (raw == "5.2") {
    return "5.2";
  }
  return {};
}

StatusOr<std::string> parse_h264_level(const YAML::Node& map,
                                       const std::string& field) {
  const YAML::Node level_node = map["level"];
  if (!level_node) {
    return std::string();
  }
  if (!level_node.IsScalar()) {
    return Status::User(with_job_error(USER_ERROR_JOB_28, field + "."));
  }
  std::string level = canonicalize_h264_level(level_node.Scalar());
  if (level.empty()) {
    return Status::User(with_job_error(
        USER_ERROR_JOB_184, field + ".level '" + level_node.Scalar() + "'."));
  }
  return level;
}

StatusOr<JobOutputVideoH264> parse_h264(const YAML::Node& node,
                                        const std::string& field) {
  JobOutputVideoH264 h264;
  if (!node) {
    return h264;
  }
  DAILYBOY_ASSIGN_OR_RETURN(const YAML::Node map, expect_map(node, field));
  if (map["crf"] && map["bitrate_kbps"]) {
    return Status::User(with_job_error(USER_ERROR_JOB_24, field + "."));
  }
  DAILYBOY_ASSIGN_OR_RETURN(const H264Preset preset,
                            parse_h264_preset(map, field));
  DAILYBOY_ASSIGN_OR_RETURN(const int crf, parse_h264_crf(map, field));
  DAILYBOY_ASSIGN_OR_RETURN(const int gop, parse_h264_gop(map, field));
  DAILYBOY_ASSIGN_OR_RETURN(const int bitrate_kbps,
                            parse_h264_bitrate(map, field));
  DAILYBOY_ASSIGN_OR_RETURN(const H264PixFmt pix_fmt,
                            parse_h264_pix_fmt(map, field));
  DAILYBOY_ASSIGN_OR_RETURN(const H264Tune tune, parse_h264_tune(map, field));
  DAILYBOY_ASSIGN_OR_RETURN(const H264Profile profile,
                            parse_h264_profile(map, field));
  DAILYBOY_RETURN_IF_ERROR(check_h264_profile_pix_fmt(profile, pix_fmt, field));
  DAILYBOY_ASSIGN_OR_RETURN(std::string level, parse_h264_level(map, field));
  DAILYBOY_ASSIGN_OR_RETURN(
      const bool faststart,
      read_optional_bool(map, "faststart", true, USER_ERROR_JOB_185,
                         field + ".faststart"));
  h264.set_preset(preset);
  h264.set_crf(crf);
  h264.set_gop(gop);
  h264.set_bitrate_kbps(bitrate_kbps);
  h264.set_pix_fmt(pix_fmt);
  h264.set_tune(tune);
  h264.set_profile(profile);
  h264.set_level(std::move(level));
  h264.set_faststart(faststart);
  return h264;
}

StatusOr<int> parse_mjpeg_qscale(const YAML::Node& map,
                                 const std::string& field) {
  const YAML::Node node = map["qscale"];
  if (!node || node.IsNull()) {
    return JobOutputVideoMjpeg::kDefaultQscale;
  }
  DAILYBOY_ASSIGN_OR_RETURN(
      const int qscale,
      read_yaml_integer(node, USER_ERROR_JOB_29, field + ".qscale"));
  if (qscale < 1 || qscale > 31) {
    return Status::User(with_job_error(
        USER_ERROR_JOB_29,
        field + ".qscale (got integer " + std::to_string(qscale) + ")."));
  }
  return qscale;
}

StatusOr<JobOutputVideoMjpeg::JobOutputVideoMjpegPixFmtValue>
parse_mjpeg_pix_fmt(const YAML::Node& map, const std::string& field) {
  using Pix = JobOutputVideoMjpeg::JobOutputVideoMjpegPixFmtValue;
  DAILYBOY_ASSIGN_OR_RETURN(
      const std::string pix_fmt,
      read_optional_quoted_string(map, "pix_fmt", USER_ERROR_JOB_186,
                                  field + ".pix_fmt", "yuv422p"));
  if (pix_fmt == "yuv420p") {
    return Pix::Yuv420p;
  }
  if (pix_fmt == "yuv422p") {
    return Pix::Yuv422p;
  }
  if (pix_fmt == "yuv444p") {
    return Pix::Yuv444p;
  }
  return Status::User(with_job_error(USER_ERROR_JOB_187,
                                     field + ".pix_fmt '" + pix_fmt + "'."));
}

StatusOr<JobOutputVideoMjpeg::JobOutputVideoMjpegHuffmanValue>
parse_mjpeg_huffman(const YAML::Node& map, const std::string& field) {
  using Huffman = JobOutputVideoMjpeg::JobOutputVideoMjpegHuffmanValue;
  DAILYBOY_ASSIGN_OR_RETURN(
      const std::string huffman,
      read_optional_quoted_string(map, "huffman", USER_ERROR_JOB_188,
                                  field + ".huffman", "optimal"));
  if (huffman == "default") {
    return Huffman::Default;
  }
  if (huffman == "optimal") {
    return Huffman::Optimal;
  }
  return Status::User(with_job_error(USER_ERROR_JOB_189,
                                     field + ".huffman '" + huffman + "'."));
}

StatusOr<JobOutputVideoMjpeg> parse_mjpeg(const YAML::Node& node,
                                          const std::string& field) {
  JobOutputVideoMjpeg mjpeg;
  if (!node) {
    return mjpeg;
  }
  DAILYBOY_ASSIGN_OR_RETURN(const YAML::Node map, expect_map(node, field));
  DAILYBOY_ASSIGN_OR_RETURN(const int qscale, parse_mjpeg_qscale(map, field));
  DAILYBOY_ASSIGN_OR_RETURN(
      const JobOutputVideoMjpeg::JobOutputVideoMjpegPixFmtValue pix_fmt,
      parse_mjpeg_pix_fmt(map, field));
  DAILYBOY_ASSIGN_OR_RETURN(
      const JobOutputVideoMjpeg::JobOutputVideoMjpegHuffmanValue huffman,
      parse_mjpeg_huffman(map, field));
  DAILYBOY_ASSIGN_OR_RETURN(
      const bool faststart,
      read_optional_bool(map, "faststart", true, USER_ERROR_JOB_185,
                         field + ".faststart"));
  mjpeg.set_qscale(qscale);
  mjpeg.set_pix_fmt(pix_fmt);
  mjpeg.set_huffman(huffman);
  mjpeg.set_faststart(faststart);
  return mjpeg;
}

using DnxhdProfile = JobOutputVideoDnxhd::JobOutputVideoDnxhdProfileValue;
using DnxhdPixFmt = JobOutputVideoDnxhd::JobOutputVideoDnxhdPixFmtValue;

bool dnxhd_is_hr(DnxhdProfile profile) {
  return profile != DnxhdProfile::Dnxhd;
}

DnxhdPixFmt dnxhd_default_pix_fmt(DnxhdProfile profile) {
  switch (profile) {
    case DnxhdProfile::DnxhrHqx:
      return DnxhdPixFmt::Yuv422p10;
    case DnxhdProfile::Dnxhr444:
      return DnxhdPixFmt::Yuv444p10;
    case DnxhdProfile::Dnxhd:
    case DnxhdProfile::DnxhrLb:
    case DnxhdProfile::DnxhrSq:
    case DnxhdProfile::DnxhrHq:
      return DnxhdPixFmt::Yuv422p;
  }
  return DnxhdPixFmt::Yuv422p;
}

bool dnxhd_pix_fmt_ok(DnxhdProfile profile, DnxhdPixFmt pix_fmt) {
  switch (profile) {
    case DnxhdProfile::Dnxhd:
      return pix_fmt == DnxhdPixFmt::Yuv422p ||
             pix_fmt == DnxhdPixFmt::Yuv422p10;
    case DnxhdProfile::DnxhrLb:
    case DnxhdProfile::DnxhrSq:
    case DnxhdProfile::DnxhrHq:
      return pix_fmt == DnxhdPixFmt::Yuv422p;
    case DnxhdProfile::DnxhrHqx:
      return pix_fmt == DnxhdPixFmt::Yuv422p10;
    case DnxhdProfile::Dnxhr444:
      return pix_fmt == DnxhdPixFmt::Yuv444p10;
  }
  return false;
}

StatusOr<DnxhdProfile> parse_dnxhd_profile(const YAML::Node& map,
                                           const std::string& field) {
  DAILYBOY_ASSIGN_OR_RETURN(
      const std::string profile,
      read_optional_quoted_string(map, "profile", USER_ERROR_JOB_190,
                                  field + ".profile", "dnxhr_hq"));
  if (profile == "dnxhd") {
    return DnxhdProfile::Dnxhd;
  }
  if (profile == "dnxhr_lb") {
    return DnxhdProfile::DnxhrLb;
  }
  if (profile == "dnxhr_sq") {
    return DnxhdProfile::DnxhrSq;
  }
  if (profile == "dnxhr_hq") {
    return DnxhdProfile::DnxhrHq;
  }
  if (profile == "dnxhr_hqx") {
    return DnxhdProfile::DnxhrHqx;
  }
  if (profile == "dnxhr_444") {
    return DnxhdProfile::Dnxhr444;
  }
  return Status::User(with_job_error(USER_ERROR_JOB_191,
                                     field + ".profile '" + profile + "'."));
}

StatusOr<int> parse_dnxhd_bitrate(const YAML::Node& map, bool hr,
                                  const std::string& field) {
  if (hr && map["bitrate_kbps"]) {
    return Status::User(with_job_error(USER_ERROR_JOB_30, field + "."));
  }
  if (hr) {
    return 0;
  }
  if (!map["bitrate_kbps"] || map["bitrate_kbps"].IsNull()) {
    return Status::User(with_job_error(USER_ERROR_JOB_31, field + "."));
  }
  DAILYBOY_ASSIGN_OR_RETURN(
      const int bitrate,
      read_yaml_integer(map["bitrate_kbps"], USER_ERROR_JOB_32,
                        field + ".bitrate_kbps"));
  if (bitrate < 1) {
    return Status::User(with_job_error(USER_ERROR_JOB_32,
                                       field + ".bitrate_kbps (got integer " +
                                           std::to_string(bitrate) + ")."));
  }
  return bitrate;
}

StatusOr<DnxhdPixFmt> parse_dnxhd_pix_fmt(const YAML::Node& map,
                                          DnxhdProfile profile,
                                          const std::string& field) {
  if (!map["pix_fmt"] || map["pix_fmt"].IsNull()) {
    return dnxhd_default_pix_fmt(profile);
  }
  if (!is_yaml_quoted_string(map["pix_fmt"])) {
    return Status::User(with_job_error(
        USER_ERROR_JOB_192,
        field + ".pix_fmt is " + describe_yaml_value(map["pix_fmt"]) + "."));
  }
  const std::string pix_fmt = map["pix_fmt"].as<std::string>();
  DnxhdPixFmt value = DnxhdPixFmt::Yuv422p;
  if (pix_fmt == "yuv422p") {
    value = DnxhdPixFmt::Yuv422p;
  } else if (pix_fmt == "yuv422p10") {
    value = DnxhdPixFmt::Yuv422p10;
  } else if (pix_fmt == "yuv444p10") {
    value = DnxhdPixFmt::Yuv444p10;
  } else {
    return Status::User(with_job_error(USER_ERROR_JOB_193,
                                       field + ".pix_fmt '" + pix_fmt + "'."));
  }
  if (!dnxhd_pix_fmt_ok(profile, value)) {
    return Status::User(with_job_error(USER_ERROR_JOB_33,
                                       field + " pix_fmt '" + pix_fmt + "'."));
  }
  return value;
}

StatusOr<JobOutputVideoDnxhd> parse_dnxhd(const YAML::Node& node,
                                          const std::string& field) {
  JobOutputVideoDnxhd dnxhd;
  if (!node) {
    return dnxhd;
  }
  DAILYBOY_ASSIGN_OR_RETURN(const YAML::Node map, expect_map(node, field));
  DAILYBOY_ASSIGN_OR_RETURN(const DnxhdProfile profile,
                            parse_dnxhd_profile(map, field));
  const bool hr = dnxhd_is_hr(profile);
  DAILYBOY_ASSIGN_OR_RETURN(const int bitrate,
                            parse_dnxhd_bitrate(map, hr, field));
  DAILYBOY_ASSIGN_OR_RETURN(const DnxhdPixFmt pix_fmt,
                            parse_dnxhd_pix_fmt(map, profile, field));
  DAILYBOY_ASSIGN_OR_RETURN(
      const bool interlaced,
      read_optional_bool(map, "interlaced", false, USER_ERROR_JOB_194,
                         field + ".interlaced"));
  if (hr && interlaced) {
    return Status::User(with_job_error(USER_ERROR_JOB_34, field + "."));
  }
  DAILYBOY_ASSIGN_OR_RETURN(
      const bool nitris_compat,
      read_optional_bool(map, "nitris_compat", false, USER_ERROR_JOB_195,
                         field + ".nitris_compat"));
  DAILYBOY_ASSIGN_OR_RETURN(
      const bool faststart,
      read_optional_bool(map, "faststart", true, USER_ERROR_JOB_185,
                         field + ".faststart"));
  dnxhd.set_profile(profile);
  if (!hr) {
    dnxhd.set_bitrate_kbps(bitrate);
  }
  dnxhd.set_pix_fmt(pix_fmt);
  dnxhd.set_interlaced(interlaced);
  dnxhd.set_nitris_compat(nitris_compat);
  dnxhd.set_faststart(faststart);
  return dnxhd;
}

using ProresProfile = JobOutputVideoProres::JobOutputVideoProresProfileValue;
using ProresPixFmt = JobOutputVideoProres::JobOutputVideoProresPixFmtValue;
using ProresQuantMat = JobOutputVideoProres::JobOutputVideoProresQuantMatValue;

bool prores_is_444(ProresProfile p) {
  return p == ProresProfile::FourFourFourFour ||
         p == ProresProfile::FourFourFourFourXq;
}

ProresPixFmt prores_default_pix_fmt(ProresProfile profile) {
  return prores_is_444(profile) ? ProresPixFmt::Yuv444p10
                                : ProresPixFmt::Yuv422p10;
}

bool prores_pix_fmt_ok(ProresProfile profile, ProresPixFmt pix_fmt) {
  if (prores_is_444(profile)) {
    return pix_fmt == ProresPixFmt::Yuv444p10;
  }
  return pix_fmt == ProresPixFmt::Yuv422p10;
}

bool prores_vendor_ok(const std::string& vendor) {
  if (vendor.size() != 4) {
    return false;
  }
  for (unsigned char c : vendor) {
    if (c < 0x20 || c > 0x7e) {
      return false;
    }
  }
  return true;
}

StatusOr<std::string> read_prores_profile_scalar(const YAML::Node& map,
                                                 const std::string& field) {
  const YAML::Node node = map["profile"];
  if (!node || node.IsNull()) {
    return std::string("hq");
  }
  if (!node.IsScalar()) {
    return Status::User(with_job_error(
        USER_ERROR_JOB_197,
        field + ".profile is " + describe_yaml_value(node) + "."));
  }
  if (is_yaml_quoted_string(node)) {
    return node.as<std::string>();
  }
  if (node.Scalar() == "4444") {
    return std::string("4444");
  }
  return Status::User(
      with_job_error(USER_ERROR_JOB_197,
                     field + ".profile is " + describe_yaml_value(node) + "."));
}

StatusOr<ProresProfile> parse_prores_profile(const YAML::Node& map,
                                             const std::string& field) {
  DAILYBOY_ASSIGN_OR_RETURN(const std::string profile,
                            read_prores_profile_scalar(map, field));
  if (profile == "proxy") {
    return ProresProfile::Proxy;
  }
  if (profile == "lt") {
    return ProresProfile::Lt;
  }
  if (profile == "standard") {
    return ProresProfile::Standard;
  }
  if (profile == "hq") {
    return ProresProfile::Hq;
  }
  if (profile == "4444") {
    return ProresProfile::FourFourFourFour;
  }
  if (profile == "4444xq") {
    return ProresProfile::FourFourFourFourXq;
  }
  return Status::User(with_job_error(USER_ERROR_JOB_198,
                                     field + ".profile '" + profile + "'."));
}

StatusOr<ProresPixFmt> parse_prores_pix_fmt(const YAML::Node& map,
                                            ProresProfile profile,
                                            const std::string& field) {
  if (!map["pix_fmt"] || map["pix_fmt"].IsNull()) {
    return prores_default_pix_fmt(profile);
  }
  if (!is_yaml_quoted_string(map["pix_fmt"])) {
    return Status::User(with_job_error(
        USER_ERROR_JOB_199,
        field + ".pix_fmt is " + describe_yaml_value(map["pix_fmt"]) + "."));
  }
  const std::string pix_fmt = map["pix_fmt"].as<std::string>();
  ProresPixFmt value = ProresPixFmt::Yuv422p10;
  if (pix_fmt == "yuv422p10") {
    value = ProresPixFmt::Yuv422p10;
  } else if (pix_fmt == "yuv444p10") {
    value = ProresPixFmt::Yuv444p10;
  } else {
    return Status::User(with_job_error(USER_ERROR_JOB_200,
                                       field + ".pix_fmt '" + pix_fmt + "'."));
  }
  if (!prores_pix_fmt_ok(profile, value)) {
    return Status::User(with_job_error(USER_ERROR_JOB_95,
                                       field + " pix_fmt '" + pix_fmt + "'."));
  }
  return value;
}

StatusOr<ProresQuantMat> parse_prores_quant_mat(const YAML::Node& map,
                                                const std::string& field) {
  DAILYBOY_ASSIGN_OR_RETURN(
      const std::string quant_mat,
      read_optional_quoted_string(map, "quant_mat", USER_ERROR_JOB_201,
                                  field + ".quant_mat", "auto"));
  if (quant_mat == "auto") {
    return ProresQuantMat::Auto;
  }
  if (quant_mat == "proxy") {
    return ProresQuantMat::Proxy;
  }
  if (quant_mat == "lt") {
    return ProresQuantMat::Lt;
  }
  if (quant_mat == "standard") {
    return ProresQuantMat::Standard;
  }
  if (quant_mat == "hq") {
    return ProresQuantMat::Hq;
  }
  if (quant_mat == "default") {
    return ProresQuantMat::Default;
  }
  return Status::User(with_job_error(
      USER_ERROR_JOB_202, field + ".quant_mat '" + quant_mat + "'."));
}

StatusOr<int> parse_prores_bits_per_mb(const YAML::Node& map,
                                       const std::string& field) {
  const YAML::Node node = map["bits_per_mb"];
  if (!node || node.IsNull()) {
    return 0;
  }
  DAILYBOY_ASSIGN_OR_RETURN(
      const int bits,
      read_yaml_integer(node, USER_ERROR_JOB_98, field + ".bits_per_mb"));
  if (bits < 0 || bits > 8192) {
    return Status::User(with_job_error(
        USER_ERROR_JOB_98,
        field + ".bits_per_mb (got integer " + std::to_string(bits) + ")."));
  }
  return bits;
}

StatusOr<int> parse_prores_mbs_per_slice(const YAML::Node& map,
                                         const std::string& field) {
  const YAML::Node node = map["mbs_per_slice"];
  if (!node || node.IsNull()) {
    return JobOutputVideoProres::kDefaultMbsPerSlice;
  }
  DAILYBOY_ASSIGN_OR_RETURN(
      const int slices,
      read_yaml_integer(node, USER_ERROR_JOB_99, field + ".mbs_per_slice"));
  if (slices < 1 || slices > 8) {
    return Status::User(with_job_error(USER_ERROR_JOB_99,
                                       field + ".mbs_per_slice (got integer " +
                                           std::to_string(slices) + ")."));
  }
  return slices;
}

StatusOr<std::string> parse_prores_vendor(const YAML::Node& map,
                                          const std::string& field) {
  DAILYBOY_ASSIGN_OR_RETURN(
      std::string vendor,
      read_optional_quoted_string(map, "vendor", USER_ERROR_JOB_203,
                                  field + ".vendor",
                                  JobOutputVideoProres::kDefaultVendor));
  if (!prores_vendor_ok(vendor)) {
    return Status::User(with_job_error(USER_ERROR_JOB_96, field + "."));
  }
  return vendor;
}

StatusOr<JobOutputVideoProres> parse_prores(const YAML::Node& node,
                                            const std::string& field) {
  JobOutputVideoProres prores;
  if (!node) {
    return prores;
  }
  DAILYBOY_ASSIGN_OR_RETURN(const YAML::Node map, expect_map(node, field));
  DAILYBOY_ASSIGN_OR_RETURN(const ProresProfile profile,
                            parse_prores_profile(map, field));
  DAILYBOY_ASSIGN_OR_RETURN(const ProresPixFmt pix_fmt,
                            parse_prores_pix_fmt(map, profile, field));
  DAILYBOY_ASSIGN_OR_RETURN(const ProresQuantMat quant_mat,
                            parse_prores_quant_mat(map, field));
  DAILYBOY_ASSIGN_OR_RETURN(const int bits_per_mb,
                            parse_prores_bits_per_mb(map, field));
  DAILYBOY_ASSIGN_OR_RETURN(const int mbs_per_slice,
                            parse_prores_mbs_per_slice(map, field));
  DAILYBOY_ASSIGN_OR_RETURN(std::string vendor,
                            parse_prores_vendor(map, field));
  if (map["alpha_bits"]) {
    return Status::User(with_job_error(USER_ERROR_JOB_97, field + "."));
  }
  DAILYBOY_ASSIGN_OR_RETURN(
      const bool faststart,
      read_optional_bool(map, "faststart", true, USER_ERROR_JOB_185,
                         field + ".faststart"));
  prores.set_profile(profile);
  prores.set_pix_fmt(pix_fmt);
  prores.set_quant_mat(quant_mat);
  prores.set_bits_per_mb(bits_per_mb);
  prores.set_mbs_per_slice(mbs_per_slice);
  prores.set_vendor(std::move(vendor));
  prores.set_faststart(faststart);
  return prores;
}

bool is_output_video_field(const std::string& field) {
  return field.rfind("output.videos[", 0) == 0;
}

StatusOr<JobOutputDisplayView> parse_display_view(const YAML::Node& node,
                                                  const std::string& field) {
  const bool video = is_output_video_field(field);
  const std::string_view omit_block =
      video ? USER_ERROR_JOB_147 : USER_ERROR_JOB_208;
  if (!node || node.IsNull()) {
    return Status::User(with_job_error(omit_block, field + "."));
  }
  DAILYBOY_ASSIGN_OR_RETURN(const YAML::Node map, expect_map(node, field));
  const std::string_view omit_display =
      video ? USER_ERROR_JOB_148 : USER_ERROR_JOB_209;
  const std::string_view omit_view =
      video ? USER_ERROR_JOB_150 : USER_ERROR_JOB_211;
  const std::string_view bad_display =
      video ? USER_ERROR_JOB_149 : USER_ERROR_JOB_210;
  const std::string_view bad_view =
      video ? USER_ERROR_JOB_151 : USER_ERROR_JOB_212;
  if (!map["display"] || map["display"].IsNull()) {
    return Status::User(with_job_error(omit_display, field + ".display."));
  }
  if (!map["view"] || map["view"].IsNull()) {
    return Status::User(with_job_error(omit_view, field + ".view."));
  }
  DAILYBOY_ASSIGN_OR_RETURN(
      std::string display,
      read_quoted_nonempty_string(map["display"], bad_display,
                                  field + ".display"));
  DAILYBOY_ASSIGN_OR_RETURN(
      std::string view,
      read_quoted_nonempty_string(map["view"], bad_view, field + ".view"));
  JobOutputDisplayView display_view;
  display_view.set_display(std::move(display));
  display_view.set_view(std::move(view));
  return display_view;
}

StatusOr<std::string> require_signal_quoted(const YAML::Node& map,
                                            const char* key,
                                            std::string_view omit_error,
                                            std::string_view type_error,
                                            const std::string& field) {
  const YAML::Node node = map[key];
  if (!node || node.IsNull()) {
    return Status::User(with_job_error(omit_error, field + "." + key + "."));
  }
  return read_quoted_nonempty_string(node, type_error, field + "." + key);
}

StatusOr<JobOutputVideoSignal::RangeValue> parse_signal_range(
    const std::string& range, const std::string& field, bool dnxhd_codec) {
  if (range == "tv") {
    return JobOutputVideoSignal::RangeValue::Tv;
  }
  if (range == "pc") {
    if (dnxhd_codec) {
      return Status::User(with_job_error(USER_ERROR_JOB_196,
                                         field + ".range '" + range + "'."));
    }
    return JobOutputVideoSignal::RangeValue::Pc;
  }
  return Status::User(
      with_job_error(USER_ERROR_JOB_155, field + ".range '" + range + "'."));
}

StatusOr<JobOutputVideoSignal> parse_video_signal(const YAML::Node& node,
                                                  const std::string& field,
                                                  bool dnxhd_codec) {
  if (!node || node.IsNull()) {
    return Status::User(with_job_error(USER_ERROR_JOB_92, field + "."));
  }
  DAILYBOY_ASSIGN_OR_RETURN(const YAML::Node map, expect_map(node, field));
  DAILYBOY_ASSIGN_OR_RETURN(
      const std::string range,
      require_signal_quoted(map, "range", USER_ERROR_JOB_153,
                            USER_ERROR_JOB_154, field));
  DAILYBOY_ASSIGN_OR_RETURN(
      const std::string matrix,
      require_signal_quoted(map, "matrix", USER_ERROR_JOB_156,
                            USER_ERROR_JOB_157, field));
  DAILYBOY_ASSIGN_OR_RETURN(
      const std::string primaries,
      require_signal_quoted(map, "primaries", USER_ERROR_JOB_159,
                            USER_ERROR_JOB_160, field));
  DAILYBOY_ASSIGN_OR_RETURN(
      const std::string transfer,
      require_signal_quoted(map, "transfer", USER_ERROR_JOB_162,
                            USER_ERROR_JOB_163, field));
  JobOutputVideoSignal signal;
  DAILYBOY_ASSIGN_OR_RETURN(const JobOutputVideoSignal::RangeValue range_value,
                            parse_signal_range(range, field, dnxhd_codec));
  signal.set_range(range_value);
  if (matrix != "bt709") {
    return Status::User(with_job_error(USER_ERROR_JOB_158,
                                       field + ".matrix '" + matrix + "'."));
  }
  signal.set_matrix(JobOutputVideoSignal::MatrixValue::Bt709);
  if (primaries != "bt709") {
    return Status::User(with_job_error(
        USER_ERROR_JOB_161, field + ".primaries '" + primaries + "'."));
  }
  signal.set_primaries(JobOutputVideoSignal::PrimariesValue::Bt709);
  if (transfer != "bt709") {
    return Status::User(with_job_error(
        USER_ERROR_JOB_164, field + ".transfer '" + transfer + "'."));
  }
  signal.set_transfer(JobOutputVideoSignal::TransferValue::Bt709);
  return signal;
}

StatusOr<std::string> read_video_path(const YAML::Node& node,
                                      const std::string& field) {
  DAILYBOY_ASSIGN_OR_RETURN(
      std::string path,
      read_quoted_nonempty_string(node, USER_ERROR_JOB_166, field + ".path"));
  if (!path_has_unresolved_token(path) && !path_ends_with_mov(path)) {
    return Status::User(
        with_job_error(USER_ERROR_JOB_167, field + ".path '" + path + "'."));
  }
  return path;
}

StatusOr<int> read_video_fps(const YAML::Node& map, const std::string& field) {
  const YAML::Node node = map["fps"];
  if (!node || node.IsNull()) {
    return 24;
  }
  DAILYBOY_ASSIGN_OR_RETURN(
      const int fps,
      read_yaml_integer(node, USER_ERROR_JOB_168, field + ".fps"));
  if (fps < 1) {
    return Status::User(with_job_error(
        USER_ERROR_JOB_168,
        field + ".fps (got integer " + std::to_string(fps) + ")."));
  }
  return fps;
}

StatusOr<std::string> read_video_id(const YAML::Node& map,
                                    const std::string& field) {
  if (!map["id"] || map["id"].IsNull()) {
    return Status::User(with_job_error(USER_ERROR_JOB_143, field + ".id."));
  }
  return read_quoted_nonempty_string(map["id"], USER_ERROR_JOB_144,
                                     field + ".id");
}

StatusOr<bool> read_video_enabled(const YAML::Node& map,
                                  const std::string& field) {
  if (!map["enabled"] || map["enabled"].IsNull()) {
    return Status::User(
        with_job_error(USER_ERROR_JOB_145, field + ".enabled."));
  }
  return read_yaml_bool(map["enabled"], USER_ERROR_JOB_146, field + ".enabled");
}

StatusOr<std::string> read_video_codec(const YAML::Node& map,
                                       const std::string& field) {
  if (!map["codec"] || map["codec"].IsNull()) {
    return Status::User(with_job_error(USER_ERROR_JOB_169, field + ".codec."));
  }
  return read_quoted_nonempty_string(map["codec"], USER_ERROR_JOB_170,
                                     field + ".codec");
}

Status set_video_codec_options(JobOutputVideo& video, const std::string& codec,
                               const YAML::Node& map,
                               const std::string& field) {
  const std::string options_field = field + ".codec_options";
  if (codec == "h264") {
    video.set_codec(JobOutputVideo::JobOutputVideoCodecValue::H264);
    DAILYBOY_ASSIGN_OR_RETURN(JobOutputVideoH264 options,
                              parse_h264(map["codec_options"], options_field));
    video.set_codec_options(options);
    return Status::Ok();
  }
  if (codec == "mjpeg") {
    video.set_codec(JobOutputVideo::JobOutputVideoCodecValue::Mjpeg);
    DAILYBOY_ASSIGN_OR_RETURN(JobOutputVideoMjpeg options,
                              parse_mjpeg(map["codec_options"], options_field));
    video.set_codec_options(options);
    return Status::Ok();
  }
  if (codec == "dnxhd") {
    video.set_codec(JobOutputVideo::JobOutputVideoCodecValue::Dnxhd);
    DAILYBOY_ASSIGN_OR_RETURN(JobOutputVideoDnxhd options,
                              parse_dnxhd(map["codec_options"], options_field));
    video.set_codec_options(options);
    return Status::Ok();
  }
  if (codec == "prores") {
    video.set_codec(JobOutputVideo::JobOutputVideoCodecValue::Prores);
    DAILYBOY_ASSIGN_OR_RETURN(
        JobOutputVideoProres options,
        parse_prores(map["codec_options"], options_field));
    video.set_codec_options(options);
    return Status::Ok();
  }
  return Status::User(
      with_job_error(USER_ERROR_JOB_171, field + ".codec '" + codec + "'."));
}

StatusOr<std::string> read_image_sequence_id(const YAML::Node& map,
                                             const std::string& field) {
  if (!map["id"] || map["id"].IsNull()) {
    return Status::User(with_job_error(USER_ERROR_JOB_204, field + ".id."));
  }
  return read_quoted_nonempty_string(map["id"], USER_ERROR_JOB_205,
                                     field + ".id");
}

StatusOr<bool> read_image_sequence_enabled(const YAML::Node& map,
                                           const std::string& field) {
  if (!map["enabled"] || map["enabled"].IsNull()) {
    return Status::User(
        with_job_error(USER_ERROR_JOB_206, field + ".enabled."));
  }
  return read_yaml_bool(map["enabled"], USER_ERROR_JOB_207, field + ".enabled");
}

StatusOr<std::string> read_image_sequence_path_pattern(
    const YAML::Node& map, const std::string& field) {
  if (!map["path_pattern"] || map["path_pattern"].IsNull()) {
    return Status::User(
        with_job_error(USER_ERROR_JOB_213, field + ".path_pattern."));
  }
  return read_quoted_nonempty_string(map["path_pattern"], USER_ERROR_JOB_214,
                                     field + ".path_pattern");
}

StatusOr<JobOutputImageSequence> parse_output_image_sequence(
    const YAML::Node& node, const std::string& field) {
  DAILYBOY_ASSIGN_OR_RETURN(const YAML::Node map, expect_map(node, field));
  JobOutputImageSequence item;
  DAILYBOY_ASSIGN_OR_RETURN(std::string id, read_image_sequence_id(map, field));
  DAILYBOY_ASSIGN_OR_RETURN(const bool enabled,
                            read_image_sequence_enabled(map, field));
  DAILYBOY_ASSIGN_OR_RETURN(
      JobOutputDisplayView display_view,
      parse_display_view(map["display_view"], field + ".display_view"));
  DAILYBOY_ASSIGN_OR_RETURN(std::string path_pattern,
                            read_image_sequence_path_pattern(map, field));
  DAILYBOY_ASSIGN_OR_RETURN(
      std::optional<JobOutputImageSequenceFormatOptions> format_options,
      parse_image_sequence_format_options(map["format_options"], path_pattern,
                                          field + ".format_options"));
  item.set_id(std::move(id));
  item.set_enabled(enabled);
  item.set_display_view(std::move(display_view));
  item.set_path_pattern(std::move(path_pattern));
  item.set_format_options(std::move(format_options));
  return item;
}

StatusOr<JobOutputVideo> parse_output_video(const YAML::Node& node,
                                            const std::string& field) {
  DAILYBOY_ASSIGN_OR_RETURN(const YAML::Node map, expect_map(node, field));
  JobOutputVideo video;
  DAILYBOY_ASSIGN_OR_RETURN(std::string id, read_video_id(map, field));
  DAILYBOY_ASSIGN_OR_RETURN(const bool enabled, read_video_enabled(map, field));
  DAILYBOY_ASSIGN_OR_RETURN(
      JobOutputDisplayView display_view,
      parse_display_view(map["display_view"], field + ".display_view"));
  if (!map["path"] || map["path"].IsNull()) {
    return Status::User(with_job_error(USER_ERROR_JOB_165, field + ".path."));
  }
  DAILYBOY_ASSIGN_OR_RETURN(std::string path,
                            read_video_path(map["path"], field));
  DAILYBOY_ASSIGN_OR_RETURN(const int fps, read_video_fps(map, field));
  DAILYBOY_ASSIGN_OR_RETURN(const std::string codec,
                            read_video_codec(map, field));
  DAILYBOY_ASSIGN_OR_RETURN(
      JobOutputVideoSignal signal,
      parse_video_signal(map["signal"], field + ".signal", codec == "dnxhd"));
  video.set_id(std::move(id));
  video.set_enabled(enabled);
  video.set_display_view(std::move(display_view));
  video.set_path(std::move(path));
  video.set_fps(fps);
  video.set_signal(signal);
  DAILYBOY_RETURN_IF_ERROR(set_video_codec_options(video, codec, map, field));
  return video;
}

StatusOr<JobOutput> parse_output(const YAML::Node& node) {
  DAILYBOY_ASSIGN_OR_RETURN(const YAML::Node map, expect_map(node, "output"));
  const bool has_videos_key = map["videos"] && !map["videos"].IsNull();
  const bool has_sequences_key =
      map["image_sequences"] && !map["image_sequences"].IsNull();
  if (!has_videos_key && !has_sequences_key) {
    return Status::User(with_job_error(USER_ERROR_JOB_39));
  }
  JobOutput out;

  {
    JobOutputVideos videos;
    std::vector<JobOutputVideo> values;
    DAILYBOY_ASSIGN_OR_RETURN(
        const YAML::Node seq,
        optional_sequence(map["videos"], "output.videos"));
    values.reserve(seq.size());
    for (std::size_t i = 0; i < seq.size(); ++i) {
      const std::string base = "output.videos[" + std::to_string(i) + "]";
      DAILYBOY_ASSIGN_OR_RETURN(JobOutputVideo video,
                                parse_output_video(seq[i], base));
      values.push_back(std::move(video));
    }
    videos.set_videos(std::move(values));
    out.set_videos(std::move(videos));
  }

  {
    JobOutputImageSequences image_sequences;
    std::vector<JobOutputImageSequence> values;
    DAILYBOY_ASSIGN_OR_RETURN(
        const YAML::Node seq,
        optional_sequence(map["image_sequences"], "output.image_sequences"));
    values.reserve(seq.size());
    for (std::size_t i = 0; i < seq.size(); ++i) {
      const std::string base =
          "output.image_sequences[" + std::to_string(i) + "]";
      DAILYBOY_ASSIGN_OR_RETURN(JobOutputImageSequence item,
                                parse_output_image_sequence(seq[i], base));
      values.push_back(std::move(item));
    }
    image_sequences.set_image_sequences(std::move(values));
    out.set_image_sequences(std::move(image_sequences));
  }

  std::map<std::string, std::string> seen_ids;
  auto add_id = [&](const std::string& id, const std::string& field) -> Status {
    const auto it = seen_ids.find(id);
    if (it != seen_ids.end()) {
      return Status::User(with_job_error(
          USER_ERROR_JOB_35,
          field + " '" + id + "' (already used by " + it->second + ")."));
    }
    seen_ids.emplace(id, field);
    return Status::Ok();
  };
  const auto& parsed_videos = out.videos().videos();
  for (std::size_t i = 0; i < parsed_videos.size(); ++i) {
    DAILYBOY_RETURN_IF_ERROR(add_id(
        parsed_videos[i].id(), "output.videos[" + std::to_string(i) + "].id"));
  }
  const auto& parsed_sequences = out.image_sequences().image_sequences();
  for (std::size_t i = 0; i < parsed_sequences.size(); ++i) {
    DAILYBOY_RETURN_IF_ERROR(
        add_id(parsed_sequences[i].id(),
               "output.image_sequences[" + std::to_string(i) + "].id"));
  }

  bool any_enabled = false;
  for (const JobOutputVideo& video : parsed_videos) {
    if (video.enabled()) {
      any_enabled = true;
      break;
    }
  }
  if (!any_enabled) {
    for (const JobOutputImageSequence& item : parsed_sequences) {
      if (item.enabled()) {
        any_enabled = true;
        break;
      }
    }
  }
  if (!any_enabled) {
    return Status::User(with_job_error(USER_ERROR_JOB_36));
  }

  return out;
}

}  // namespace dailyboy
