#pragma once

#include <dailyboy/log.hpp>
#include <string>

namespace dailyboy::api {

using LogLevel = dailyboy::LogLevel;

/*!
 * \brief Creates the default stderr logger for API clients.
 * \param logger_name spdlog logger name (default \c "dailyboy_api").
 */
inline void init_logging(const std::string& logger_name = "dailyboy_api") {
  dailyboy::init_logging(logger_name);
}

inline void set_log_level(LogLevel level) { dailyboy::set_log_level(level); }

inline void log_debug(std::string_view message) {
  dailyboy::log_debug(message);
}

inline void log_info(std::string_view message) { dailyboy::log_info(message); }

inline void log_warn(std::string_view message) { dailyboy::log_warn(message); }

inline void log_error(std::string_view message) {
  dailyboy::log_error(message);
}

}  // namespace dailyboy::api
