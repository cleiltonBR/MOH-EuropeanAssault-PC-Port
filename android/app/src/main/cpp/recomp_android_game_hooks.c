/*
 * Android-only adapters for the game's manually lifted functions.
 *
 * The core game update bodies are shared with the Windows build. Desktop-only
 * mouse aim, sprint injection, FOV experiments and profiling callbacks are
 * intentionally not enabled here until their Android input/UI counterparts
 * exist. The underlying lifted update routines still execute.
 */
#include <stdint.h>

void game_patch_000137C0(void);
void game_patch_0017B210(void);
void game_patch_001149E0(void);
void game_patch_00114B60(void);
void game_patch_0011AC60(void);
void game_patch_001AD630(void);
void game_patch_00196467(void);

/* The Windows launcher samples the game thread for profiling. Android has no
 * sampler yet; keep the shared frame-limiter patch independent of that tool. */
void game_sampler_note_thread(void) {}

extern void moh_android_note_frame(void);

void sub_000137C0(void)
{
    game_patch_000137C0();
    moh_android_note_frame();
}

void sub_0017B210(void) { game_patch_0017B210(); }
void sub_001149E0(void) { game_patch_001149E0(); }
void sub_00114B60(void) { game_patch_00114B60(); }
void sub_0011AC60(void) { game_patch_0011AC60(); }
void sub_001AD630(void) { game_patch_001AD630(); }
void sub_00196467(void) { game_patch_00196467(); }

/* Optional desktop FOV instrumentation/overrides. */
void sub_0018B480_enter(void) {}
void sub_0018E150_enter(void) {}
void sub_001C68C0_enter(void) {}

/* Desktop push-buffer timing diagnostics only; Android GPU execution is a
 * separate integration task and is not faked by this hook. */
void sub_002DE0C0_enter(void) {}
