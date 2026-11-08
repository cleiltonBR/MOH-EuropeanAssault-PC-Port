# Android ARM64 manual-hook boundary

This directory is standalone and does not modify or link the Windows game
executable or the working graphics APK.

The layer models the Xbox guest CPU as an explicit x86-32 register file,
independent of ARM64 host registers. Guest addresses remain 32-bit Xbox VAs.
Bounds-checked memory helpers use memcpy so unaligned x86 accesses are safe
on strict-alignment ARM64 hosts.

Seven patch IDs and four _enter IDs have stable registration slots. Nothing
is enabled by default. The original hook bodies are deliberately not copied
until their dependencies have Android implementations; this avoids importing
Windows APIs or presenting stubs as completed ports.

Host validation:
  cmake -S android/manual_hooks -B build/manual-hooks
  cmake --build build/manual-hooks --parallel
  ctest --test-dir build/manual-hooks --output-on-failure

Android ARM64 validation:
  cmake -S android/manual_hooks -B build/manual-hooks-arm64 \
    -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake" \
    -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-24 -DBUILD_TESTING=ON
  cmake --build build/manual-hooks-arm64 --parallel
  ctest --test-dir build/manual-hooks-arm64 --output-on-failure

Next gate: port each original patch body against this context API, one at a
time. The first three entry hooks require camera/FOV dependency removal or
replacement; 0x002DE0C0 requires Android-safe NV2A semaphore and software
method event handling. Do not link this library into the graphics APK until
the ARM64 build and guest-memory/register tests pass.
