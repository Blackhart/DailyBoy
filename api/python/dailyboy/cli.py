"""makeDaily CLI — render dailies from a YAML job."""

from __future__ import annotations

import argparse
import sys

from dailyboy import LogLevel, init_logging, makeDaily as make_daily_core, set_log_level


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="makeDaily",
        description="DailyBoy — generate VFX dailies from a YAML job.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="Example:\n  makeDaily examples/job.mvp.example.yaml",
    )
    parser.add_argument(
        "job_yaml",
        metavar="job.yaml",
        help="Path to the job file (.yaml / .yml)",
    )
    parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        help="DEBUG log level",
    )
    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    init_logging("makeDaily")
    if args.verbose:
        set_log_level(LogLevel.Debug)
    return int(make_daily_core(args.job_yaml))


if __name__ == "__main__":
    sys.exit(main())
