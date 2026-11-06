# Android port (work in progress)

This directory is the Android ARM64 application/bootstrap for Medal of Honor:
European Assault.

## Current milestone

- Android Gradle application structure.
- ARM64-only (arm64-v8a) NDK configuration.
- Kotlin launcher activity calling a native JNI library.
- Native CMake target for packaging a shared library into a debug APK.
- First Android runtime step: the portable POSIX host layer (`platform`, including host timing, thread primitives and generic Win32 compatibility helpers) is now added to the NDK build.
- Xbox kernel/HLE sources compile in the Android NDK build.

**This bootstrap is not the game yet.** It currently displays a status message.
The Xbox graphics/audio/input runtime, generated game code and game assets are
not connected. This milestone is not playable.

## Build

The GitHub Actions workflow builds the ARM64 debug APK from the repository root:

```sh
gradle --no-daemon -p android :app:assembleDebug
```

The debug APK is produced at:
`android/app/build/outputs/apk/debug/app-debug.apk`

## Verified graphics architecture

The existing desktop graphics path is not directly Android-compatible:

- Windows uses the D3D8 compatibility implementation backed by D3D11.
- Linux/macOS select `src/d3d/d3d8_gl.c`, which creates an SDL2 window and requests an OpenGL 3.3 core context through libepoxy.
- That GL backend is an early implementation: its own source comments identify unfinished texture-format mapping, Xbox vertex-shader translation and pixel register-combiner translation.
- The NV2A PGRAPH implementation is currently `nv2a_pgraph_d3d11.c` and calls the D3D8 interface. It is not an independent GLES renderer.
- The current Android CMake target intentionally builds only the platform and kernel libraries. It does not link the desktop graphics stack.

Therefore, adding SDL2 or changing the requested GL context version alone is insufficient. The Android port needs a deliberate GLES 3.x backend and a compatible NV2A/D3D8 presentation path. The first renderer milestone should be a small Android EGL/GLES surface with shader compilation, clear, viewport, textured quad and present tests, followed by the D3D8 state/resource/draw implementation and NV2A integration.

## Porting sequence

1. Compile and validate the portable POSIX host-platform layer with the Android NDK (complete).
2. Compile and validate the Xbox kernel/HLE sources for Android/Bionic (initial compile milestone complete; runtime behavior still needs device validation).
3. Build an Android EGL/GLES 3.x rendering smoke test, independent of the game.
4. Port the D3D8 compatibility layer from the desktop GL implementation to GLES, accounting for shader-language and texture-format differences.
5. Adapt NV2A PGRAPH calls to the Android graphics backend; remove the current D3D11-specific dependency from the Android path.
6. Implement Android audio output and connect the Xbox DirectSound/APU mixer.
7. Implement touchscreen controls and Android game-controller input.
8. Integrate privately generated recompilation output and user-provided game data through Android document/file APIs.
9. Validate gameplay, performance, lifecycle, saves and APK packaging on a physical ARM64 device.

## Generated game code and assets

The repository intentionally excludes the large machine-translated game code
generated from the owner's `DEFAULT.XBE`, as well as copyrighted game assets.
The game cannot run without the generated code and the user's legally obtained
game data.

The PC project already supports `RECOMP_GEN_DIR` to point CMake at a private
generated-source directory. The Android workflow does not currently receive
that private directory, and it must not be committed to this public repository.
A later integration step must provide it to GitHub Actions through a private,
access-controlled artifact or another user-approved private transfer route.

Never commit or upload `DEFAULT.XBE`, generated game C files, or game assets
to this public repository.

## Android/Bionic compatibility

The POSIX host layer has Android-specific fallbacks for thread affinity,
secure memory clearing, and anonymous shared-memory file descriptors.
`TerminateThread` reports `ERROR_CALL_NOT_IMPLEMENTED` on Android because
Bionic does not provide `pthread_cancel`; callers must not assume forced
thread termination is available. The Android NDK build validates compilation
of the host/kernel layers only, not the complete game runtime.
