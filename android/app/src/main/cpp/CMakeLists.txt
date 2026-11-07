cmake_minimum_required(VERSION 3.22.1)
project(moh_android LANGUAGES C CXX)

# Build the portable host primitives used by the Xbox kernel/runtime.
set(XBOXRECOMP_PLATFORM_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../source/xboxrecomp-0.16.0/src/platform")

add_subdirectory(
    "${XBOXRECOMP_PLATFORM_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}/xboxrecomp-platform")

# Compile the Xbox kernel/HLE sources against Android host primitives.
get_filename_component(XBOXRECOMP_KERNEL_DIR "${XBOXRECOMP_PLATFORM_DIR}/../kernel" ABSOLUTE)
add_subdirectory(
    "${XBOXRECOMP_KERNEL_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}/xboxrecomp-kernel")

# Android uses the GLES3 branch of the shared D3D8 compatibility backend.
# The EGL surface is owned by MainActivity and borrowed by D3D8 on its render thread.
get_filename_component(XBOXRECOMP_D3D_DIR "${XBOXRECOMP_PLATFORM_DIR}/../d3d" ABSOLUTE)
add_subdirectory(
    "${XBOXRECOMP_D3D_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}/xboxrecomp-d3d")

add_library(moh_android SHARED native_entry.cpp)
target_compile_features(moh_android PRIVATE cxx_std_17)
target_compile_options(moh_android PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(moh_android PRIVATE
    platform
    xbox_kernel
    xbox_d3d8
    android
    EGL
    GLESv3
    log
)
target_compile_definitions(moh_android PRIVATE MOH_ANDROID_PLATFORM_LINKED=1)

# Build the portable SSE compatibility check as an independent ARM64 target.
include("${CMAKE_CURRENT_SOURCE_DIR}/recomp_sse_compat_test.cmake")
