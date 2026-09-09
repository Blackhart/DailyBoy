# api/python

**`dailyboy`** package: API (`makeDaily`) and CLI (`makeDaily` console script) via **pybind11**.

**CMake prerequisite:** `-DDAILYBOY_BUILD_PYTHON=ON` (otherwise no `_dailyboy_native` — no Python API or CLI).

```bash
cmake -S ../.. -B ../../build -DDAILYBOY_BUILD_PYTHON=ON
cmake --build ../../build
```

```python
from dailyboy import makeDaily

rc = makeDaily("examples/job.mvp.example.cy2026.yaml")
```

### CLI `makeDaily`

```bash
# from repository root, .venv activated
source .venv/bin/activate
pip install -r requirements-dev.txt
cmake -S . -B build -DDAILYBOY_BUILD_PYTHON=ON && cmake --build build
pip install -e api/python

makeDaily examples/job.mvp.example.cy2026.yaml
makeDaily -h
```

Without `pip install -e`: `PYTHONPATH=api/python python -m dailyboy.cli …` (after CMake build).

The `_dailyboy_native*.so` module is generated in `dailyboy/` next to `__init__.py`. Use `PYTHONPATH=api/python` or `pip install -e api/python` after build.

**Python:** 3.13.x ([VFX CY2026](../../docs/specs/vfx-platform.md)). Build prerequisite: Python headers (`python3-dev`).
