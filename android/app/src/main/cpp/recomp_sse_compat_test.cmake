# Standalone compile-and-link check for the ARM64 generated-code SSE shim.
# Kept independent from moh_android and xbox_d3d8 by design.
add_executable(recomp_sse_compat_test EXCLUDE_FROM_ALL recomp_sse_compat_test.c)
target_compile_features(recomp_sse_compat_test PRIVATE c_std_11)
target_compile_options(recomp_sse_compat_test PRIVATE -Wall -Wextra -Werror)
target_link_libraries(recomp_sse_compat_test PRIVATE m)
