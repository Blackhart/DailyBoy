#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <dailyboy/log.hpp>
#include <dailyboy/makeDaily.hpp>

namespace py = pybind11;

PYBIND11_MODULE(_dailyboy_native, m) {
  m.doc() = "DailyBoy — Python bindings (C++ engine)";

  py::enum_<dailyboy::LogLevel>(m, "LogLevel")
      .value("Debug", dailyboy::LogLevel::Debug)
      .value("Info", dailyboy::LogLevel::Info)
      .value("Warn", dailyboy::LogLevel::Warn)
      .value("Error", dailyboy::LogLevel::Error);

  m.def("init_logging", &dailyboy::init_logging,
        py::arg("logger_name") = "dailyboy",
        "Create the default stderr logger.");
  m.def("set_log_level", &dailyboy::set_log_level, py::arg("level"),
        "Set the minimum log level on the default logger.");
  m.def(
      "makeDaily",
      [](const std::string& job_yaml_path) {
        return dailyboy::makeDaily(job_yaml_path);
      },
      py::arg("job_yaml_path"),
      "Run rendering for a YAML job file. Returns 0 on success.");
}
