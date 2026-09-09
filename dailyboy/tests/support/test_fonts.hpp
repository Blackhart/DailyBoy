#pragma once

/*!
 * \file test_fonts.hpp
 * \brief Vendored DejaVu paths for unit / perf tests (see tests/data/fonts/).
 */

#ifndef DAILYBOY_TEST_FONTS_DIR
#error "DAILYBOY_TEST_FONTS_DIR must be set by CMake"
#endif

namespace dailyboy {
namespace test {

inline constexpr const char* kDejaVuSans =
    DAILYBOY_TEST_FONTS_DIR "/DejaVuSans.ttf";
inline constexpr const char* kDejaVuSansBold =
    DAILYBOY_TEST_FONTS_DIR "/DejaVuSans-Bold.ttf";
inline constexpr const char* kDejaVuSansMono =
    DAILYBOY_TEST_FONTS_DIR "/DejaVuSansMono.ttf";

/*!
 * \brief Repo-relative font paths stored in job YAML fixtures (OS-portable).
 */
inline constexpr const char* kDejaVuSansRel =
    "dailyboy/tests/data/fonts/DejaVuSans.ttf";
inline constexpr const char* kDejaVuSansBoldRel =
    "dailyboy/tests/data/fonts/DejaVuSans-Bold.ttf";
inline constexpr const char* kDejaVuSansMonoRel =
    "dailyboy/tests/data/fonts/DejaVuSansMono.ttf";

}  // namespace test
}  // namespace dailyboy
