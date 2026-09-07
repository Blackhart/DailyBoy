#!/usr/bin/env python3
"""Build a 48 kHz stereo WAV that speaks each plate frame number (espeak)."""

from __future__ import annotations

import os
import struct
import subprocess
import sys
import tempfile
import wave


def samples_per_frame(fps: int) -> int:
    return 48000 // fps


def speak_frame_to_wav(frame: int, path: str) -> None:
    subprocess.run(["espeak", "-s", "250", "-w", path, str(frame)], check=True)


def read_mono_pcm(path: str) -> tuple[list[int], int]:
    with wave.open(path, "rb") as handle:
        raw = handle.readframes(handle.getnframes())
        rate = handle.getframerate()
        channels = handle.getnchannels()
    samples = list(struct.unpack("<" + "h" * (len(raw) // 2), raw))
    if channels == 2:
        samples = samples[0::2]
    return samples, rate


def resample_linear(samples: list[int], src_rate: int, dst_rate: int) -> list[int]:
    if src_rate == dst_rate or not samples:
        return samples
    out_len = max(1, int(round(len(samples) * dst_rate / src_rate)))
    return [sample_at(samples, i, out_len) for i in range(out_len)]


def sample_at(samples: list[int], index: int, out_len: int) -> int:
    src = index * (len(samples) - 1) / max(1, out_len - 1)
    left = int(src)
    frac = src - left
    a = samples[left]
    b = samples[min(left + 1, len(samples) - 1)]
    return int(a + (b - a) * frac)


def pad_or_trim_mono(samples: list[int], want: int) -> list[int]:
    if len(samples) >= want:
        return samples[:want]
    return samples + [0] * (want - len(samples))


def stereo_interleave(mono: list[int]) -> list[int]:
    return [sample for sample in mono for _ in (0, 1)]


def write_stereo_wav(path: str, interleaved: list[int], rate: int = 48000) -> None:
    with wave.open(path, "wb") as handle:
        handle.setnchannels(2)
        handle.setsampwidth(2)
        handle.setframerate(rate)
        handle.writeframes(struct.pack("<" + "h" * len(interleaved), *interleaved))


def append_spoken_frame(pcm: list[int], frame: int, tmp: str, want: int) -> None:
    raw = os.path.join(tmp, f"{frame}.wav")
    speak_frame_to_wav(frame, raw)
    mono, rate = read_mono_pcm(raw)
    mono = pad_or_trim_mono(resample_linear(mono, rate, 48000), want)
    pcm.extend(stereo_interleave(mono))


def build_guide_wav(out_path: str, start: int, end: int, fps: int) -> None:
    want = samples_per_frame(fps)
    pcm: list[int] = []
    with tempfile.TemporaryDirectory() as tmp:
        for frame in range(start, end + 1):
            append_spoken_frame(pcm, frame, tmp, want)
    write_stereo_wav(out_path, pcm)


def main() -> None:
    out_path = sys.argv[1]
    start = int(sys.argv[2])
    end = int(sys.argv[3])
    fps = int(sys.argv[4])
    build_guide_wav(out_path, start, end, fps)
    print(out_path)


if __name__ == "__main__":
    main()
