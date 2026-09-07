#pragma once

#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace dailyboy {

/*!
 * \brief OCIO display + view pair for a review deliverable.
 */
class JobOutputDisplayView {
 public:
  JobOutputDisplayView() = default;
  ~JobOutputDisplayView() = default;

  const std::string& display() const { return display_; }
  void set_display(std::string display) { display_ = std::move(display); }

  const std::string& view() const { return view_; }
  void set_view(std::string view) { view_ = std::move(view); }

 private:
  std::string display_;
  std::string view_;
};

/*!
 * \brief Video signal tags and RGB→YCbCr matrix (\c signal on each video).
 */
class JobOutputVideoSignal {
 public:
  /*!
   * \brief Limited vs full range (maps to \c signal.range).
   */
  enum class RangeValue : int {
    Tv = 0,
    Pc = 1,
  };

  /*!
   * \brief YCbCr matrix / \c AVCOL_SPC (maps to \c signal.matrix).
   */
  enum class MatrixValue : int {
    Bt709 = 0,
  };

  /*!
   * \brief Color primaries (maps to \c signal.primaries).
   */
  enum class PrimariesValue : int {
    Bt709 = 0,
  };

  /*!
   * \brief Transfer characteristic (maps to \c signal.transfer).
   */
  enum class TransferValue : int {
    Bt709 = 0,
  };

  JobOutputVideoSignal() = default;
  ~JobOutputVideoSignal() = default;

  RangeValue range() const { return range_; }
  void set_range(RangeValue range) { range_ = range; }

  MatrixValue matrix() const { return matrix_; }
  void set_matrix(MatrixValue matrix) { matrix_ = matrix; }

  PrimariesValue primaries() const { return primaries_; }
  void set_primaries(PrimariesValue primaries) { primaries_ = primaries; }

  TransferValue transfer() const { return transfer_; }
  void set_transfer(TransferValue transfer) { transfer_ = transfer; }

 private:
  RangeValue range_ = RangeValue::Tv;
  MatrixValue matrix_ = MatrixValue::Bt709;
  PrimariesValue primaries_ = PrimariesValue::Bt709;
  TransferValue transfer_ = TransferValue::Bt709;
};

/*!
 * \brief H.264 encoding settings (\c codec_options when \c codec is \c h264).
 */
class JobOutputVideoH264 {
 public:
  /*!
   * \brief x264 preset (maps to \c codec_options.preset in the job YAML).
   */
  enum class JobOutputVideoH264PresetValue : int {
    Ultrafast = 0,
    Superfast = 1,
    Veryfast = 2,
    Faster = 3,
    Fast = 4,
    Medium = 5,
    Slow = 6,
  };

  /*!
   * \brief H.264 pixel format (maps to \c codec_options.pix_fmt).
   */
  enum class JobOutputVideoH264PixFmtValue : int {
    Yuv420p = 0,
    Yuv422p = 1,
  };

  /*!
   * \brief x264 tune (maps to \c codec_options.tune). \c Unspecified means omit
   *        the option.
   */
  enum class JobOutputVideoH264TuneValue : int {
    Unspecified = 0,
    Film = 1,
    Animation = 2,
    Grain = 3,
    Stillimage = 4,
    Fastdecode = 5,
    Zerolatency = 6,
  };

  /*!
   * \brief H.264 profile (maps to \c codec_options.profile). \c Unspecified
   *        selects \c high or \c high422 from \c pix_fmt.
   */
  enum class JobOutputVideoH264ProfileValue : int {
    Unspecified = 0,
    Baseline = 1,
    Main = 2,
    High = 3,
    High422 = 4,
  };

  static constexpr int kDefaultCrf = 23;

  JobOutputVideoH264() = default;
  ~JobOutputVideoH264() = default;

  JobOutputVideoH264PresetValue preset() const { return preset_; }
  void set_preset(JobOutputVideoH264PresetValue preset) { preset_ = preset; }

  int crf() const { return crf_; }
  void set_crf(int crf) { crf_ = crf; }

  int bitrate_kbps() const { return bitrate_kbps_; }
  void set_bitrate_kbps(int bitrate_kbps) { bitrate_kbps_ = bitrate_kbps; }

  int gop() const { return gop_; }
  void set_gop(int gop) { gop_ = gop; }

