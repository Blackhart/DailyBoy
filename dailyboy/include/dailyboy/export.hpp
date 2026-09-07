#pragma once

#if defined(_WIN32)
#if defined(DAILYBOY_BUILD_SHARED)
#define DAILYBOY_API __declspec(dllexport)
#else
#define DAILYBOY_API __declspec(dllimport)
#endif
#else
#define DAILYBOY_API __attribute__((visibility("default")))
#endif

/*!
 * \def DAILYBOY_API
 * \brief Symbol visibility for the shared DailyBoy library.
 */
