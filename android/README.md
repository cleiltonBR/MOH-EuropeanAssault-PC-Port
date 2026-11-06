# Android port (work in progress)

This directory is the Android ARM64 application/bootstrap for Medal of Honor:
European Assault.

## Current milestone

- Android Gradle application structure.
- ARM64-only (arm64-v8a) NDK configuration.
- Kotlin launcher activity calling a native JNI library.
- Native CMake target for packaging a shared library into a debug APK.

**This bootstrap is not the game yet.** It currently displays a status message.
The Xbox recompilation runtime, generated game code, graphics, audio, input,
file access and game assets are not connected. This milestone is not playable.

## Build

Open this android/ directory as a Gradle project in Android Studio, or run
gradle --no-daemon assembleDebug from this directory.

The debug APK is produced at:
app/build/outputs/apk/debug/app-debug.apk

## Porting sequence

1. Make the Android NDK build compile the existing platform/runtime sources.
2. Replace desktop OpenGL 3.3/SDL assumptions with an Android OpenGL ES 3.x
   context and compatible D3D8 translation path.
3. Implement Android audio output and connect the Xbox DirectSound/APU mixer.
4. Implement touchscreen controls and Android game-controller input.
5. Integrate generated recompilation output and user-provided game data through
   Android document/file APIs.
6. Validate gameplay, performance, lifecycle, saves and APK packaging.

## Important source limitation

The repository intentionally excludes source/mygame/src/recomp/gen/, the large
machine-translated game code generated from the owner's DEFAULT.XBE. A legal
USA/NTSC-U game copy and the documented recompiler pipeline are required to
generate it. The Android port cannot run the game without that code and data.