  JobOutputVideoH264PixFmtValue pix_fmt() const { return pix_fmt_; }
  void set_pix_fmt(JobOutputVideoH264PixFmtValue pix_fmt) {
    pix_fmt_ = pix_fmt;
  }

  JobOutputVideoH264TuneValue tune() const { return tune_; }
  void set_tune(JobOutputVideoH264TuneValue tune) { tune_ = tune; }

  JobOutputVideoH264ProfileValue profile() const { return profile_; }
  void set_profile(JobOutputVideoH264ProfileValue profile) {
    profile_ = profile;
  }

  const std::string& level() const { return level_; }
  void set_level(std::string level) { level_ = std::move(level); }

  bool faststart() const { return faststart_; }
  void set_faststart(bool faststart) { faststart_ = faststart; }

 private:
  JobOutputVideoH264PresetValue preset_ = JobOutputVideoH264PresetValue::Medium;
  int crf_ = kDefaultCrf;
  int bitrate_kbps_ = 0;
  int gop_ = 0;
  JobOutputVideoH264PixFmtValue pix_fmt_ =
      JobOutputVideoH264PixFmtValue::Yuv420p;
  JobOutputVideoH264TuneValue tune_ = JobOutputVideoH264TuneValue::Unspecified;
  JobOutputVideoH264ProfileValue profile_ =
      JobOutputVideoH264ProfileValue::Unspecified;
  std::string level_;
  bool faststart_ = true;
};

/*!
 * \brief MJPEG encoding settings (\c codec_options when \c codec is \c mjpeg).
 */
class JobOutputVideoMjpeg {
 public:
  /*!
   * \brief MJPEG pixel format (maps to \c codec_options.pix_fmt).
   */
  enum class JobOutputVideoMjpegPixFmtValue : int {
    Yuv420p = 0,
    Yuv422p = 1,
    Yuv444p = 2,
  };

  /*!
   * \brief Huffman table strategy (maps to \c codec_options.huffman).
   */
  enum class JobOutputVideoMjpegHuffmanValue : int {
    Default = 0,
    Optimal = 1,
  };

  static constexpr int kDefaultQscale = 3;

  JobOutputVideoMjpeg() = default;
  ~JobOutputVideoMjpeg() = default;

  int qscale() const { return qscale_; }
  void set_qscale(int qscale) { qscale_ = qscale; }

  JobOutputVideoMjpegPixFmtValue pix_fmt() const { return pix_fmt_; }
  void set_pix_fmt(JobOutputVideoMjpegPixFmtValue pix_fmt) {
    pix_fmt_ = pix_fmt;
  }

  JobOutputVideoMjpegHuffmanValue huffman() const { return huffman_; }
  void set_huffman(JobOutputVideoMjpegHuffmanValue huffman) {
    huffman_ = huffman;
  }

  bool faststart() const { return faststart_; }
  void set_faststart(bool faststart) { faststart_ = faststart; }

 private:
  int qscale_ = kDefaultQscale;
  JobOutputVideoMjpegPixFmtValue pix_fmt_ =
      JobOutputVideoMjpegPixFmtValue::Yuv422p;
  JobOutputVideoMjpegHuffmanValue huffman_ =
      JobOutputVideoMjpegHuffmanValue::Optimal;
  bool faststart_ = true;
};

/*!
 * \brief DNxHD/DNxHR encoding settings (\c codec_options when \c codec is
 *        \c dnxhd).
 */
class JobOutputVideoDnxhd {
 public:
  /*!
   * \brief FFmpeg \c dnxhd profile (maps to \c codec_options.profile).
   */
  enum class JobOutputVideoDnxhdProfileValue : int {
    Dnxhd = 0,
    DnxhrLb = 1,
    DnxhrSq = 2,
    DnxhrHq = 3,
    DnxhrHqx = 4,
    Dnxhr444 = 5,
  };

  /*!
   * \brief Encoder pixel format (maps to \c codec_options.pix_fmt).
   */
  enum class JobOutputVideoDnxhdPixFmtValue : int {
    Yuv422p = 0,
    Yuv422p10 = 1,
    Yuv444p10 = 2,
  };

  JobOutputVideoDnxhd() = default;
  ~JobOutputVideoDnxhd() = default;

