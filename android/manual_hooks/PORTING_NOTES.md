# Android manual-hook porting inventory

This file records the source-level dependency audit and implementation status for the seven manual game patches and four entry hooks. Compilation and host contract tests do not prove that the title boots; runtime integration remains a separate milestone.

## Platform boundary

The Android layer keeps the Xbox guest ABI (32-bit registers, guest virtual addresses, little-endian guest memory) separate from the ARM64 host ABI. Guest reads in the Android entry hooks are bounds-checked and use unaligned-safe copies. The lifted game functions continue to use the generated runtime's guest-memory/register contract.

Do not bring desktop-only input, camera/FOV, keyboard, sampler, or diagnostic features into the Android target. Preserve the original guest-game behavior where possible and omit only the desktop injection/diagnostic additions.

## Seven game patches

All seven Android game-patch symbols are now present in `android/app/src/main/cpp/game_patches_android.c`, using the private generated runtime headers.

| Guest address / function | Android implementation status |
|---|---|
| `0x000137C0 game_patch_000137C0` | Original guest vblank/frame-count path retained. Desktop FPS pacing, sampler calls and `YieldProcessor` removed. |
| `0x001149E0 game_patch_001149E0` | Original lifted yaw update retained; native mouse-aim injection removed. |
| `0x00114B60 game_patch_00114B60` | Original lifted pitch update retained; player probe and native mouse-aim injection removed. |
| `0x0011AC60 game_patch_0011AC60` | Original lifted sprint/player update retained; desktop sprint injection removed. |
| `0x0017B210 game_patch_0017B210` | Lifted timer/object update retained, including x87 and guest-call behavior. No direct Win32 dependency was found. |
| `0x00196467 game_patch_00196467` | Android monotonic-clock bounded stream-flush wait. If the stream still reports active after 2 seconds, continue without releasing it. |
| `0x001AD630 game_patch_001AD630` | Original guest emitter-update body retained. Desktop FPS accumulation and the diagnostic/rescue `emitter_reap` routine are omitted; Android emitter-pool cleanup remains a separate audio-lifetime task. |

## Four entry hooks

| Entry | Android implementation status |
|---|---|
| `sub_0018B480_enter` | Intentional no-op; Android retains native game projection values. |
| `sub_0018E150_enter` | Intentional no-op; desktop FOV rewrite and memory-watch diagnostics excluded. |
| `sub_001C68C0_enter` | Intentional no-op; desktop projection/FOV rewrite excluded. |
| `sub_002DE0C0_enter` | Implemented: register the guest semaphore with the NV2A executor and signal the guest D3D busy event on software method 5. |

The Android DirectSound helper entries `sub_002ECCBF_enter` and `sub_002ECD1F_enter` also use checked guest-memory reads. The former records the flushed stream pointer; the latter yields during repeated status polling.

## Validation and remaining integration

1. Host contract tests cover guest-memory bounds, unaligned little-endian access, hook registration, and the D3D semaphore/software-method/event contract.
2. The standalone manual-hook library and entry-hook behavior tests have passed host CMake/CTest and Android `arm64-v8a` compilation.
3. The generated-code ARM64 workflow is compiling the seven Android patch bodies against the private generated game headers; the latest run must finish successfully before treating that compile as validated.
4. Port the Android emitter-pool cleanup only after the audio service's emitter ownership and lifetime rules are confirmed. Do not copy desktop diagnostic or keyboard code.
5. Connect the generated archive to the Android loader/runtime only after XBE image mapping, guest memory initialization, kernel startup and manual dispatch are ready.

A successful static-library or APK build is not evidence that Medal of Honor has booted. Keep the PR as draft and do not replace the known-working graphics APK until runtime smoke tests pass.
