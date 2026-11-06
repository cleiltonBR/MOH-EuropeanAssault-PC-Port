# Android port (work in progress)

This directory is the Android ARM64 application/bootstrap for Medal of Honor:
European Assault.

## Current milestone

- Android Gradle application structure.
- ARM64-only (arm64-v8a) NDK configuration.
- Kotlin launcher activity calling a native JNI library.
- Native CMake target for packaging a shared library into a debug APK.
- First Android runtime step: the portable POSIX host layer (`platform`, including host timing, thread primitives and generic Win32 compatibility helpers) is now added to the NDK build.

**This bootstrap is not the game yet.** It currently displays a status message.
The Xbox recompilation runtime, generated game code, graphics, audio, input,
file access and game assets are not connected. This milestone is not playable.

## Build

Open this android/ directory as a Gradle project in Android Studio, or run
gradle --no-daemon assembleDebug from this directory.

The debug APK is produced at:
app/build/outputs/apk/debug/app-debug.apk

## Porting sequence

1. Compile and validate the portable POSIX host-platform layer with the Android NDK (initial integration is now in place).
2. Audit and adapt the Xbox kernel HLE for Android/Bionic, especially memory mapping, exception handling and thread semantics.
3. Replace desktop OpenGL 3.3/SDL assumptions with an Android OpenGL ES 3.x context and a compatible D3D8 translation path.
4. Implement Android audio output and connect the Xbox DirectSound/APU mixer.
5. Implement touchscreen controls and Android game-controller input.
6. Integrate privately generated recompilation output and user-provided game data through Android document/file APIs.
7. Validate gameplay, performance, lifecycle, saves and APK packaging.

## Important source limitation

The repository intentionally excludes source/mygame/src/recomp/gen/, the large
machine-translated game code generated from the owner's DEFAULT.XBE. A legal
USA/NTSC-U game copy and the documented recompiler pipeline are required to
generate it. The Android port cannot run the game without that code and data.


## Private XBE recompilation from Android / Termux

The repository does not contain the game's generated C code. If you have your
own USA/NTSC-U `DEFAULT.XBE`, the included Termux helper runs the toolkit's
five analysis/recompilation stages on the Android device:

1. XBE header/section/import parsing.
2. x86 disassembly and function detection.
3. CRT / RenderWare / XDK / game-function identification.
4. ABI and calling-convention analysis.
5. x86-to-C recompilation, split into manageable source files.

### Termux setup

Install Termux from a trusted source, open it, then run:

```sh
pkg update
pkg install git python coreutils clang make
termux-setup-storage
git clone --branch android-port https://github.com/cleiltonBR/MOH-EuropeanAssault-PC-Port.git
cd MOH-EuropeanAssault-PC-Port
bash android/tools/recompile-xbe-termux.sh ~/storage/downloads/DEFAULT.XBE
```

Replace the XBE path if the file is stored somewhere else. The first run installs
the Python Capstone dependency. The analysis can take a long time and use
substantial memory and storage; keep the phone charging and do not let Android
put Termux to sleep.

### Where the output goes

The script copies the XBE into `$HOME/MOH-EuropeanAssault-private/input/` and
writes all generated code and analysis reports under
`$HOME/MOH-EuropeanAssault-private/recompilation/`. These paths are outside the
Git checkout. The XBE and generated code are not pushed to GitHub.

For a native desktop build, `source/mygame/CMakeLists.txt` now accepts
`RECOMP_GEN_DIR` so a local build can point at the private generated folder.
This only prepares the generated sources for a later build; it does not make
the current Android bootstrap playable.

### Important status

- The generated code has **not** been generated or compiled by this repository
  workflow yet; the helper must be run on the device that holds the XBE.
- The project's existing `source/gen_hand_edits.txt` documents historical
  edits made inside a previous generated output. They are not automatically
  replayed by the toolkit, so those changes must be reviewed individually
  before treating a fresh generation as equivalent to the existing PC port.
- The Android JNI target still does not link the Xbox runtime or generated game
  code. The next engineering milestone after generation is adapting the
  runtime/platform, graphics, audio and input layers for Android ARM64.
- Never commit or upload `DEFAULT.XBE`, generated game C files, or game assets
  to this public repository.


### Android/Bionic compatibility

The POSIX host layer now has Android-specific fallbacks for thread affinity, secure memory clearing, and anonymous shared-memory file descriptors. `TerminateThread` reports `ERROR_CALL_NOT_IMPLEMENTED` on Android because Bionic does not provide `pthread_cancel`; callers must not assume forced thread termination is available. The Android NDK build validates this host layer only, not the game runtime.