  JobOutputVideoDnxhdProfileValue profile() const { return profile_; }
  void set_profile(JobOutputVideoDnxhdProfileValue profile) {
    profile_ = profile;
  }

  JobOutputVideoDnxhdPixFmtValue pix_fmt() const { return pix_fmt_; }
  void set_pix_fmt(JobOutputVideoDnxhdPixFmtValue pix_fmt) {
    pix_fmt_ = pix_fmt;
  }

  int bitrate_kbps() const { return bitrate_kbps_; }
  void set_bitrate_kbps(int bitrate_kbps) { bitrate_kbps_ = bitrate_kbps; }

  bool interlaced() const { return interlaced_; }
  void set_interlaced(bool interlaced) { interlaced_ = interlaced; }

  bool nitris_compat() const { return nitris_compat_; }
  void set_nitris_compat(bool nitris_compat) { nitris_compat_ = nitris_compat; }

  bool faststart() const { return faststart_; }
  void set_faststart(bool faststart) { faststart_ = faststart; }

 private:
  JobOutputVideoDnxhdProfileValue profile_ =
      JobOutputVideoDnxhdProfileValue::DnxhrHq;
  JobOutputVideoDnxhdPixFmtValue pix_fmt_ =
      JobOutputVideoDnxhdPixFmtValue::Yuv422p;
  int bitrate_kbps_ = 0;
  bool interlaced_ = false;
  bool nitris_compat_ = false;
  bool faststart_ = true;
};

/*!
 * \brief Encoder-specific \c codec_options (\c h264, \c mjpeg, or \c dnxhd).
 */
using JobOutputVideoCodecOptions =
    std::variant<JobOutputVideoH264, JobOutputVideoMjpeg, JobOutputVideoDnxhd>;

/*!
 * \brief One video deliverable (QuickTime container).
 */
class JobOutputVideo {
 public:
  /*!
   * \brief Encoder selected by \c codec in the job YAML.
   */
  enum class JobOutputVideoCodecValue : int {
    H264 = 0,
    Mjpeg = 1,
    Dnxhd = 2,
  };

  JobOutputVideo() = default;
  ~JobOutputVideo() = default;

  const std::string& id() const { return id_; }
  void set_id(std::string id) { id_ = std::move(id); }

  bool enabled() const { return enabled_; }
  void set_enabled(bool enabled) { enabled_ = enabled; }

  const JobOutputDisplayView& display_view() const { return display_view_; }
  JobOutputDisplayView& display_view() { return display_view_; }
  void set_display_view(JobOutputDisplayView display_view) {
    display_view_ = std::move(display_view);
  }

  const JobOutputVideoSignal& signal() const { return signal_; }
  JobOutputVideoSignal& signal() { return signal_; }
  void set_signal(JobOutputVideoSignal signal) { signal_ = std::move(signal); }

  const std::string& path() const { return path_; }
  void set_path(std::string path) { path_ = std::move(path); }

  int fps() const { return fps_; }
  void set_fps(int fps) { fps_ = fps; }

  JobOutputVideoCodecValue codec() const { return codec_; }
  void set_codec(JobOutputVideoCodecValue codec) { codec_ = codec; }

  const JobOutputVideoCodecOptions& codec_options() const {
    return codec_options_;
  }
  JobOutputVideoCodecOptions& codec_options() { return codec_options_; }
  void set_codec_options(JobOutputVideoCodecOptions codec_options) {
    codec_options_ = std::move(codec_options);
  }

 private:
  std::string id_;
  bool enabled_ = false;
  JobOutputDisplayView display_view_;
  JobOutputVideoSignal signal_;
  std::string path_;
  int fps_ = 24;
  JobOutputVideoCodecValue codec_ = JobOutputVideoCodecValue::H264;
  JobOutputVideoCodecOptions codec_options_;
};

/*!
 * \brief Video deliverables (\c output.videos in the job YAML).
 */
class JobOutputVideos {
 public:
  JobOutputVideos() = default;
  ~JobOutputVideos() = default;

  const std::vector<JobOutputVideo>& videos() const { return videos_; }
  std::vector<JobOutputVideo>& videos() { return videos_; }
  void set_videos(std::vector<JobOutputVideo> videos) {
    videos_ = std::move(videos);
  }

