/*!
 * \file parse_output.cpp
 * \brief Parse job YAML output videos and image sequences.
 */

#include "job/parse_output.hpp"

#include <optional>

#include "job/parse_format_options.hpp"
#include "job/yaml_read.hpp"

namespace dailyboy {

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

  DAILYBOY_ASSIGN_OR_RETURN(
      const std::string preset,
      as_optional<std::string>(map, "preset", "medium", field));
  if (preset == "ultrafast") {
    h264.set_preset(
        JobOutputVideoH264::JobOutputVideoH264PresetValue::Ultrafast);
  } else if (preset == "superfast") {
    h264.set_preset(
        JobOutputVideoH264::JobOutputVideoH264PresetValue::Superfast);
  } else if (preset == "veryfast") {
    h264.set_preset(
        JobOutputVideoH264::JobOutputVideoH264PresetValue::Veryfast);
  } else if (preset == "faster") {
    h264.set_preset(JobOutputVideoH264::JobOutputVideoH264PresetValue::Faster);
  } else if (preset == "fast") {
    h264.set_preset(JobOutputVideoH264::JobOutputVideoH264PresetValue::Fast);
  } else if (preset == "medium") {
    h264.set_preset(JobOutputVideoH264::JobOutputVideoH264PresetValue::Medium);
  } else if (preset == "slow") {
    h264.set_preset(JobOutputVideoH264::JobOutputVideoH264PresetValue::Slow);
  } else {
    return Status::User(
        with_job_error(USER_ERROR_JOB_25, field + ".preset '" + preset + "'."));
  }

  DAILYBOY_ASSIGN_OR_RETURN(
      int crf,
      as_optional<int>(map, "crf", JobOutputVideoH264::kDefaultCrf, field));
  DAILYBOY_ASSIGN_OR_RETURN(int gop, as_optional<int>(map, "gop", 0, field));
  DAILYBOY_ASSIGN_OR_RETURN(int bitrate_kbps,
                            as_optional<int>(map, "bitrate_kbps", 0, field));
  DAILYBOY_ASSIGN_OR_RETURN(
      const std::string pix_fmt,
      as_optional<std::string>(map, "pix_fmt", "yuv420p", field));
  h264.set_crf(crf);
  h264.set_gop(gop);
  h264.set_bitrate_kbps(bitrate_kbps);
  if (pix_fmt == "yuv420p") {
    h264.set_pix_fmt(
        JobOutputVideoH264::JobOutputVideoH264PixFmtValue::Yuv420p);
  } else if (pix_fmt == "yuv422p") {
    h264.set_pix_fmt(
        JobOutputVideoH264::JobOutputVideoH264PixFmtValue::Yuv422p);
  } else {
    return Status::User(with_job_error(USER_ERROR_JOB_25,
                                       field + ".pix_fmt '" + pix_fmt + "'."));
  }

  DAILYBOY_ASSIGN_OR_RETURN(const std::string tune,
                            as_optional<std::string>(map, "tune", "", field));
  if (tune.empty()) {
    h264.set_tune(JobOutputVideoH264::JobOutputVideoH264TuneValue::Unspecified);
  } else if (tune == "film") {
    h264.set_tune(JobOutputVideoH264::JobOutputVideoH264TuneValue::Film);
  } else if (tune == "animation") {
    h264.set_tune(JobOutputVideoH264::JobOutputVideoH264TuneValue::Animation);
  } else if (tune == "grain") {
    h264.set_tune(JobOutputVideoH264::JobOutputVideoH264TuneValue::Grain);
  } else if (tune == "stillimage") {
    h264.set_tune(JobOutputVideoH264::JobOutputVideoH264TuneValue::Stillimage);
  } else if (tune == "fastdecode") {
    h264.set_tune(JobOutputVideoH264::JobOutputVideoH264TuneValue::Fastdecode);
  } else if (tune == "zerolatency") {
    h264.set_tune(JobOutputVideoH264::JobOutputVideoH264TuneValue::Zerolatency);
  } else {
    return Status::User(
        with_job_error(USER_ERROR_JOB_25, field + ".tune '" + tune + "'."));
  }

