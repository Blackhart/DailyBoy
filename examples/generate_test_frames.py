#!/usr/bin/env python3
"""Generate orange test plates with a large centered frame number.

Writes ``examples/frames/<WxH>/plate.####.png`` for each resolution below
(DNxHD CIDs, DNxHR minimum, and a 2K DNxHR size).

    python3 examples/generate_test_frames.py
    python3 examples/generate_test_frames.py 1920x1080
"""

from __future__ import annotations

import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

FRAMES_ROOT = Path(__file__).resolve().parent / "frames"
PATTERN = "plate.{frame:04d}.png"
FRAME_START = 1001
FRAME_END = 1048
ORANGE = (255, 122, 0)
WHITE = (255, 255, 255)
BLACK = (20, 20, 20)
FONT_PATH = "dailyboy/tests/data/fonts/DejaVuSans-Bold.ttf"

# (width, height) — HD CIDs, DNxHR min, 2K.
RESOLUTIONS: tuple[tuple[int, int], ...] = (
    (256, 120),
    (960, 720),
    (1280, 720),
    (1440, 1080),
    (1920, 1080),
    (2048, 1080),
)


def parse_resolution(text: str) -> tuple[int, int]:
    parts = text.lower().split("x")
    if len(parts) != 2:
        raise ValueError(f"expected WxH, got {text!r}")
    return int(parts[0]), int(parts[1])


def font_for_height(height: int) -> ImageFont.FreeTypeFont:
    size = max(24, int(round(280 * height / 720)))
    return ImageFont.truetype(FONT_PATH, size)


def outline_width(height: int) -> int:
    return max(2, int(round(6 * height / 720)))


def write_sequence(width: int, height: int) -> None:
    directory = FRAMES_ROOT / f"{width}x{height}"
    directory.mkdir(parents=True, exist_ok=True)
    font = font_for_height(height)
    outline = outline_width(height)
    step = 2 if outline >= 4 else 1
    for frame in range(FRAME_START, FRAME_END + 1):
        image = Image.new("RGB", (width, height), ORANGE)
        draw = ImageDraw.Draw(image)
        text = str(frame)
        left, top, right, bottom = draw.textbbox((0, 0), text, font=font)
        x = (width - (right - left)) // 2 - left
        y = (height - (bottom - top)) // 2 - top
        for dx in range(-outline, outline + 1, step):
            for dy in range(-outline, outline + 1, step):
                if dx == 0 and dy == 0:
                    continue
                draw.text((x + dx, y + dy), text, font=font, fill=BLACK)
        draw.text((x, y), text, font=font, fill=WHITE)
        path = directory / PATTERN.format(frame=frame)
        image.save(path, optimize=True)
        print(path)


def main(argv: list[str]) -> None:
    if argv:
        resolutions = [parse_resolution(arg) for arg in argv]
    else:
        resolutions = list(RESOLUTIONS)
    for width, height in resolutions:
        write_sequence(width, height)


if __name__ == "__main__":
    main(sys.argv[1:])
