cmake_minimum_required(VERSION 3.22.1)
project(moh_android LANGUAGES C CXX)

# Build the portable host primitives used by the Xbox kernel/runtime.
# This is an initial Android integration milestone: it deliberately does not
# pull in SDL, desktop OpenGL, or the generated game code yet.
set(XBOXRECOMP_PLATFORM_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../source/xboxrecomp-0.16.0/src/platform")

add_subdirectory(
    "${XBOXRECOMP_PLATFORM_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}/xboxrecomp-platform")

add_library(moh_android SHARED native_entry.cpp)
target_compile_features(moh_android PRIVATE cxx_std_17)
target_compile_options(moh_android PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(moh_android PRIVATE platform)

target_compile_definitions(moh_android PRIVATE MOH_ANDROID_PLATFORM_LINKED=1)
