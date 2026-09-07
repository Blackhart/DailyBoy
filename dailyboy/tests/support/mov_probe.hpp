#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/pixfmt.h>
}

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

namespace dailyboy {
namespace test {

struct MovInfo {
  int packets = -1;
  int key_packets = 0;
  AVCodecID codec_id = AV_CODEC_ID_NONE;
  AVPixelFormat pix_fmt = AV_PIX_FMT_NONE;
  int width = 0;
  int height = 0;
  int profile = 0;
  int level = 0;
  AVColorRange color_range = AVCOL_RANGE_UNSPECIFIED;
  AVFieldOrder field_order = AV_FIELD_UNKNOWN;
  int64_t bit_rate = 0;
  bool has_audio = false;
  AVCodecID audio_codec_id = AV_CODEC_ID_NONE;
  int audio_sample_rate = 0;
  int audio_channels = 0;
};

inline uint32_t read_be32(std::istream& in) {
  unsigned char b[4] = {0, 0, 0, 0};
  in.read(reinterpret_cast<char*>(b), 4);
  return (static_cast<uint32_t>(b[0]) << 24) |
         (static_cast<uint32_t>(b[1]) << 16) |
         (static_cast<uint32_t>(b[2]) << 8) | static_cast<uint32_t>(b[3]);
}

inline uint64_t read_be64(std::istream& in) {
  const uint32_t hi = read_be32(in);
  const uint32_t lo = read_be32(in);
  return (static_cast<uint64_t>(hi) << 32) | static_cast<uint64_t>(lo);
}

/*!
 * \brief True when a \c moov atom appears before \c mdat (MOV +faststart).
 */
inline bool moov_before_mdat(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    return false;
  }
  while (in) {
    const std::streampos start = in.tellg();
    const uint32_t size32 = read_be32(in);
    char type[4] = {};
    in.read(type, 4);
    if (!in) {
      break;
    }
    const std::string tag(type, 4);
    if (tag == "moov") {
      return true;
    }
    if (tag == "mdat") {
      return false;
    }
    uint64_t size = size32;
    std::streamoff header = 8;
    if (size32 == 1) {
      size = read_be64(in);
      header = 16;
    }
    if (size < static_cast<uint64_t>(header)) {
      break;
    }
    in.seekg(start + static_cast<std::streamoff>(size));
  }
  return false;
}

inline MovInfo probe_mov(const std::filesystem::path& path) {
  MovInfo info;
  AVFormatContext* format = nullptr;
  if (avformat_open_input(&format, path.c_str(), nullptr, nullptr) < 0) {
    return info;
  }
  if (avformat_find_stream_info(format, nullptr) < 0) {
    avformat_close_input(&format);
    return info;
  }
  int video =
      av_find_best_stream(format, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
  if (video < 0) {
    avformat_close_input(&format);
    return info;
  }
  const AVCodecParameters* par = format->streams[video]->codecpar;
  info.codec_id = par->codec_id;
  info.pix_fmt = static_cast<AVPixelFormat>(par->format);
  info.width = par->width;
  info.height = par->height;
  info.profile = par->profile;
  info.level = par->level;
  info.color_range = par->color_range;
  info.field_order = par->field_order;
  info.bit_rate = par->bit_rate;
  info.packets = 0;
  const int audio =
      av_find_best_stream(format, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
  if (audio >= 0) {
    const AVCodecParameters* apar = format->streams[audio]->codecpar;
    info.has_audio = true;
    info.audio_codec_id = apar->codec_id;
    info.audio_sample_rate = apar->sample_rate;
    info.audio_channels = apar->ch_layout.nb_channels;
  }
  AVPacket* pkt = av_packet_alloc();
  while (av_read_frame(format, pkt) >= 0) {
    if (pkt->stream_index == video) {
      ++info.packets;
      if ((pkt->flags & AV_PKT_FLAG_KEY) != 0) {
        ++info.key_packets;
      }
    }
    av_packet_unref(pkt);
  }
  av_packet_free(&pkt);
  avformat_close_input(&format);
  return info;
}

inline bool file_contains(const std::filesystem::path& path,
                          const std::string& needle) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    return false;
  }
  const std::string data((std::istreambuf_iterator<char>(in)),
                         std::istreambuf_iterator<char>());
  return data.find(needle) != std::string::npos;
}

inline std::filesystem::path unique_mov(const char* name) {
  const std::filesystem::path dir =
      std::filesystem::path(DAILYBOY_TEST_BINARY_DIR) / name;
  std::filesystem::remove_all(dir);
  std::filesystem::create_directories(dir);
  return dir / "out.mov";
}

}  // namespace test
}  // namespace dailyboy