  DAILYBOY_ASSIGN_OR_RETURN(
      const std::string profile,
      as_optional<std::string>(map, "profile", "", field));
  if (profile.empty()) {
    h264.set_profile(
        JobOutputVideoH264::JobOutputVideoH264ProfileValue::Unspecified);
  } else if (profile == "baseline") {
    h264.set_profile(
        JobOutputVideoH264::JobOutputVideoH264ProfileValue::Baseline);
  } else if (profile == "main") {
    h264.set_profile(JobOutputVideoH264::JobOutputVideoH264ProfileValue::Main);
  } else if (profile == "high") {
    h264.set_profile(JobOutputVideoH264::JobOutputVideoH264ProfileValue::High);
  } else if (profile == "high422") {
    h264.set_profile(
        JobOutputVideoH264::JobOutputVideoH264ProfileValue::High422);
  } else {
    return Status::User(with_job_error(USER_ERROR_JOB_25,
                                       field + ".profile '" + profile + "'."));
  }

  const bool is_422 =
      h264.pix_fmt() ==
      JobOutputVideoH264::JobOutputVideoH264PixFmtValue::Yuv422p;
  if (h264.profile() ==
          JobOutputVideoH264::JobOutputVideoH264ProfileValue::High422 &&
      !is_422) {
    return Status::User(with_job_error(USER_ERROR_JOB_26, field + "."));
  }
  if (is_422 &&
      (h264.profile() ==
           JobOutputVideoH264::JobOutputVideoH264ProfileValue::Baseline ||
       h264.profile() ==
           JobOutputVideoH264::JobOutputVideoH264ProfileValue::Main ||
       h264.profile() ==
           JobOutputVideoH264::JobOutputVideoH264ProfileValue::High)) {
    return Status::User(with_job_error(USER_ERROR_JOB_27, field + "."));
  }

  std::string level;
  if (map["level"]) {
    const YAML::Node level_node = map["level"];
    if (!level_node.IsScalar()) {
      return Status::User(with_job_error(USER_ERROR_JOB_28, field + "."));
    }
    level = canonicalize_h264_level(level_node.Scalar());
    if (level.empty()) {
      return Status::User(with_job_error(
          USER_ERROR_JOB_25, field + ".level '" + level_node.Scalar() + "'."));
    }
  }
  h264.set_level(std::move(level));

  DAILYBOY_ASSIGN_OR_RETURN(bool faststart,
                            as_optional<bool>(map, "faststart", true, field));
  h264.set_faststart(faststart);

  return h264;
}

