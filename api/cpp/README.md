# api/cpp

Public C++ API (`dailyboy_api`, shared library) linked to the [`dailyboy`](../../dailyboy/) engine.

| CMake target | Role |
| ------------ | ---- |
| `dailyboy` | Engine: `dailyboy::makeDaily(job_yaml_path)` |
| `dailyboy_api` | API: `dailyboy::api::makeDaily(job_yaml_path)` → calls the engine |

```bash
cmake -S ../.. -B ../../build
cmake --build ../../build
```

Integration: `target_link_libraries(my_app PRIVATE dailyboy_api)` (transitive link to `dailyboy`).

### CLI `makeDaily`

```bash
cmake --build build --target makeDaily
./build/bin/makeDaily examples/job.mvp.example.cy2026.yaml
./build/bin/makeDaily -h
```

Options: positional `job.yaml`, or `-j` / `--job`; `-h` / `--help`.
