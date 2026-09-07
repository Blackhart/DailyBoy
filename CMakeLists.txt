cmake_minimum_required(VERSION 3.28)

project(
    DailyBoy
    VERSION 1.0.0
    DESCRIPTION "VFX dailies generator"
    LANGUAGES CXX
)

include(cmake/DailyBoy.cmake)
add_subdirectory(dailyboy)
add_subdirectory(api)
add_subdirectory(examples)
