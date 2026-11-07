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
    uint32_t iface = *(volatile uint32_t *)(
        (uintptr_t)stack_arg + (uintptr_t)g_xbox_mem_offset);

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
