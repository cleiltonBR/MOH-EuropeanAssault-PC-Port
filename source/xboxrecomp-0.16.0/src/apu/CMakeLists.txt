# MCPX APU Emulation Library
# Extracted from xemu (https://github.com/xemu-project/xemu)
#
# Voice Processor (VP): ADPCM/PCM decode, sample mixing, envelopes, filters
# Global Processor (GP): Stub - effects bypass
# Encode Processor (EP): Stub - direct mixbin passthrough

add_library(xbox_apu STATIC
    apu_core.c
    apu_vp.c
    apu_dsp.c
    apu_mmio_hook.c
    apu_xaudio2.c
    apu_wav.c
)

target_include_directories(xbox_apu PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# qemu_shim.h and the AArch64 MMIO decoder live in nv2a/.
target_include_directories(xbox_apu PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../nv2a
)
target_compile_features(xbox_apu PRIVATE c_std_11)

if(MSVC)
    target_compile_options(xbox_apu PRIVATE
        /W3
        /wd4244
        /wd4146
        /wd4267
    )
    target_compile_definitions(xbox_apu PRIVATE
        _CRT_SECURE_NO_WARNINGS
        _USE_MATH_DEFINES
    )
endif()

if(WIN32)
    target_link_libraries(xbox_apu PRIVATE platform kernel32 winmm xaudio2_8 ole32)
elseif(ANDROID)
    # Android uses the native AAudio device API; SDL2 is not part of this port.
    target_sources(xbox_apu PRIVATE apu_aaudio.c)
    target_link_libraries(xbox_apu PRIVATE platform aaudio m)
else()
    # macOS and Linux use the SDL2 queue backend.
    target_sources(xbox_apu PRIVATE apu_sdl2.c)
    find_package(SDL2 REQUIRED)
    target_link_libraries(xbox_apu PRIVATE platform SDL2::SDL2 m)
endif()
