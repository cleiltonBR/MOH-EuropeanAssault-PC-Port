/*
 * Android-only adapters for the game's manually lifted functions.
 *
 * The core game update bodies are shared with the Windows build. Desktop-only
 * mouse aim, sprint injection, FOV experiments and profiling callbacks are
 * intentionally not enabled here until their Android input/UI counterparts
 * exist. The underlying lifted update routines still execute.
 */
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <android/log.h>
#include "platform/xbox_winnt.h"
#include "kernel/kernel.h"
#include "kernel/xbox_memory_layout.h"

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

/*
 * Android D3D_BlockOnTime bridge.
 *
 * The desktop hook registers the guest semaphore with the NV2A pushbuffer
 * executor and handles D3D software method 5 by signalling the device's busy
 * event. This is not timing instrumentation: without the semaphore target,
 * BACK_END_WRITE_SEMAPHORE_RELEASE cannot complete the guest fence; without
 * the software-method callback, a title that waits for the GPU interrupt can
 * sleep forever.
 *
 * Keep the guest addresses and callback semantics identical to the original
 * game patch, but use the Android runtime's already-linked kernel/NV2A code.
 */
#define MOH_GAME_D3D_DEVICE_PTR 0x002E9548u
#define MOH_GAME_D3D_BUSY_EVENT 0x1DCCu

extern void nv2a_pb_set_semaphore_target(uint32_t guest_va);
extern void nv2a_pb_set_swmethod_handler(void (*fn)(uint32_t param));
extern void xbox_KeSetEventGuest(uint32_t guest_va);

static int s_d3d_pb_hooks_installed;

static int moh_guest_dword_readable(uint32_t va)
{
    const uint64_t end = (uint64_t)va + sizeof(uint32_t);
    const size_t mapped = xbox_GetMappedSize();

    if (!xbox_GetMemoryBase() || va < 0x1000u)
        return 0;
    if (end <= mapped)
        return 1;
    return va >= XBOX_CONTIG_BASE\n        && end <= (uint64_t)XBOX_CONTIG_BASE + XBOX_CONTIG_SIZE;
}

static uint32_t moh_guest_read32(uint32_t va)
{
    const uintptr_t host = (uintptr_t)((ptrdiff_t)(uintptr_t)va
                                       + g_xbox_mem_offset);
    return *(volatile uint32_t *)host;
}

static void moh_d3d_swmethod(uint32_t param)
{
    uint32_t device;

    if ((param & 0x1Fu) != 5u
            || !moh_guest_dword_readable(MOH_GAME_D3D_DEVICE_PTR))
        return;

    device = moh_guest_read32(MOH_GAME_D3D_DEVICE_PTR);
    if (!device || !moh_guest_dword_readable(device + MOH_GAME_D3D_BUSY_EVENT))
        return;

    xbox_KeSetEventGuest(device + MOH_GAME_D3D_BUSY_EVENT);
}

void sub_002DE0C0_enter(void)
{
    uint32_t device, semaphore;

    if (s_d3d_pb_hooks_installed
            || !moh_guest_dword_readable(MOH_GAME_D3D_DEVICE_PTR))
        return;

    device = moh_guest_read32(MOH_GAME_D3D_DEVICE_PTR);
    if (!device || !moh_guest_dword_readable(device + 0x30u))
        return;

    semaphore = moh_guest_read32(device + 0x30u);
    if (!semaphore || !moh_guest_dword_readable(semaphore))
        return;

    nv2a_pb_set_semaphore_target(semaphore);
    nv2a_pb_set_swmethod_handler(moh_d3d_swmethod);
    s_d3d_pb_hooks_installed = 1;
    __android_log_print(ANDROID_LOG_INFO, "MOH-Recomp",
        "D3D/NV2A synchronization installed: device=0x%08X semaphore=0x%08X",
        device, semaphore);
}
