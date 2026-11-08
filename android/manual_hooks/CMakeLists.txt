cmake_minimum_required(VERSION 3.16)
project(moh_android_manual_hooks C)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
add_library(moh_android_manual_hooks STATIC src/android_manual_hooks.c)
target_include_directories(moh_android_manual_hooks PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
  target_compile_options(moh_android_manual_hooks PRIVATE -Wall -Wextra -Werror -Wconversion -Wsign-conversion)
endif()
include(CTest)
if(BUILD_TESTING)
  add_executable(test_android_manual_hooks tests/test_android_manual_hooks.c)
  target_link_libraries(test_android_manual_hooks PRIVATE moh_android_manual_hooks)
  add_test(NAME android_manual_hook_contract COMMAND test_android_manual_hooks)
endif()

# Exercise the Android-specific entry hooks with a small simulated Xbox guest
# address space. This verifies the GPU semaphore/event contract on the host.
if(BUILD_TESTING)
  add_executable(test_android_enter_hooks
    tests/test_android_enter_hooks.c
    ../app/src/main/cpp/recomp_android_hooks.c
  )
  set_target_properties(test_android_enter_hooks PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON)
  target_compile_definitions(test_android_enter_hooks PRIVATE _POSIX_C_SOURCE=200809L)
  target_compile_options(test_android_enter_hooks PRIVATE -Wall -Wextra -Werror)
  add_test(NAME android_manual_enter_hooks COMMAND test_android_enter_hooks)
endif()
