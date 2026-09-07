#pragma once

#include <dailyboy/log.hpp>
#include <string>
#include <string_view>

namespace dailyboy {

/*!
 * \brief Debug banner that visually separates a log section.
 */
inline void log_debug_banner(std::string_view title) {
  log_debug("======== " + std::string(title) + " ========");
}

}  // namespace dailyboy
