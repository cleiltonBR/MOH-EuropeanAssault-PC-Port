/*
 * Android-only adapters for the game's manually lifted functions.
 *
 * The core game update bodies are shared with the Windows build. Desktop-only
 * mouse aim, sprint injection, FOV experiments and profiling callbacks are
 * intentionally not enabled here until their Android input/UI counterparts
 * exist. The underlying lifted update routines still execute.
 */
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <android/log.h>
#include "platform/xbox_winnt.h"

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

/* Android equivalents for the small game-patch settings surface.
 * Desktop mouse/keyboard controls are not enabled on Android yet, so their
 * counters remain neutral and developer hotkeys stay opt-in. */
volatile unsigned g_mkb_rt_presses = 0;
volatile double g_mkb_gate_until_ms = 0.0;

int game_dev_keys(void)
{
    const char *value = getenv("RECOMP_DEV_KEYS");
    return value && *value && *value != '0';
}

int game_blood_get(void)
{
    const char *value = getenv("RECOMP_BLOOD");
    int level = value && *value ? atoi(value) : 0;
    return level < 0 ? 0 : level > 3 ? 3 : level;
}

/* Mouse-only aim correction has no Android counterpart yet. These report the
 * neutral, unscoped game view rather than inventing touch/gamepad state. */
int game_scope_up(void) { return 0; }
float game_view_fov_deg(void) { return 70.0f; }

extern int moh_android_is_foreground(void);

HWND GetForegroundWindow(void)
{
    return moh_android_is_foreground()
        ? (HWND)(uintptr_t)1 : (HWND)0;
}

DWORD GetWindowThreadProcessId(HWND window, LPDWORD process_id)
{
    if (!window) {
        if (process_id) *process_id = 0;
        return 0;
    }
    if (process_id) *process_id = (DWORD)getpid();
    /* The caller only compares the process id; no Win32 window thread exists. */
    return 1;
}

void recomp_osd_show(const char *message)
{
    __android_log_print(ANDROID_LOG_INFO, "MOH-Recomp", "%s",
                        message ? message : "");
}

/* Optional desktop FOV instrumentation/overrides. */
void sub_0018B480_enter(void) {}
void sub_0018E150_enter(void) {}
void sub_001C68C0_enter(void) {}

/* Desktop push-buffer timing diagnostics only; Android GPU execution is a
 * separate integration task and is not faked by this hook. */
void sub_002DE0C0_enter(void) {}
