# Android manual-hook porting inventory

This file records the source-level dependency audit for the seven manual game patches and four entry hooks. It is deliberately a porting plan, not a claim that the hooks are implemented.

## Platform boundary

The Android layer must keep the Xbox guest ABI (32-bit registers, guest virtual addresses, little-endian guest memory) separate from the ARM64 host ABI. Guest functions must not dereference guest addresses as host pointers. All guest reads/writes go through the checked memory adapter.

Do not bring desktop-only input, camera/FOV, keyboard, sampler, or diagnostic features into the Android target. Preserve the original guest-game behavior where possible and omit only the desktop injection/diagnostic additions.

## Seven game patches

| Guest address / function | Source behavior and host dependencies | Android treatment |
|---|---|---|
| `0x000137C0 game_patch_000137C0` | Frame/vblank limiter; desktop additions use `patch_fps`, QueryPerformanceCounter, YieldProcessor and game sampler. | Port guest frame-count logic separately from desktop FPS override. Add Android monotonic-clock/yield adapter only if a configurable host-FPS mode is required. Do not import sampler/diagnostics. |
| `0x001149E0 game_patch_001149E0` | Camera/player yaw update; desktop-only `aim_inject_yaw()` is prepended. | Keep original lifted guest body; omit mouse-aim injection. No FOV feature is needed. |
| `0x00114B60 game_patch_00114B60` | Camera/player pitch update; desktop additions call `look_is_p1`, `game_pc_note` and `aim_inject_pitch()`. | Keep original lifted guest body; omit player probe and mouse-aim injection. |
| `0x0011AC60 game_patch_0011AC60` | Sprint/player update; desktop `sprint_apply()` is prepended. | Keep original lifted guest body; omit desktop sprint injection. |
| `0x0017B210 game_patch_0017B210` | Timer/object update; uses guest memory, translated x87 stack helpers and calls guest functions. No direct Win32 API in the audited body. | Candidate for an early functional port after confirming the Android runtime exports the same x87 and guest-call contracts. |
| `0x00196467 game_patch_00196467` | DirectSound stream flush/status/release sequence; desktop patch uses GetTickCount and Sleep for a bounded wait. | Requires an Android audio-stream completion contract and monotonic wait/sleep adapter. Do not copy Win32 calls; do not release a stream while audio service still owns it. |
| `0x001AD630 game_patch_001AD630` | Audio emitter update; desktop addition calls `emitter_reap()` and optional FPS-based sound accumulation using environment variables. | Port guest emitter logic only after audio runtime ownership/lifetime is mapped. Omit desktop emitter diagnostics and environment-driven frame pacing. |

## Four entry hooks

| Entry | Source responsibility | Android decision |
|---|---|---|
| `sub_0018B480_enter` | FOV/projection parameter widening; uses desktop FOV state, settings, timing, and optional memory watches. | Intentionally not ported. Keep the hook unregistered/no-op on Android. |
| `sub_0018E150_enter` | Camera object FOV rewrite; uses desktop FOV state and memory-watch diagnostics. | Intentionally not ported. Keep the hook unregistered/no-op on Android. |
| `sub_001C68C0_enter` | Projection-call FOV/far-plane modification and FOV diagnostics. | Intentionally not ported. Keep the hook unregistered/no-op on Android. |
| `sub_002DE0C0_enter` | D3D BlockOnTime GPU semaphore setup; depends on pushbuffer executor semaphore target, software-method callback, guest event signaling, and optional host-clock diagnostics. | Required only when the Android NV2A/pushbuffer executor provides these contracts. Port the minimal semaphore/event setup; exclude BLOCK_STATS, stack sampling and desktop memory-watch diagnostics. |

## Integration order

1. Verify guest memory helpers and register contracts (host tests + ARM64 compile).
2. Wire only the required Android runtime services into a separate adapter; keep unported hooks unregistered.
3. Port low-dependency guest patches first, then audio and GPU synchronization after their Android service contracts are confirmed.
4. Add runtime tests for register preservation, guest stack arguments, memory bounds, and callback behavior.
5. Only then connect the layer to the Android game runtime. Never use a successful static-library build as evidence that the game boots.
