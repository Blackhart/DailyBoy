# DailyBoy

[![Build](https://github.com/Blackhart/DailyBoy/actions/workflows/ci.yml/badge.svg)](https://github.com/Blackhart/DailyBoy/actions/workflows/ci.yml)
[![Coverage](https://codecov.io/gh/Blackhart/DailyBoy/branch/develop/graph/badge.svg)](https://codecov.io/gh/Blackhart/DailyBoy)

**DailyBoy** is an open-source tool to generate VFX dailies.
It convert your image sequences into reviewable videos or image sequences with OCIO color management, customizable slates, and burn-ins. All driven by a single YAML job files.

DailyBoy follows the **[VFX Reference Platform](https://vfxplatform.com/)**:
- **Default:** **CY2026** (C++20, Python 3.13, OpenColorIO 2.5, OpenImageIO 3.0, FFmpeg 7.1)
- **Optional:** **CY2025** (C++17, Python 3.11, OCIO 2.4, TBB 2021, OIIO 3.0)
- **Optional:** **CY2024** (C++17, Python 3.11, OCIO 2.3, TBB 2020, OIIO 3.0)

## Table of contents

- [Usage](#usage)
- [CMake structure, options, and dependencies](#cmake-structure-options-and-dependencies)
- [Build, install, and test](#build-install-and-test-with-cmake)
- [Continuous integration](#continuous-integration)
- [License](#license)

## Usage

### Job YAML

The **job YAML** is a YAML configuration file that defines a dailies render. It describes everything needed to process an image sequence into one or more review-ready deliverables: framing and layout, input and output paths, color management (using OpenColorIO), and overlay details like slates and burn-ins. 

See the [reference documentation](docs/reference/reference-job-yaml.md) for a full explanation of the fields, allowed values, and their effect on each deliverable.

An example job file is provided at [`examples/job.mvp.example.yaml`](examples/job.mvp.example.yaml).

### CLI

DailyBoy provides both a **Python CLI** and a **C++ CLI**:

#### C++

From an install prefix, load the bundled libraries, then run the C++ CLI (default):

```bash
source /path/to/install/share/dailyboy/env.sh
makeDaily /path/to/job.yaml
```

#### Python

Python CLI (requires `DAILYBOY_BUILD_PYTHON=ON` at build time):

```bash
source /path/to/install/share/dailyboy/env.sh
python -m dailyboy.cli /path/to/job.yaml
```

Alternatively, you can also install DailyBoy via `pip install dailyboy` (or `pip install -e api/python` in a dev tree).

### API (embed in your tools)

You can also integrate DailyBoy directly into your existing pipeline using either of the provided APIs (Python and C++).

#### C++

```cpp
#include <dailyboy/api/makeDaily.hpp>
#include <dailyboy/api/log.hpp>

int main() {
  dailyboy::api::init_logging("my_tool");
  return dailyboy::api::makeDaily("path/to/job.yaml");
}
```

#### Python

```python
from dailyboy import init_logging, makeDaily

init_logging("my_tool")
exit_code = makeDaily("path/to/job.yaml")
```

## CMake structure, options, and dependencies

### Core CMake options

- `DAILYBOY_VFX_PLATFORM` (default: `2026`) — VFX year: `2024`, `2025`, or `2026`.
- `BUILD_TESTING` (default: `ON`) — CTest and test targets.
- `DAILYBOY_BUILD_PYTHON` (default: `ON`) — pybind11 extension and Python package pieces.
- `DAILYBOY_PYTHON_BUILD_IN_SOURCE` (default: `ON`) — build native module in source tree (`OFF` if the tree is read-only).
- `DAILYBOY_EXPORT_COMPILE_COMMANDS` (default: `ON`) — generate `compile_commands.json`.
- `DAILYBOY_COMPILE_COMMANDS_SYMLINK` (default: `ON`) — root symlink to build `compile_commands.json`.
- `DAILYBOY_ENABLE_COVERAGE` (default: `OFF`, requires `BUILD_TESTING`) — instrument the `dailyboy` library with gcov. After `ctest -L unit`, run `cmake --build --preset coverage-report` for HTML / LCOV / Cobertura under `build/CY2026/debug/coverage/` (CI: `ci/coverage_upload.sh`).
- `DAILYBOY_GCOVR_HTML_THEME` (default: `green`) — gcovr HTML theme.
- `DAILYBOY_ENABLE_SANITIZERS` (default: `OFF`) — ASan/UBSan on DailyBoy targets (`sanitize` preset).

Versions and git tags are defined in `cmake/versions/`. All third-party deps are built from source.

### Third-party dependencies

#### Core C++ libraries (core and API)
- [cxxopts](https://github.com/jarro2783/cxxopts) — C++ command-line argument parsing
- [spdlog](https://github.com/gabime/spdlog) — fast logging
- [yaml-cpp](https://github.com/jbeder/yaml-cpp) — YAML parsing
- [fileseq](https://github.com/shotgunsoftware/fileseq) — file sequence handling (image sequences)
- [nlohmann/json](https://github.com/nlohmann/json) — modern JSON handling
- [json-schema-validator](https://github.com/pboettch/json-schema-validator) — JSON Schema validation
- [pybind11](https://github.com/pybind/pybind11) — Python/C++ binding (enabled if `DAILYBOY_BUILD_PYTHON=ON`)

#### Imaging and video
- [zlib](https://zlib.net/) — compression
- [oneTBB](https://github.com/oneapi-src/oneTBB) — high-performance threading (Intel TBB)
- [OpenColorIO](https://opencolorio.org/) — VFX color management
- [libjpeg-turbo](https://libjpeg-turbo.org/) — fast JPEG
- [libpng](http://www.libpng.org/) — PNG
- [libtiff](https://gitlab.com/libtiff/libtiff) — TIFF
- [LibRaw](https://www.libraw.org/) — photographic RAW files
- [libde265](https://github.com/strukturag/libde265) — H.265/HEVC decoder
- [x264](https://www.videolan.org/developers/x264.html) — H.264 encoder
- [x265](https://bitbucket.org/multicoreware/x265_git) — H.265 encoder
- [libaom](https://aomedia.googlesource.com/aom/) — AV1 encoder/decoder
- [libheif](https://github.com/strukturag/libheif) — HEIF/HEIC
- [FFmpeg](https://ffmpeg.org/) — video reading and writing (**MJPEG, DNxHD, H.264, H.265, MOV, MP4**)
- [OpenImageIO](https://openimageio.org/) — advanced image/video I/O (**EXR, TIFF, OCIO, JPEG, PNG, RAW, FFmpeg, TBB, HEIF, etc.**)

#### Notes
- FFmpeg is built with: **MJPEG, DNxHD, H.264, H.265, MOV, MP4** support.
- OpenImageIO is built with: **EXR, TIFF, OCIO, JPEG, PNG, RAW, FFmpeg, TBB, HEIF**, and more.

## Build, install, and test with CMake

Requires **CMake 3.28** and Ninja.

### Configure and build

To configure and build the project with CMake, follow these steps:

1. **Configure the build directory with CMake presets.** The project provides presets for CY2026, CY2025, and CY2024. Use the debug or release preset as needed:

   ```bash
   # CY2026
   cmake --preset debug
   cmake --preset release

   # CY2025
   cmake --preset cy2025-debug
   cmake --preset cy2025-release

   # CY2024
   cmake --preset cy2024-debug
   cmake --preset cy2024-release
   ```

2. **Build the project** with Ninja (invoked automatically via CMake):

   ```bash
   # CY2026
   cmake --build --preset debug
   cmake --build --preset release

   # CY2025
   cmake --build --preset cy2025-debug
   cmake --build --preset cy2025-release

   # CY2024
   cmake --build --preset cy2024-debug
   cmake --build --preset cy2024-release
   ```

   The build artifacts will appear in `build/CY2026/debug/`, `build/CY2026/release/`, or their CY2025/CY2024 equivalents, depending on your preset.

**Notes:**
- By default, the build includes both C++ and Python components, and all dependencies are built from source.
- You can customize build options (for example, enabling/disabling Python bindings, tests, sanitizers, or code coverage) using CMake cache options or via the environment. See the list of variables above for configuration options.

See [`CMakePresets.json`](CMakePresets.json) and the `cmake/` directory for full documentation on available presets and customizations.

### Install

```bash
# CY2026
cmake --install build/CY2026/debug
cmake --install build/CY2026/release

# CY2025
cmake --install build/CY2025/debug
cmake --install build/CY2025/release

# CY2024
cmake --install build/CY2024/debug
cmake --install build/CY2024/release
```

### Run tests

tests + coverage:

```bash
cmake --preset debug && cmake --build --preset debug
export LD_LIBRARY_PATH="$(find build/CY2026/debug/_deps -type d -name lib 2>/dev/null | paste -sd:):build/CY2026/debug/dailyboy:build/CY2026/debug/api/cpp${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
ctest --preset tests
cmake --build --preset coverage-report
```

That writes `build/CY2026/debug/coverage/index.html` (and LCOV/Cobertura).

ASan/UBSan:

```bash
cmake --preset sanitize && cmake --build --preset sanitize
export LD_LIBRARY_PATH="$(find build/CY2026/sanitize/_deps -type d -name lib 2>/dev/null | paste -sd:):build/CY2026/sanitize/dailyboy:build/CY2026/sanitize/api/cpp${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
ctest --preset sanitize
```

Perf and load:

```bash
cmake --preset release && cmake --build --preset release
export LD_LIBRARY_PATH="$(find build/CY2026/release/_deps -type d -name lib 2>/dev/null | paste -sd:):build/CY2026/release/dailyboy:build/CY2026/release/api/cpp${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
ctest --preset perf
ctest --preset load
```

CY2025 unit tests:

```bash
cmake --preset cy2025-debug && cmake --build --preset cy2025-debug
export LD_LIBRARY_PATH="$(find build/CY2025/debug/_deps -type d -name lib 2>/dev/null | paste -sd:):build/CY2025/debug/dailyboy:build/CY2025/debug/api/cpp${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
ctest --preset cy2025-tests
```

CY2024 unit tests:

```bash
cmake --preset cy2024-debug && cmake --build --preset cy2024-debug
export LD_LIBRARY_PATH="$(find build/CY2024/debug/_deps -type d -name lib 2>/dev/null | paste -sd:):build/CY2024/debug/dailyboy:build/CY2024/debug/api/cpp${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
ctest --preset cy2024-tests
```

## Continuous integration

The CI system is built around a **base Ubuntu 24.04 image** ([`docker/Dockerfile.base`](docker/Dockerfile.base)) and a **base Rocky Linux 9 image** ([`docker/rocky/Dockerfile.rocky9.base`](docker/rocky/Dockerfile.rocky9.base)), and there is a separate Docker image for each VFX platform year ([`Dockerfile.cy2026`](docker/Dockerfile.cy2026), [`Dockerfile.cy2025`](docker/Dockerfile.cy2025), and [`Dockerfile.cy2024`](docker/Dockerfile.cy2024) for Ubuntu; [`Dockerfile.rocky9.cy2026`](docker/rocky/Dockerfile.rocky9.cy2026), [`Dockerfile.rocky9.cy2025`](docker/rocky/Dockerfile.rocky9.cy2025), and [`Dockerfile.rocky9.cy2024`](docker/rocky/Dockerfile.rocky9.cy2024) for Rocky). The workflow definitions are in [`.github/workflows/`](.github/workflows/), and supporting shell scripts are found in [`ci/`](ci/).

Here is how CI is organized:

| Workflow | What triggers it? | What does it run? |
| --- | --- | --- |
| [`ci.yml`](.github/workflows/ci.yml) | On PRs or pushes to `develop` or `main` | **Ubuntu:** format + CY2024/25/26 debug + tests (coverage on CY2026). **Rocky 9:** CY2024/25/26 debug + tests (no sanitize/tidy/coverage) |
| [`nightly.yml`](.github/workflows/nightly.yml) | On scheduled cron or manual trigger | **Ubuntu CY2026 only** on branch `develop`: debug (+ Codecov) ∥ sanitize ∥ **clang-tidy** |

**Code coverage** is collected only for **Ubuntu CY2026** builds. In CY2024/CY2025 debug builds, coverage is disabled (`DAILYBOY_ENABLE_COVERAGE=OFF`).

You can manually build and run CI jobs in Docker with the following commands:

```bash
# Ubuntu 24.04
docker build -t dailyboy-ci:ubuntu24-base -f docker/ubuntu/Dockerfile.ubuntu24.base docker/ubuntu
docker build -t dailyboy-ci:ubuntu24-cy2026 -f docker/ubuntu/Dockerfile.ubuntu24.cy2026 docker/ubuntu
docker build -t dailyboy-ci:ubuntu24-cy2025 -f docker/ubuntu/Dockerfile.ubuntu24.cy2025 docker/ubuntu
docker build -t dailyboy-ci:ubuntu24-cy2024 -f docker/ubuntu/Dockerfile.ubuntu24.cy2024 docker/ubuntu

DAILYBOY_CI_IMAGE=dailyboy-ci:ubuntu24-cy2026 ./ci/docker.sh ./ci/format.sh
DAILYBOY_CI_IMAGE=dailyboy-ci:ubuntu24-cy2026 ./ci/docker.sh ./ci/build.sh 2026 debug
DAILYBOY_CI_IMAGE=dailyboy-ci:ubuntu24-cy2025 ./ci/docker.sh ./ci/build.sh 2025 debug
DAILYBOY_CI_IMAGE=dailyboy-ci:ubuntu24-cy2024 ./ci/docker.sh ./ci/build.sh 2024 debug
DAILYBOY_CI_IMAGE=dailyboy-ci:ubuntu24-cy2026 ./ci/docker.sh ./ci/test.sh 2026 tests
DAILYBOY_CI_IMAGE=dailyboy-ci:ubuntu24-cy2025 ./ci/docker.sh ./ci/test.sh 2025 tests
DAILYBOY_CI_IMAGE=dailyboy-ci:ubuntu24-cy2024 ./ci/docker.sh ./ci/test.sh 2024 tests
DAILYBOY_CI_IMAGE=dailyboy-ci:ubuntu24-cy2026 ./ci/docker.sh ./ci/tidy.sh

# Rocky Linux 9
docker build -t dailyboy-ci:rocky9-base -f docker/rocky/Dockerfile.rocky9.base docker/rocky
docker build -t dailyboy-ci:rocky9-cy2026 -f docker/rocky/Dockerfile.rocky9.cy2026 docker/rocky
docker build -t dailyboy-ci:rocky9-cy2025 -f docker/rocky/Dockerfile.rocky9.cy2025 docker/rocky
docker build -t dailyboy-ci:rocky9-cy2024 -f docker/rocky/Dockerfile.rocky9.cy2024 docker/rocky

# Rocky uses DAILYBOY_BUILD_ROOT=docker/rocky9/ (Ubuntu default: docker/ubuntu24/)
DAILYBOY_BUILD_ROOT=docker/rocky9/ DAILYBOY_CI_IMAGE=dailyboy-ci:rocky9-cy2026 ./ci/docker.sh ./ci/build.sh 2026 debug
DAILYBOY_BUILD_ROOT=docker/rocky9/ DAILYBOY_CI_IMAGE=dailyboy-ci:rocky9-cy2025 ./ci/docker.sh ./ci/build.sh 2025 debug
DAILYBOY_BUILD_ROOT=docker/rocky9/ DAILYBOY_CI_IMAGE=dailyboy-ci:rocky9-cy2024 ./ci/docker.sh ./ci/build.sh 2024 debug
DAILYBOY_BUILD_ROOT=docker/rocky9/ DAILYBOY_CI_IMAGE=dailyboy-ci:rocky9-cy2026 ./ci/docker.sh ./ci/test.sh 2026 tests
DAILYBOY_BUILD_ROOT=docker/rocky9/ DAILYBOY_CI_IMAGE=dailyboy-ci:rocky9-cy2025 ./ci/docker.sh ./ci/test.sh 2025 tests
DAILYBOY_BUILD_ROOT=docker/rocky9/ DAILYBOY_CI_IMAGE=dailyboy-ci:rocky9-cy2024 ./ci/docker.sh ./ci/test.sh 2024 tests
```

**Build caching:** GitHub Actions caches **ccache** and deps under `build/docker/ubuntu24/CY*/…/_deps` or `build/docker/rocky9/CY*/…/_deps`.
 

If you want to run tests with `ctest` directly (not using the helper scripts), remember to set `LD_LIBRARY_PATH` as shown above, so the build can find its dependencies.

## License

DailyBoy source code is licensed under the [Apache License 2.0](LICENSE).

Bundled video stacks built with this project use **FFmpeg** configured with `--enable-gpl` plus **x264** and **x265** (GPL). Redistributing binaries that include those libraries may therefore require GPL compliance for that stack (source offer, notices), even though DailyBoy itself remains Apache-2.0. Other third-party dependencies keep their own licenses.
