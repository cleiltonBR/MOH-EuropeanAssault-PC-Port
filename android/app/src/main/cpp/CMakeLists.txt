cmake_minimum_required(VERSION 3.22.1)
project(moh_android LANGUAGES C CXX)

set(XBOXRECOMP_PLATFORM_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../source/xboxrecomp-0.16.0/src/platform")

add_subdirectory(
    "${XBOXRECOMP_PLATFORM_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}/xboxrecomp-platform")

get_filename_component(XBOXRECOMP_KERNEL_DIR "${XBOXRECOMP_PLATFORM_DIR}/../kernel" ABSOLUTE)
add_subdirectory(
    "${XBOXRECOMP_KERNEL_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}/xboxrecomp-kernel")

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

# The private generated code is compiled for the actual APK, not as a
# disconnected audit-only archive. The entry-point reference in native_entry.cpp
# forces the linker to pull the generated XBE entry object into libmoh_android.
option(MOH_BUILD_GENERATED_RECOMP "Build and link private generated game code for Android ARM64" OFF)
set(RECOMP_GEN_DIR "" CACHE PATH "Private directory containing generated recompilation C sources")
set(RECOMP_ROOT_DIR "" CACHE PATH "Private recompilation root containing dispatcher and unresolved stubs")
if(MOH_BUILD_GENERATED_RECOMP)
    if(NOT ANDROID OR NOT CMAKE_ANDROID_ARCH_ABI STREQUAL "arm64-v8a")
        message(FATAL_ERROR "MOH_BUILD_GENERATED_RECOMP requires an Android arm64-v8a build")
    endif()
    if(NOT RECOMP_GEN_DIR OR NOT EXISTS "${RECOMP_GEN_DIR}/recomp_funcs.h"
       OR NOT EXISTS "${RECOMP_GEN_DIR}/recomp_types.h")
        message(FATAL_ERROR "RECOMP_GEN_DIR must contain private recomp_funcs.h and recomp_types.h")
    endif()
    file(GLOB MOH_RECOMP_GENERATED_SOURCES CONFIGURE_DEPENDS "${RECOMP_GEN_DIR}/*.c")
    if(NOT MOH_RECOMP_GENERATED_SOURCES)
        message(FATAL_ERROR "No generated game C sources found in RECOMP_GEN_DIR")
    endif()
    if(NOT RECOMP_ROOT_DIR OR NOT EXISTS "${RECOMP_ROOT_DIR}/recomp_dispatch.c"
       OR NOT EXISTS "${RECOMP_ROOT_DIR}/recomp_stubs_unresolved.c")
        message(FATAL_ERROR "RECOMP_ROOT_DIR must contain recomp_dispatch.c and recomp_stubs_unresolved.c")
    endif()
    list(APPEND MOH_RECOMP_GENERATED_SOURCES
        "${RECOMP_ROOT_DIR}/recomp_dispatch.c"
        "${RECOMP_ROOT_DIR}/recomp_stubs_unresolved.c"
    )
    add_library(moh_generated_recomp_arm64 STATIC ${MOH_RECOMP_GENERATED_SOURCES})
    target_include_directories(moh_generated_recomp_arm64 PRIVATE
        "${RECOMP_ROOT_DIR}"
        "${RECOMP_GEN_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}"
    )
    target_compile_definitions(moh_generated_recomp_arm64 PRIVATE RECOMP_GENERATED_CODE=1)
    target_compile_options(moh_generated_recomp_arm64 PRIVATE
        -include "${CMAKE_CURRENT_SOURCE_DIR}/recomp_sse_compat.h"
        -Wall -Wextra -Wno-parentheses-equality
    )
    target_link_libraries(moh_generated_recomp_arm64 PRIVATE m)
    target_link_libraries(moh_android PRIVATE moh_generated_recomp_arm64)
    target_compile_definitions(moh_android PRIVATE MOH_HAS_GENERATED_RECOMP=1)
endif()

include("${CMAKE_CURRENT_SOURCE_DIR}/recomp_sse_compat_test.cmake")
