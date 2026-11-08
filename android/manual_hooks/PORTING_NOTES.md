# Android manual-hook porting inventory

This file records the source-level dependency audit and implementation status for the seven manual game patches and four entry hooks. Compilation and host contract tests do not prove that the title boots; runtime integration remains a separate milestone.

## Platform boundary

The Android layer keeps the Xbox guest ABI (32-bit registers, guest virtual addresses, little-endian guest memory) separate from the ARM64 host ABI. Guest reads are bounds-checked and decoded without assuming host alignment or byte order.

Do not bring desktop-only input, camera/FOV, keyboard, sampler, or diagnostic features into the Android target. Preserve the original guest-game behavior where possible and omit only the desktop injection/diagnostic additions.

## Seven game patches

| Guest address / function | Source behavior and host dependencies | Android status |
|---|---|---|
| `0x000137C0 game_patch_000137C0` | Frame/vblank limiter; desktop additions use `patch_fps`, host-clock pacing, `YieldProcessor` and game sampler. | **Ported:** original guest vblank/frame-count path only. Desktop FPS pacing and sampler calls removed. |
| `0x001149E0 game_patch_001149E0` | Camera/player yaw update; desktop-only `aim_inject_yaw()` is prepended. | **Ported:** original lifted guest body; mouse-aim injection removed. |
| `0x00114B60 game_patch_00114B60` | Camera/player pitch update; desktop additions call `look_is_p1`, `game_pc_note` and `aim_inject_pitch()`. | **Ported:** original lifted guest body; player probe and mouse-aim injection removed. |
| `0x0011AC60 game_patch_0011AC60` | Sprint/player update; desktop `sprint_apply()` is prepended. | **Ported:** original lifted guest body; desktop sprint injection removed. |
| `0x0017B210 game_patch_0017B210` | Timer/object update; uses guest memory, translated x87 stack helpers and calls guest functions. | **Ported:** lifted guest body retained, including x87 and guest-call behavior. No direct Win32 dependency was found. |
| `0x00196467 game_patch_00196467` | DirectSound stream flush/status/release sequence; desktop patch uses `GetTickCount` and `Sleep` for a bounded wait. | Pending: adapt the bounded wait to Android monotonic time and the actual audio-stream lifetime contract. Never release a stream while the audio service owns it. |
| `0x001AD630 game_patch_001AD630` | Audio emitter update; desktop addition calls `emitter_reap()` and optional FPS-based sound accumulation using environment variables. | Pending: map Android audio emitter ownership/lifetime before porting cleanup or frame accumulation. |

The five ported functions live in `android/app/src/main/cpp/game_patches_android.c` and are included in the optional ARM64 generated-code compile target. Their latest generated-code CI compile is pending.

## Four entry hooks

| Entry | Source responsibility | Android status |
|---|---|---|
| `sub_0018B480_enter` | FOV/projection parameter widening and desktop FOV diagnostics. | Implemented as an intentional no-op; Android keeps native game projection values. |
| `sub_0018E150_enter` | Camera object FOV rewrite and memory-watch diagnostics. | Implemented as an intentional no-op. |
| `sub_001C68C0_enter` | Projection-call FOV/far-plane modification and FOV diagnostics. | Implemented as an intentional no-op. |
| `sub_002DE0C0_enter` | D3D BlockOnTime GPU semaphore setup and software-method event signaling. | Implemented: register the guest semaphore with the NV2A executor and signal the guest D3D busy event on software method 5. No BLOCK_STATS, stack sampling or desktop memory-watch code is imported. |

The Android DirectSound helper entries `sub_002ECCBF_enter` and `sub_002ECD1F_enter` also use checked guest-memory reads. The former records the flushed stream pointer; the latter yields during repeated status polling.

## Validation and remaining integration

1. Host tests cover guest-memory bounds, unaligned little-endian access, hook registration, and the D3D semaphore/software-method/event contract.
2. The standalone manual-hook library has passed host CMake/CTest and Android `arm64-v8a` compilation.
3. The five lifted game patches and Android hook source are being compiled together against the private generated game headers by the ARM64 bootstrap workflow.
4. Port the two remaining audio patches only after the Android audio service lifetime/completion contracts are verified.
5. Connect the generated archive to the Android loader/runtime only after XBE image mapping, guest memory initialization, kernel startup and manual dispatch are all ready.

A successful static-library or APK build is not evidence that Medal of Honor has booted. Keep the PR as draft and do not replace the known-working graphics APK until runtime smoke tests pass.
