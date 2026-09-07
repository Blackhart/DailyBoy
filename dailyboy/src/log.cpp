#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <dailyboy/log.hpp>
#include <memory>
#include <mutex>
#include <string>

namespace dailyboy {
namespace {

/*!
 * \brief Process-wide default logger; created once under \c g_log_mutex.
 */
std::shared_ptr<spdlog::logger> g_logger;

/*!
 * \brief Guards \c g_logger creation and the idempotent init_logging() path.
 */
std::mutex g_log_mutex;

spdlog::level::level_enum to_spdlog_level(LogLevel level) {
  switch (level) {
    case LogLevel::Debug:
      return spdlog::level::debug;
    case LogLevel::Info:
      return spdlog::level::info;
    case LogLevel::Warn:
      return spdlog::level::warn;
    case LogLevel::Error:
      return spdlog::level::err;
  }
  return spdlog::level::info;
}

/*!
 * \brief Creates \c g_logger. Caller must hold \c g_log_mutex.
 */
void init_logging_locked(const std::string& logger_name) {
  if (g_logger) {
    return;
  }

  auto sink = std::make_shared<spdlog::sinks::stderr_color_sink_mt>();
  g_logger = std::make_shared<spdlog::logger>(logger_name, std::move(sink));
  g_logger->set_level(spdlog::level::info);
  g_logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] %v");
  spdlog::register_logger(g_logger);
}

/*!
 * \brief Returns the default logger, calling init_logging() on first use.
 */
spdlog::logger& ensure_logger() {
  std::lock_guard lock(g_log_mutex);
  if (!g_logger) {
    init_logging_locked("dailyboy");
  }
  return *g_logger;
}

}  // namespace

void init_logging(const std::string& logger_name) {
  std::lock_guard lock(g_log_mutex);
  init_logging_locked(logger_name);
}

void set_log_level(LogLevel level) {
  ensure_logger().set_level(to_spdlog_level(level));
}

void log_debug(std::string_view message) { ensure_logger().debug(message); }

void log_info(std::string_view message) { ensure_logger().info(message); }

void log_warn(std::string_view message) { ensure_logger().warn(message); }

void log_error(std::string_view message) { ensure_logger().error(message); }

}  // namespace dailyboy