StatusOr<JobOutputVideoMjpeg> parse_mjpeg(const YAML::Node& node,
                                          const std::string& field) {
  JobOutputVideoMjpeg mjpeg;
  if (!node) {
    return mjpeg;
  }
  DAILYBOY_ASSIGN_OR_RETURN(const YAML::Node map, expect_map(node, field));

  DAILYBOY_ASSIGN_OR_RETURN(
      int qscale, as_optional<int>(map, "qscale",
                                   JobOutputVideoMjpeg::kDefaultQscale, field));
  if (qscale < 1 || qscale > 31) {
    return Status::User(with_job_error(USER_ERROR_JOB_29, field + "."));
  }
  mjpeg.set_qscale(qscale);

  DAILYBOY_ASSIGN_OR_RETURN(
      const std::string pix_fmt,
      as_optional<std::string>(map, "pix_fmt", "yuv422p", field));
  if (pix_fmt == "yuv420p") {
    mjpeg.set_pix_fmt(
        JobOutputVideoMjpeg::JobOutputVideoMjpegPixFmtValue::Yuv420p);
  } else if (pix_fmt == "yuv422p") {
    mjpeg.set_pix_fmt(
        JobOutputVideoMjpeg::JobOutputVideoMjpegPixFmtValue::Yuv422p);
  } else if (pix_fmt == "yuv444p") {
    mjpeg.set_pix_fmt(
        JobOutputVideoMjpeg::JobOutputVideoMjpegPixFmtValue::Yuv444p);
  } else {
    return Status::User(with_job_error(USER_ERROR_JOB_25,
                                       field + ".pix_fmt '" + pix_fmt + "'."));
  }

  DAILYBOY_ASSIGN_OR_RETURN(
      const std::string huffman,
      as_optional<std::string>(map, "huffman", "optimal", field));
  if (huffman == "default") {
    mjpeg.set_huffman(
        JobOutputVideoMjpeg::JobOutputVideoMjpegHuffmanValue::Default);
  } else if (huffman == "optimal") {
    mjpeg.set_huffman(
        JobOutputVideoMjpeg::JobOutputVideoMjpegHuffmanValue::Optimal);
  } else {
    return Status::User(with_job_error(USER_ERROR_JOB_25,
                                       field + ".huffman '" + huffman + "'."));
  }

  DAILYBOY_ASSIGN_OR_RETURN(bool faststart,
                            as_optional<bool>(map, "faststart", true, field));
  mjpeg.set_faststart(faststart);

  return mjpeg;
}

bool dnxhd_is_hr(JobOutputVideoDnxhd::JobOutputVideoDnxhdProfileValue profile) {
  return profile != JobOutputVideoDnxhd::JobOutputVideoDnxhdProfileValue::Dnxhd;
}

JobOutputVideoDnxhd::JobOutputVideoDnxhdPixFmtValue dnxhd_default_pix_fmt(
    JobOutputVideoDnxhd::JobOutputVideoDnxhdProfileValue profile) {
  using Profile = JobOutputVideoDnxhd::JobOutputVideoDnxhdProfileValue;
  using Pix = JobOutputVideoDnxhd::JobOutputVideoDnxhdPixFmtValue;
  switch (profile) {
    case Profile::DnxhrHqx:
      return Pix::Yuv422p10;
    case Profile::Dnxhr444:
      return Pix::Yuv444p10;
    case Profile::Dnxhd:
    case Profile::DnxhrLb:
    case Profile::DnxhrSq:
    case Profile::DnxhrHq:
      return Pix::Yuv422p;
  }
  return Pix::Yuv422p;
}

bool dnxhd_pix_fmt_ok(
    JobOutputVideoDnxhd::JobOutputVideoDnxhdProfileValue profile,
    JobOutputVideoDnxhd::JobOutputVideoDnxhdPixFmtValue pix_fmt) {
  using Profile = JobOutputVideoDnxhd::JobOutputVideoDnxhdProfileValue;
  using Pix = JobOutputVideoDnxhd::JobOutputVideoDnxhdPixFmtValue;
  switch (profile) {
    case Profile::Dnxhd:
      return pix_fmt == Pix::Yuv422p || pix_fmt == Pix::Yuv422p10;
    case Profile::DnxhrLb:
    case Profile::DnxhrSq:
    case Profile::DnxhrHq:
      return pix_fmt == Pix::Yuv422p;
    case Profile::DnxhrHqx:
      return pix_fmt == Pix::Yuv422p10;
    case Profile::Dnxhr444:
      return pix_fmt == Pix::Yuv444p10;
  }
  return false;
}