 private:
  std::vector<JobOutputVideo> videos_;
};

/*!
 * \brief PNG \c format_options (extension \c .png).
 */
class JobOutputImageSequencePng {
 public:
  static constexpr int kDefaultBitDepth = 8;
  static constexpr int kDefaultCompressionLevel = 6;
  static constexpr int kDefaultFilter = 0;

  JobOutputImageSequencePng() = default;
  ~JobOutputImageSequencePng() = default;

  int bit_depth() const { return bit_depth_; }
  void set_bit_depth(int bit_depth) { bit_depth_ = bit_depth; }

  const std::string& compression() const { return compression_; }
  void set_compression(std::string compression) {
    compression_ = std::move(compression);
  }

  int compression_level() const { return compression_level_; }
  void set_compression_level(int compression_level) {
    compression_level_ = compression_level;
  }

  int filter() const { return filter_; }
  void set_filter(int filter) { filter_ = filter; }

 private:
  int bit_depth_ = kDefaultBitDepth;
  std::string compression_ = "default";
  int compression_level_ = kDefaultCompressionLevel;
  int filter_ = kDefaultFilter;
};

/*!
 * \brief JPEG \c format_options (extension \c .jpg / \c .jpeg).
 */
class JobOutputImageSequenceJpeg {
 public:
  static constexpr int kDefaultBitDepth = 8;
  static constexpr int kDefaultCompressionLevel = 98;

  JobOutputImageSequenceJpeg() = default;
  ~JobOutputImageSequenceJpeg() = default;

  int bit_depth() const { return bit_depth_; }
  void set_bit_depth(int bit_depth) { bit_depth_ = bit_depth; }

  int compression_level() const { return compression_level_; }
  void set_compression_level(int compression_level) {
    compression_level_ = compression_level;
  }

  const std::string& subsampling() const { return subsampling_; }
  void set_subsampling(std::string subsampling) {
    subsampling_ = std::move(subsampling);
  }

  bool progressive() const { return progressive_; }
  void set_progressive(bool progressive) { progressive_ = progressive; }

 private:
  int bit_depth_ = kDefaultBitDepth;
  int compression_level_ = kDefaultCompressionLevel;
  std::string subsampling_;
  bool progressive_ = false;
};

/*!
 * \brief TIFF \c format_options (extension \c .tif / \c .tiff).
 */
class JobOutputImageSequenceTiff {
 public:
  static constexpr int kDefaultCompressionLevel = 6;

  JobOutputImageSequenceTiff() = default;
  ~JobOutputImageSequenceTiff() = default;

  const std::string& bit_depth() const { return bit_depth_; }
  void set_bit_depth(std::string bit_depth) {
    bit_depth_ = std::move(bit_depth);
  }

  bool bigtiff() const { return bigtiff_; }
  void set_bigtiff(bool bigtiff) { bigtiff_ = bigtiff; }

  const std::string& compression() const { return compression_; }
  void set_compression(std::string compression) {
    compression_ = std::move(compression);
  }

  int compression_level() const { return compression_level_; }
  void set_compression_level(int compression_level) {
    compression_level_ = compression_level;
  }

 private:
  std::string bit_depth_ = "8";
  bool bigtiff_ = false;
  std::string compression_ = "zip";
  int compression_level_ = kDefaultCompressionLevel;
};

/*!
 * \brief OpenEXR \c format_options (extension \c .exr).
 */
class JobOutputImageSequenceExr {
 public:
  static constexpr int kDefaultZipLevel = 4;

  JobOutputImageSequenceExr() = default;
  ~JobOutputImageSequenceExr() = default;

  const std::string& bit_depth() const { return bit_depth_; }
  void set_bit_depth(std::string bit_depth) {
    bit_depth_ = std::move(bit_depth);
  }

  const std::string& compression() const { return compression_; }
  void set_compression(std::string compression) {
    compression_ = std::move(compression);
  }

  bool has_compression_level() const { return has_compression_level_; }
  int compression_level() const { return compression_level_; }
  void set_compression_level(int compression_level) {
    compression_level_ = compression_level;
    has_compression_level_ = true;
  }

