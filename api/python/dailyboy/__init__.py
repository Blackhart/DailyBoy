"""DailyBoy Python API — pybind11 extension to the C++ engine (when DAILYBOY_BUILD_PYTHON=ON)."""

try:
    from dailyboy._dailyboy_native import LogLevel, init_logging, makeDaily, set_log_level
except ImportError as exc:
    raise ImportError(
        "Native module '_dailyboy_native' not found. "
        "The Python API and CLI require a CMake build with "
        "-DDAILYBOY_BUILD_PYTHON=ON, then: cmake --build build"
    ) from exc

__all__ = ["LogLevel", "init_logging", "makeDaily", "set_log_level"]