StatusOr<JobOutputVideoDnxhd> parse_dnxhd(const YAML::Node& node,
                                          const std::string& field) {
  JobOutputVideoDnxhd dnxhd;
  if (!node) {
    return dnxhd;
  }
  DAILYBOY_ASSIGN_OR_RETURN(const YAML::Node map, expect_map(node, field));

  DAILYBOY_ASSIGN_OR_RETURN(
      const std::string profile,
      as_optional<std::string>(map, "profile", "dnxhr_hq", field));
  if (profile == "dnxhd") {
    dnxhd.set_profile(
        JobOutputVideoDnxhd::JobOutputVideoDnxhdProfileValue::Dnxhd);
  } else if (profile == "dnxhr_lb") {
    dnxhd.set_profile(
        JobOutputVideoDnxhd::JobOutputVideoDnxhdProfileValue::DnxhrLb);
  } else if (profile == "dnxhr_sq") {
    dnxhd.set_profile(
        JobOutputVideoDnxhd::JobOutputVideoDnxhdProfileValue::DnxhrSq);
  } else if (profile == "dnxhr_hq") {
    dnxhd.set_profile(
        JobOutputVideoDnxhd::JobOutputVideoDnxhdProfileValue::DnxhrHq);
  } else if (profile == "dnxhr_hqx") {
    dnxhd.set_profile(
        JobOutputVideoDnxhd::JobOutputVideoDnxhdProfileValue::DnxhrHqx);
  } else if (profile == "dnxhr_444") {
    dnxhd.set_profile(
        JobOutputVideoDnxhd::JobOutputVideoDnxhdProfileValue::Dnxhr444);
  } else {
    return Status::User(with_job_error(USER_ERROR_JOB_25,
                                       field + ".profile '" + profile + "'."));
  }

  const bool hr = dnxhd_is_hr(dnxhd.profile());
  if (hr && map["bitrate_kbps"]) {
    return Status::User(with_job_error(USER_ERROR_JOB_30, field + "."));
  }
  if (!hr && !map["bitrate_kbps"]) {
    return Status::User(with_job_error(USER_ERROR_JOB_31, field + "."));
  }
  if (!hr) {
    DAILYBOY_ASSIGN_OR_RETURN(int bitrate_kbps,
                              as_required<int>(map, "bitrate_kbps", field));
    if (bitrate_kbps < 1) {
      return Status::User(with_job_error(USER_ERROR_JOB_32, field + "."));
    }
    dnxhd.set_bitrate_kbps(bitrate_kbps);
  }

  if (map["pix_fmt"]) {
    DAILYBOY_ASSIGN_OR_RETURN(const std::string pix_fmt,
                              as_required<std::string>(map, "pix_fmt", field));
    if (pix_fmt == "yuv422p") {
      dnxhd.set_pix_fmt(
          JobOutputVideoDnxhd::JobOutputVideoDnxhdPixFmtValue::Yuv422p);
    } else if (pix_fmt == "yuv422p10") {
      dnxhd.set_pix_fmt(
          JobOutputVideoDnxhd::JobOutputVideoDnxhdPixFmtValue::Yuv422p10);
    } else if (pix_fmt == "yuv444p10") {
      dnxhd.set_pix_fmt(
          JobOutputVideoDnxhd::JobOutputVideoDnxhdPixFmtValue::Yuv444p10);
    } else {
      return Status::User(with_job_error(
          USER_ERROR_JOB_25, field + ".pix_fmt '" + pix_fmt + "'."));
    }
    if (!dnxhd_pix_fmt_ok(dnxhd.profile(), dnxhd.pix_fmt())) {
      return Status::User(with_job_error(
          USER_ERROR_JOB_33,
          field + " pix_fmt '" + pix_fmt + "' profile '" + profile + "'."));
    }
  } else {
    dnxhd.set_pix_fmt(dnxhd_default_pix_fmt(dnxhd.profile()));
  }

  DAILYBOY_ASSIGN_OR_RETURN(bool interlaced,
                            as_optional<bool>(map, "interlaced", false, field));
  if (hr && interlaced) {
    return Status::User(with_job_error(USER_ERROR_JOB_34, field + "."));
  }
  dnxhd.set_interlaced(interlaced);

  DAILYBOY_ASSIGN_OR_RETURN(
      bool nitris_compat,
      as_optional<bool>(map, "nitris_compat", false, field));
  dnxhd.set_nitris_compat(nitris_compat);

  DAILYBOY_ASSIGN_OR_RETURN(bool faststart,
                            as_optional<bool>(map, "faststart", true, field));
  dnxhd.set_faststart(faststart);

  return dnxhd;
}

