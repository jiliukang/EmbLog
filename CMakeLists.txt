cmake_minimum_required(VERSION 3.14)

project(
        EmbLog
        VERSION 0.1.0
        DESCRIPTION "Character input/output library for embedded systems."
        HOMEPAGE_URL "https://github.com/jiliukang/emblog"
        LANGUAGES CXX
)

OPTION(THREADX_ADAPTER  "Enable ThreadX adapter."  ON )

add_library(EmbLog INTERFACE)
add_library(EmbLog::EmbLog ALIAS EmbLog)

set_property(
        TARGET EmbLog PROPERTY
        EXPORT_NAME EmbLog
)

target_include_directories(
        EmbLog
        INTERFACE
        "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
)
if (THREADX_ADAPTER)
target_include_directories(
        EmbLog
        INTERFACE
        "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/adapter/include>"
)

endif ()

OPTION(THREADX_ADAPTER  "Enable ThreadX adapter."  ON )

target_compile_features(EmbLog INTERFACE cxx_std_20)


