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

/* Match the generated runtime's per-thread x86 register state. */
extern __thread uint32_t g_esp;
extern ptrdiff_t g_xbox_mem_offset;

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

/*
 * sub_002ECCBF_enter:
 * FlushEx(iface, rt, flags). The desktop hook obtains the CMcpxStream pointer
 * from *(iface + 4), equivalent to iface - 0x1C + 0x20. Preserve the guest
 * offset's sign when translating the 32-bit guest address to host memory.
 */
void sub_002ECCBF_enter(void)
{
    uint32_t stack_arg = (uint32_t)(g_esp + 4u);
    uintptr_t stack_host = (uintptr_t)((ptrdiff_t)(uintptr_t)stack_arg
                                       + g_xbox_mem_offset);
    uint32_t iface = *(volatile uint32_t *)stack_host;
    uint32_t mcpx_guest;
    uintptr_t mcpx_host;

    if (!iface) {
        s_flush_mcpx = 0;
        s_flush_monotonic_ms = monotonic_ms();
        return;
    }

    mcpx_guest = iface + 4u;
    mcpx_host = (uintptr_t)((ptrdiff_t)(uintptr_t)mcpx_guest
                            + g_xbox_mem_offset);
    s_flush_mcpx = *(volatile uint32_t *)mcpx_host;
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
    uint32_t stack_arg = (uint32_t)(g_esp + 4u);
    uintptr_t host_addr = (uintptr_t)((ptrdiff_t)(uintptr_t)stack_arg
                                      + g_xbox_mem_offset);
    uint32_t iface = *(volatile uint32_t *)host_addr;

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
