cmake_minimum_required(VERSION 3.22.1)
project(moh_android LANGUAGES C CXX)

set(XBOXRECOMP_PLATFORM_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../source/xboxrecomp-0.16.0/src/platform")

add_subdirectory(
    "${XBOXRECOMP_PLATFORM_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}/xboxrecomp-platform")

# Android's audio device backend is native AAudio, not the desktop SDL2 path.
get_filename_component(XBOXRECOMP_APU_DIR "${XBOXRECOMP_PLATFORM_DIR}/../apu" ABSOLUTE)
add_subdirectory(
    "${XBOXRECOMP_APU_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}/xboxrecomp-apu")

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
# Keep the APU before the kernel archive: its implementation consumes kernel
# memory-layout and platform primitives. This also makes the native AAudio
# dependency part of the final Android shared library.
target_link_libraries(moh_android PRIVATE
    platform
    xbox_apu
    xbox_kernel
    xbox_d3d8
    android
    aaudio
    EGL
    GLESv3
    log
)
target_compile_definitions(moh_android PRIVATE MOH_ANDROID_PLATFORM_LINKED=1)

# Compile the private generated code for ARM64 as a separate validation target.
# It is deliberately not linked into libmoh_android until the Android XBE loader,
# guest-memory setup and kernel startup path are ported. Forcing xbe_entry_point
# into the APK before those dependencies exist causes an incomplete native link.
option(MOH_BUILD_GENERATED_RECOMP "Compile private generated game code for Android ARM64" OFF)
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
    get_filename_component(MOH_GAME_SRC_DIR
        "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../source/mygame/src" ABSOLUTE)
    get_filename_component(XBOXRECOMP_SRC_DIR "${XBOXRECOMP_PLATFORM_DIR}/.." ABSOLUTE)
    target_sources(moh_generated_recomp_arm64 PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/recomp_android_hooks.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/recomp_android_game_hooks.c"
        "${MOH_GAME_SRC_DIR}/game_patches.c"
    )
    target_include_directories(moh_generated_recomp_arm64 PRIVATE
        "${RECOMP_ROOT_DIR}"
        "${RECOMP_GEN_DIR}"
        "${MOH_GAME_SRC_DIR}"
        "${XBOXRECOMP_SRC_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}"
    )
    target_compile_definitions(moh_generated_recomp_arm64 PRIVATE
        RECOMP_GENERATED_CODE=1
        MOH_ANDROID_RUNTIME=1
    )
    target_compile_options(moh_generated_recomp_arm64 PRIVATE
        -include "${CMAKE_CURRENT_SOURCE_DIR}/recomp_sse_compat.h"
        -Wall -Wextra -Wno-parentheses-equality
        -Wno-unused-label
        -Wno-unused-variable
        -Wno-unused-but-set-variable
        -Wno-macro-redefined
        -Wno-gnu-anonymous-struct
    )
    target_link_libraries(moh_generated_recomp_arm64 PRIVATE m)
    add_dependencies(moh_android moh_generated_recomp_arm64)
    target_compile_definitions(moh_android PRIVATE MOH_GENERATED_RECOMP_COMPILED=1)
endif()

include("${CMAKE_CURRENT_SOURCE_DIR}/recomp_sse_compat_test.cmake")
