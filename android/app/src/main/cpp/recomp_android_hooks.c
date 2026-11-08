/*
 * Android adaptations for title-specific entry hooks.
 *
 * Keep these hooks separate from the desktop implementation in
 * source/mygame/src/recomp_manual.c: that file depends on Win32 APIs and
 * desktop runtime state. Each hook here must preserve the original guest
 * behavior while using Android/POSIX host facilities.
 */
#include <stddef.h>
#include <stdint.h>
#include <time.h>
#include <string.h>

extern __thread uint32_t g_esp;
extern ptrdiff_t xbox_GetMemoryOffset(void);
extern int xbox_IsXboxAddress(uintptr_t address);

/* NV2A pushbuffer executor hooks. */
extern void nv2a_pb_set_semaphore_target(uint32_t guest_va);
extern void nv2a_pb_set_swmethod_handler(void (*fn)(uint32_t param));
extern void xbox_KeSetEventGuest(uint32_t guest_va);

/* DirectSound FlushEx bookkeeping mirrored from the desktop manual hook.
 * This is diagnostic state only: it never edits the APU stream state or
 * forces a stream to stop. */
static volatile uint32_t s_flush_mcpx;
static volatile uint64_t s_flush_monotonic_ms;

static uint64_t monotonic_ms(void)
{
    struct timespec now;

    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0)
        return 0;

    return (uint64_t)now.tv_sec * 1000u
         + (uint64_t)now.tv_nsec / 1000000u;
}

/* Read the 32-bit Xbox guest ABI without assuming host alignment. */
static int android_guest_range(uint32_t va, size_t size)
{
    uint64_t last;

    if (size == 0)
        return xbox_IsXboxAddress((uintptr_t)va);

    last = (uint64_t)va + (uint64_t)size - 1u;
    if (last > UINT32_MAX)
        return 0;

    return xbox_IsXboxAddress((uintptr_t)va)
        && xbox_IsXboxAddress((uintptr_t)last);
}

static int android_guest_read_u32(uint32_t va, uint32_t *out)
{
    uintptr_t host_address;
    ptrdiff_t offset;

    if (!out || !android_guest_range(va, sizeof(*out)))
        return 0;

    offset = xbox_GetMemoryOffset();
    host_address = (uintptr_t)((ptrdiff_t)(uintptr_t)va + offset);
    memcpy(out, (const void *)host_address, sizeof(*out));
    return 1;
}

/*
 * sub_002ECCBF_enter:
 * FlushEx(iface, rt, flags). The desktop hook obtains the CMcpxStream pointer
 * from *(iface + 4), equivalent to iface - 0x1C + 0x20. Invalid or not-yet
 * mapped guest arguments are ignored rather than dereferenced on the host.
 */
void sub_002ECCBF_enter(void)
{
    uint32_t stack_arg, iface, mcpx_guest, mcpx;

    if (g_esp > UINT32_MAX - 4u) {
        s_flush_mcpx = 0;
        s_flush_monotonic_ms = monotonic_ms();
        return;
    }

    stack_arg = g_esp + 4u;
    if (!android_guest_read_u32(stack_arg, &iface) || !iface
        || iface > UINT32_MAX - 4u
        || !android_guest_read_u32(iface + 4u, &mcpx)) {
        s_flush_mcpx = 0;
        s_flush_monotonic_ms = monotonic_ms();
        return;
    }

    mcpx_guest = mcpx;
    s_flush_mcpx = mcpx_guest;
    s_flush_monotonic_ms = monotonic_ms();
}

/*
 * sub_002ECD1F_enter:
 * The title polls DirectSound GetStatus in a tight loop while waiting for a
 * flushed stream to stop. Yield after 64 consecutive polls of the same
 * interface so the audio service thread can make progress.
 */
void sub_002ECD1F_enter(void)
{
    static _Thread_local uint32_t last_iface;
    static _Thread_local unsigned streak;
    uint32_t stack_arg, iface;

    if (g_esp > UINT32_MAX - 4u
        || !android_guest_read_u32(g_esp + 4u, &iface)) {
        last_iface = 0;
        streak = 0;
        return;
    }

    stack_arg = g_esp + 4u;
    (void)stack_arg;

    if (iface != last_iface) {
        last_iface = iface;
        streak = 0;
        return;
    }

    if (++streak > 64u) {
        struct timespec delay = { 0, 1000000L };
        (void)nanosleep(&delay, NULL);
    }
}

/* The three desktop FOV entry hooks only modify optional desktop camera
 * settings and emit diagnostics. Android keeps the game's native projection
 * values unchanged, so these hooks intentionally do nothing. */
void sub_0018B480_enter(void) { }
void sub_0018E150_enter(void) { }
void sub_001C68C0_enter(void) { }

/* D3D BlockOnTime: connect the title's semaphore to the NV2A executor and
 * wake the guest D3D busy event when software method 5 is consumed. */
#define MOH_D3D_DEVICE_PTR 0x002E9548u
#define MOH_D3D_BUSY_EVENT 0x1DCCu

static void android_d3d_swmethod(uint32_t param)
{
    uint32_t dev, event_va;

    if ((param & 0x1Fu) != 5u
        || !android_guest_read_u32(MOH_D3D_DEVICE_PTR, &dev)
        || !dev || dev > UINT32_MAX - MOH_D3D_BUSY_EVENT)
        return;

    event_va = dev + MOH_D3D_BUSY_EVENT;
    if (android_guest_range(event_va, sizeof(uint32_t)))
        xbox_KeSetEventGuest(event_va);
}

void sub_002DE0C0_enter(void)
{
    static int registered;
    uint32_t dev, sem;

    if (registered)
        return;

    if (!android_guest_read_u32(MOH_D3D_DEVICE_PTR, &dev) || !dev
        || dev > UINT32_MAX - 0x30u
        || !android_guest_read_u32(dev + 0x30u, &sem)
        || !android_guest_range(sem, sizeof(uint32_t)))
        return;

    nv2a_pb_set_semaphore_target(sem);
    nv2a_pb_set_swmethod_handler(android_d3d_swmethod);
    registered = 1;
}