 private:
  std::string bit_depth_ = "h16";
  std::string compression_ = "zip";
  int compression_level_ = kDefaultZipLevel;
  bool has_compression_level_ = false;
};

/*!
 * \brief HEIF/AVIF \c format_options (extension \c .heif / \c .heic / \c .avif).
 */
class JobOutputImageSequenceHeif {
 public:
  static constexpr int kDefaultBitDepth = 8;
  static constexpr int kDefaultCompressionLevel = 75;

  JobOutputImageSequenceHeif() = default;
  ~JobOutputImageSequenceHeif() = default;

  int bit_depth() const { return bit_depth_; }
  void set_bit_depth(int bit_depth) { bit_depth_ = bit_depth; }

  const std::string& compression() const { return compression_; }
  void set_compression(std::string compression) {
    compression_ = std::move(compression);
  }

  int compression_level() const { return compression_level_; }
  void set_compression_level(int compression_level) {
    compression_level_ = compression_level;
  }

 private:
  int bit_depth_ = kDefaultBitDepth;
  std::string compression_ = "heic";
  int compression_level_ = kDefaultCompressionLevel;
};

/*!
 * \brief Optional per-sequence writer options (discriminated by path extension).
 */
using JobOutputImageSequenceFormatOptions =
    std::variant<JobOutputImageSequencePng, JobOutputImageSequenceJpeg,
                 JobOutputImageSequenceTiff, JobOutputImageSequenceExr,
                 JobOutputImageSequenceHeif>;

/*!
 * \brief One image sequence deliverable.
 */
class JobOutputImageSequence {
 public:
  JobOutputImageSequence() = default;
  ~JobOutputImageSequence() = default;

  const std::string& id() const { return id_; }
  void set_id(std::string id) { id_ = std::move(id); }

  bool enabled() const { return enabled_; }
  void set_enabled(bool enabled) { enabled_ = enabled; }

  const JobOutputDisplayView& display_view() const { return display_view_; }
  JobOutputDisplayView& display_view() { return display_view_; }
  void set_display_view(JobOutputDisplayView display_view) {
    display_view_ = std::move(display_view);
  }

  const std::string& path_pattern() const { return path_pattern_; }
  void set_path_pattern(std::string path_pattern) {
    path_pattern_ = std::move(path_pattern);
  }

  const std::optional<JobOutputImageSequenceFormatOptions>& format_options()
      const {
    return format_options_;
  }
  std::optional<JobOutputImageSequenceFormatOptions>& format_options() {
    return format_options_;
  }
  void set_format_options(
      std::optional<JobOutputImageSequenceFormatOptions> format_options) {
    format_options_ = std::move(format_options);
  }

 private:
  std::string id_;
  bool enabled_ = false;
  JobOutputDisplayView display_view_;
  std::string path_pattern_;
  std::optional<JobOutputImageSequenceFormatOptions> format_options_;
};

/*!
 * \brief Image sequence deliverables (\c output.image_sequences in the job YAML).
 */
class JobOutputImageSequences {
 public:
  JobOutputImageSequences() = default;
  ~JobOutputImageSequences() = default;

  const std::vector<JobOutputImageSequence>& image_sequences() const {
    return image_sequences_;
  }
  std::vector<JobOutputImageSequence>& image_sequences() {
    return image_sequences_;
  }
  void set_image_sequences(
      std::vector<JobOutputImageSequence> image_sequences) {
    image_sequences_ = std::move(image_sequences);
  }

 private:
  std::vector<JobOutputImageSequence> image_sequences_;
};

/*!
 * \brief Output deliverables section of the job (\c output in the job YAML).
 */
class JobOutput {
 public:
  JobOutput() = default;
  ~JobOutput() = default;

  const JobOutputVideos& videos() const { return videos_; }
  JobOutputVideos& videos() { return videos_; }
  void set_videos(JobOutputVideos videos) { videos_ = std::move(videos); }

  const JobOutputImageSequences& image_sequences() const {
    return image_sequences_;
  }
  JobOutputImageSequences& image_sequences() { return image_sequences_; }
  void set_image_sequences(JobOutputImageSequences image_sequences) {
    image_sequences_ = std::move(image_sequences);
  }

 private:
  JobOutputVideos videos_;
  JobOutputImageSequences image_sequences_;
};

}  // namespace dailyboy