bool prores_is_444(JobOutputVideoProres::JobOutputVideoProresProfileValue p) {
  using Profile = JobOutputVideoProres::JobOutputVideoProresProfileValue;
  return p == Profile::FourFourFourFour || p == Profile::FourFourFourFourXq;
}

JobOutputVideoProres::JobOutputVideoProresPixFmtValue prores_default_pix_fmt(
    JobOutputVideoProres::JobOutputVideoProresProfileValue profile) {
  using Pix = JobOutputVideoProres::JobOutputVideoProresPixFmtValue;
  return prores_is_444(profile) ? Pix::Yuv444p10 : Pix::Yuv422p10;
}

bool prores_pix_fmt_ok(
    JobOutputVideoProres::JobOutputVideoProresProfileValue profile,
    JobOutputVideoProres::JobOutputVideoProresPixFmtValue pix_fmt) {
  using Pix = JobOutputVideoProres::JobOutputVideoProresPixFmtValue;
  if (prores_is_444(profile)) {
    return pix_fmt == Pix::Yuv444p10 || pix_fmt == Pix::Yuva444p10;
  }
  return pix_fmt == Pix::Yuv422p10;
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

StatusOr<JobOutputVideoProres> parse_prores(const YAML::Node& node,
                                            const std::string& field) {
  JobOutputVideoProres prores;
  if (!node) {
    return prores;
  }
  DAILYBOY_ASSIGN_OR_RETURN(const YAML::Node map, expect_map(node, field));

  std::string profile = "hq";
  if (map["profile"]) {
    const YAML::Node profile_node = map["profile"];
    if (!profile_node.IsScalar()) {
      return Status::User(
          with_job_error(USER_ERROR_JOB_25, field + ".profile."));
    }
    profile = profile_node.Scalar();
  }
  if (profile == "proxy") {
    prores.set_profile(
        JobOutputVideoProres::JobOutputVideoProresProfileValue::Proxy);
  } else if (profile == "lt") {
    prores.set_profile(
        JobOutputVideoProres::JobOutputVideoProresProfileValue::Lt);
  } else if (profile == "standard") {
    prores.set_profile(
        JobOutputVideoProres::JobOutputVideoProresProfileValue::Standard);
  } else if (profile == "hq") {
    prores.set_profile(
        JobOutputVideoProres::JobOutputVideoProresProfileValue::Hq);
  } else if (profile == "4444") {
    prores.set_profile(JobOutputVideoProres::JobOutputVideoProresProfileValue::
                           FourFourFourFour);
  } else if (profile == "4444xq") {
    prores.set_profile(JobOutputVideoProres::JobOutputVideoProresProfileValue::
                           FourFourFourFourXq);
  } else {
    return Status::User(with_job_error(USER_ERROR_JOB_25,
                                       field + ".profile '" + profile + "'."));
  }

  if (map["pix_fmt"]) {
    DAILYBOY_ASSIGN_OR_RETURN(const std::string pix_fmt,
                              as_required<std::string>(map, "pix_fmt", field));
    if (pix_fmt == "yuv422p10") {
      prores.set_pix_fmt(
          JobOutputVideoProres::JobOutputVideoProresPixFmtValue::Yuv422p10);
    } else if (pix_fmt == "yuv444p10") {
      prores.set_pix_fmt(
          JobOutputVideoProres::JobOutputVideoProresPixFmtValue::Yuv444p10);
    } else if (pix_fmt == "yuva444p10") {
      prores.set_pix_fmt(
          JobOutputVideoProres::JobOutputVideoProresPixFmtValue::Yuva444p10);
    } else {
      return Status::User(with_job_error(
          USER_ERROR_JOB_25, field + ".pix_fmt '" + pix_fmt + "'."));
    }
    if (!prores_pix_fmt_ok(prores.profile(), prores.pix_fmt())) {
      return Status::User(with_job_error(
          USER_ERROR_JOB_95,
          field + " pix_fmt '" + pix_fmt + "' profile '" + profile + "'."));
    }
  } else {
    prores.set_pix_fmt(prores_default_pix_fmt(prores.profile()));
  }

  DAILYBOY_ASSIGN_OR_RETURN(
      const std::string quant_mat,
      as_optional<std::string>(map, "quant_mat", "auto", field));
  if (quant_mat == "auto") {
    prores.set_quant_mat(
        JobOutputVideoProres::JobOutputVideoProresQuantMatValue::Auto);
  } else if (quant_mat == "proxy") {
    prores.set_quant_mat(
        JobOutputVideoProres::JobOutputVideoProresQuantMatValue::Proxy);
  } else if (quant_mat == "lt") {
    prores.set_quant_mat(
        JobOutputVideoProres::JobOutputVideoProresQuantMatValue::Lt);
  } else if (quant_mat == "standard") {
    prores.set_quant_mat(
        JobOutputVideoProres::JobOutputVideoProresQuantMatValue::Standard);
  } else if (quant_mat == "hq") {
    prores.set_quant_mat(
        JobOutputVideoProres::JobOutputVideoProresQuantMatValue::Hq);
  } else if (quant_mat == "default") {
    prores.set_quant_mat(
        JobOutputVideoProres::JobOutputVideoProresQuantMatValue::Default);
  } else {
    return Status::User(with_job_error(
        USER_ERROR_JOB_25, field + ".quant_mat '" + quant_mat + "'."));
  }

  DAILYBOY_ASSIGN_OR_RETURN(int bits_per_mb,
                            as_optional<int>(map, "bits_per_mb", 0, field));
  if (bits_per_mb < 0 || bits_per_mb > 8192) {
    return Status::User(with_job_error(USER_ERROR_JOB_98, field + "."));
  }
  prores.set_bits_per_mb(bits_per_mb);

  DAILYBOY_ASSIGN_OR_RETURN(
      int mbs_per_slice,
      as_optional<int>(map, "mbs_per_slice",
                       JobOutputVideoProres::kDefaultMbsPerSlice, field));
  if (mbs_per_slice < 1 || mbs_per_slice > 8) {
    return Status::User(with_job_error(USER_ERROR_JOB_99, field + "."));
  }
  prores.set_mbs_per_slice(mbs_per_slice);

  DAILYBOY_ASSIGN_OR_RETURN(
      std::string vendor,
      as_optional<std::string>(map, "vendor",
                               JobOutputVideoProres::kDefaultVendor, field));
  if (!prores_vendor_ok(vendor)) {
    return Status::User(with_job_error(USER_ERROR_JOB_96, field + "."));
  }
  prores.set_vendor(std::move(vendor));

  DAILYBOY_ASSIGN_OR_RETURN(int alpha_bits,
                            as_optional<int>(map, "alpha_bits", 0, field));
  if (alpha_bits != 0 && alpha_bits != 8 && alpha_bits != 16) {
    return Status::User(with_job_error(
        USER_ERROR_JOB_25,
        field + ".alpha_bits '" + std::to_string(alpha_bits) + "'."));
  }
  using Pix = JobOutputVideoProres::JobOutputVideoProresPixFmtValue;
  const bool alpha_allowed =
      prores_is_444(prores.profile()) && prores.pix_fmt() == Pix::Yuva444p10;
  if (alpha_bits != 0 && !alpha_allowed) {
    return Status::User(with_job_error(USER_ERROR_JOB_97, field + "."));
  }
  if (prores.pix_fmt() == Pix::Yuva444p10 && alpha_bits == 0) {
    return Status::User(with_job_error(USER_ERROR_JOB_97, field + "."));
  }
  prores.set_alpha_bits(alpha_bits);

  DAILYBOY_ASSIGN_OR_RETURN(bool faststart,
                            as_optional<bool>(map, "faststart", true, field));
  prores.set_faststart(faststart);

  return prores;
}

StatusOr<JobOutputDisplayView> parse_display_view(const YAML::Node& node,
                                                  const std::string& field) {
  DAILYBOY_ASSIGN_OR_RETURN(const YAML::Node map, expect_map(node, field));
  JobOutputDisplayView display_view;
  if (!map["display"] || !map["view"]) {
    return Status::User(with_job_error(USER_ERROR_JOB_50, field + "."));
  }
  DAILYBOY_ASSIGN_OR_RETURN(
      std::string display,
      read_quoted_nonempty_string(map["display"], USER_ERROR_JOB_50,
                                  field + ".display"));
  DAILYBOY_ASSIGN_OR_RETURN(
      std::string view, read_quoted_nonempty_string(
                            map["view"], USER_ERROR_JOB_50, field + ".view"));
  display_view.set_display(std::move(display));
  display_view.set_view(std::move(view));
  return display_view;
}

StatusOr<JobOutputVideoSignal> parse_video_signal(const YAML::Node& node,
                                                  const std::string& field,
                                                  bool dnxhd_codec) {
  if (!node) {
    return Status::User(with_job_error(USER_ERROR_JOB_92, field + "."));
  }
  DAILYBOY_ASSIGN_OR_RETURN(const YAML::Node map, expect_map(node, field));
  JobOutputVideoSignal signal;

  DAILYBOY_ASSIGN_OR_RETURN(const std::string range,
                            as_required<std::string>(map, "range", field));
  if (range == "tv") {
    signal.set_range(JobOutputVideoSignal::RangeValue::Tv);
  } else if (range == "pc") {
    if (dnxhd_codec) {
      return Status::User(
          with_job_error(USER_ERROR_JOB_25, field + ".range '" + range + "'."));
    }
    signal.set_range(JobOutputVideoSignal::RangeValue::Pc);
  } else {
    return Status::User(
        with_job_error(USER_ERROR_JOB_25, field + ".range '" + range + "'."));
  }

  DAILYBOY_ASSIGN_OR_RETURN(const std::string matrix,
                            as_required<std::string>(map, "matrix", field));
  if (matrix == "bt709") {
    signal.set_matrix(JobOutputVideoSignal::MatrixValue::Bt709);
  } else {
    return Status::User(
        with_job_error(USER_ERROR_JOB_25, field + ".matrix '" + matrix + "'."));
  }

  DAILYBOY_ASSIGN_OR_RETURN(const std::string primaries,
                            as_required<std::string>(map, "primaries", field));
  if (primaries == "bt709") {
    signal.set_primaries(JobOutputVideoSignal::PrimariesValue::Bt709);
  } else {
    return Status::User(with_job_error(
        USER_ERROR_JOB_25, field + ".primaries '" + primaries + "'."));
  }

  DAILYBOY_ASSIGN_OR_RETURN(const std::string transfer,
                            as_required<std::string>(map, "transfer", field));
  if (transfer == "bt709") {
    signal.set_transfer(JobOutputVideoSignal::TransferValue::Bt709);
  } else {
    return Status::User(with_job_error(
        USER_ERROR_JOB_25, field + ".transfer '" + transfer + "'."));
  }

  return signal;
}

StatusOr<JobOutput> parse_output(const YAML::Node& node) {
  DAILYBOY_ASSIGN_OR_RETURN(const YAML::Node map, expect_map(node, "output"));
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
      DAILYBOY_ASSIGN_OR_RETURN(const YAML::Node video_map,
                                expect_map(seq[i], base));
      JobOutputVideo video;
      DAILYBOY_ASSIGN_OR_RETURN(
          std::string id, as_required<std::string>(video_map, "id", base));
      DAILYBOY_ASSIGN_OR_RETURN(bool enabled,
                                as_required<bool>(video_map, "enabled", base));
      DAILYBOY_ASSIGN_OR_RETURN(JobOutputDisplayView display_view,
                                parse_display_view(video_map["display_view"],
                                                   base + ".display_view"));
      DAILYBOY_ASSIGN_OR_RETURN(
          std::string path, as_required<std::string>(video_map, "path", base));
      video.set_id(std::move(id));
      video.set_enabled(enabled);
      video.set_display_view(std::move(display_view));
      video.set_path(std::move(path));
      DAILYBOY_ASSIGN_OR_RETURN(int fps,
                                as_optional<int>(video_map, "fps", 24, base));
      video.set_fps(fps);
      DAILYBOY_ASSIGN_OR_RETURN(
          const std::string codec,
          as_required<std::string>(video_map, "codec", base));
      const bool dnxhd_codec = codec == "dnxhd";
      DAILYBOY_ASSIGN_OR_RETURN(
          JobOutputVideoSignal signal,
          parse_video_signal(video_map["signal"], base + ".signal",
                             dnxhd_codec));
      video.set_signal(signal);
      if (codec == "h264") {
        video.set_codec(JobOutputVideo::JobOutputVideoCodecValue::H264);
        DAILYBOY_ASSIGN_OR_RETURN(
            JobOutputVideoH264 options,
            parse_h264(video_map["codec_options"], base + ".codec_options"));
        video.set_codec_options(options);
      } else if (codec == "mjpeg") {
        video.set_codec(JobOutputVideo::JobOutputVideoCodecValue::Mjpeg);
        DAILYBOY_ASSIGN_OR_RETURN(
            JobOutputVideoMjpeg options,
            parse_mjpeg(video_map["codec_options"], base + ".codec_options"));
        video.set_codec_options(options);
      } else if (codec == "dnxhd") {
        video.set_codec(JobOutputVideo::JobOutputVideoCodecValue::Dnxhd);
        DAILYBOY_ASSIGN_OR_RETURN(
            JobOutputVideoDnxhd options,
            parse_dnxhd(video_map["codec_options"], base + ".codec_options"));
        video.set_codec_options(options);
      } else if (codec == "prores") {
        video.set_codec(JobOutputVideo::JobOutputVideoCodecValue::Prores);
        DAILYBOY_ASSIGN_OR_RETURN(
            JobOutputVideoProres options,
            parse_prores(video_map["codec_options"], base + ".codec_options"));
        video.set_codec_options(options);
      } else {
        std::string detail = base;
        detail += ".codec '";
        detail += codec;
        detail += "'.";
        return Status::User(with_job_error(USER_ERROR_JOB_25, detail));
      }
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
      DAILYBOY_ASSIGN_OR_RETURN(const YAML::Node item_map,
                                expect_map(seq[i], base));
      JobOutputImageSequence item;
      DAILYBOY_ASSIGN_OR_RETURN(std::string id,
                                as_required<std::string>(item_map, "id", base));
      DAILYBOY_ASSIGN_OR_RETURN(bool enabled,
                                as_required<bool>(item_map, "enabled", base));
      DAILYBOY_ASSIGN_OR_RETURN(
          JobOutputDisplayView display_view,
          parse_display_view(item_map["display_view"], base + ".display_view"));
      DAILYBOY_ASSIGN_OR_RETURN(
          std::string path_pattern,
          as_required<std::string>(item_map, "path_pattern", base));
      DAILYBOY_ASSIGN_OR_RETURN(
          std::optional<JobOutputImageSequenceFormatOptions> format_options,
          parse_image_sequence_format_options(item_map["format_options"],
                                              path_pattern,
                                              base + ".format_options"));
      item.set_id(std::move(id));
      item.set_enabled(enabled);
      item.set_display_view(std::move(display_view));
      item.set_path_pattern(std::move(path_pattern));
      item.set_format_options(std::move(format_options));
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
