/*
 * Fixed-address mappings in the POSIX Win32 shim.
 *
 * Win32 refuses a VirtualAlloc or MapViewOfFileEx at an address that is
 * already in use. mmap(MAP_FIXED) does the opposite: it unmaps whatever is
 * there and succeeds. Linux has MAP_FIXED_NOREPLACE for this; Darwin has
 * nothing, so the shim claims the range with mach_vm_map(VM_FLAGS_FIXED) and
 * only then maps over its own placeholder. These checks pin both halves:
 *
 *   1. refusal      a fixed request over a live mapping fails, and the bytes
 *                   that were there survive it
 *   2. aliasing     two views of one CreateFileMapping are the same memory,
 *                   which is what the 28 RAM mirror views are built on
 *   3. no residue   a fixed view that fails after the range was claimed gives
 *                   the range back, so the same address can be mapped again
 *   4. release      UnmapViewOfFile and VirtualFree(MEM_RELEASE) free the
 *                   address for reuse
 *
 * The addresses come from the OS: map anonymously, note where it landed,
 * unmap. Nothing here assumes a fixed host layout.
 */
#include "win32_compat.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

static int failures = 0;

static void check(int ok, const char *what)
{
    printf("  %-60s %s\n", what, ok ? "PASS" : "FAIL");
    if (!ok) failures++;
}

/* An address range nothing currently occupies. */
static void *free_range(size_t len)
{
    void *p = mmap(NULL, len, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED) return NULL;
    munmap(p, len);
    return p;
}

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);

    const size_t len = 4 * (size_t)sysconf(_SC_PAGESIZE);

    HANDLE map = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
                                    0, (DWORD)len, NULL);
    check(map != NULL, "CreateFileMapping returns a mapping");
    if (!map) return 1;

    /* 1. Refusal. A live anonymous mapping stands in for whatever else the
     *    process might have at the address a guest asks for. */
    unsigned char *live = mmap(NULL, len, PROT_READ | PROT_WRITE,
                               MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (live == MAP_FAILED) { perror("mmap"); return 1; }
    memset(live, 0xC3, len);

    void *v = MapViewOfFileEx(map, FILE_MAP_ALL_ACCESS, 0, 0, len, live);
    check(v == NULL, "MapViewOfFileEx over a live mapping fails");
    check(live[0] == 0xC3 && live[len - 1] == 0xC3,
          "...and the live mapping keeps its contents");

    void *a = VirtualAlloc(live, len, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    check(a == NULL, "VirtualAlloc over a live mapping fails");
    check(live[0] == 0xC3 && live[len - 1] == 0xC3,
          "...and the live mapping keeps its contents");
    munmap(live, len);

    /* 2. Aliasing: a fixed view and a floating view of the same mapping. */
    unsigned char *at = free_range(len);
    unsigned char *v1 = MapViewOfFileEx(map, FILE_MAP_ALL_ACCESS, 0, 0, len, at);
    unsigned char *v2 = MapViewOfFile(map, FILE_MAP_ALL_ACCESS, 0, 0, len);
    check(at && v1 == at, "MapViewOfFileEx maps at a free address");
    check(v2 != NULL, "MapViewOfFile maps a second view");
    if (v1 == at && v2) {
        v1[0x10] = 0x5A;
        v2[len - 1] = 0xA5;
        check(v2[0x10] == 0x5A && v1[len - 1] == 0xA5,
              "the two views alias the same memory");
    }

    /* 4. Release. Done before 3 so 3 starts from a known free address. */
    check(v1 && UnmapViewOfFile(v1), "UnmapViewOfFile releases the fixed view");
    unsigned char *again = MapViewOfFileEx(map, FILE_MAP_ALL_ACCESS, 0, 0, len, at);
    check(again == at, "the address can be mapped again after UnmapViewOfFile");
    if (again) UnmapViewOfFile(again);
    if (v2) UnmapViewOfFile(v2);

    /* 3. No residue. An offset that is not page aligned makes the mmap itself
     *    fail, after the range has been checked and, on Darwin, claimed. */
    void *bad = MapViewOfFileEx(map, FILE_MAP_ALL_ACCESS, 0, 1, len / 2, at);
    check(bad == NULL, "MapViewOfFileEx with an unaligned offset fails");
    unsigned char *after = MapViewOfFileEx(map, FILE_MAP_ALL_ACCESS, 0, 0, len, at);
    check(after == at, "...and leaves the address free for the next view");
    if (after) UnmapViewOfFile(after);

    a = VirtualAlloc(at, len, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    check(a == at, "VirtualAlloc maps at a free address");
    check(a && VirtualFree(a, 0, MEM_RELEASE), "VirtualFree(MEM_RELEASE) releases it");
    a = VirtualAlloc(at, len, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    check(a == at, "the address can be allocated again after VirtualFree");
    if (a) VirtualFree(a, 0, MEM_RELEASE);

    /* 5. Partial release from a large guest-window reservation. The Xbox
     *    memory layout reserves a large PROT_NONE span, then carves RAM,
     *    mirrors and device windows out of it. Releasing the first slice must
     *    preserve the remaining reservation, and interior slices must remain
     *    releasable after the registry entry has moved forward. */
    {
        const size_t page = (size_t)sysconf(_SC_PAGESIZE);
        const size_t span_len = 16 * page;
        unsigned char *span = VirtualAlloc(NULL, span_len, MEM_RESERVE, PAGE_NOACCESS);
        check(span != NULL, "VirtualAlloc reserves a guest-window span");
        if (span) {
            check(VirtualFree(span, 4 * page, MEM_RELEASE),
                  "VirtualFree releases the first slice of a reserved span");
            unsigned char *first = MapViewOfFileEx(
                map, FILE_MAP_ALL_ACCESS, 0, 0, 4 * page, span);
            check(first == span, "a file view replaces the released first slice");

            unsigned char *middle_addr = span + 8 * page;
            check(VirtualFree(middle_addr, 4 * page, MEM_RELEASE),
                  "VirtualFree releases an interior slice after the first release");
            unsigned char *middle = MapViewOfFileEx(
                map, FILE_MAP_ALL_ACCESS, 0, 0, 4 * page, middle_addr);
            check(middle == middle_addr, "a file view replaces the released interior slice");

            if (first) UnmapViewOfFile(first);
            if (middle) UnmapViewOfFile(middle);
            check(VirtualFree(span, span_len, MEM_RELEASE),
                  "VirtualFree cleans up the remaining fragmented guest span");
        }
    }

    CloseHandle(map);

    printf("\n%d failure(s)\n", failures);
    return failures ? 1 : 0;
}
