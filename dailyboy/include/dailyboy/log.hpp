#pragma once

#include <cstdint>
#include <dailyboy/export.hpp>
#include <string>
#include <string_view>

namespace dailyboy {

/*!
 * \brief Text log levels for the default stderr logger.
 */
enum class LogLevel : std::uint8_t { Debug, Info, Warn, Error };

/*!
 * \brief Creates the default stderr color logger.
 * \param logger_name spdlog logger name (default \c "dailyboy").
 * \note Idempotent: a second call is a no-op.
 */
DAILYBOY_API void init_logging(const std::string& logger_name = "dailyboy");

/*!
 * \brief Sets the minimum level emitted on the default logger.
 */
DAILYBOY_API void set_log_level(LogLevel level);

DAILYBOY_API void log_debug(std::string_view message);
DAILYBOY_API void log_info(std::string_view message);
DAILYBOY_API void log_warn(std::string_view message);
DAILYBOY_API void log_error(std::string_view message);

}  // namespace dailyboy
