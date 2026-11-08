#ifndef MOH_ANDROID_MANUAL_HOOKS_H
#define MOH_ANDROID_MANUAL_HOOKS_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Original Xbox guest state remains x86-32 on every host architecture. */
typedef struct MohGuestRegisters {
    uint32_t eax, ecx, edx, ebx, esp, ebp, esi, edi, eflags;
} MohGuestRegisters;

typedef struct MohAndroidGuest {
    uint8_t *memory;
    size_t memory_size;
    uint32_t memory_guest_base;
    MohGuestRegisters regs;
} MohAndroidGuest;

typedef enum MohManualPatchId {
    MOH_PATCH_000137C0 = 0, MOH_PATCH_001149E0, MOH_PATCH_00114B60,
    MOH_PATCH_0011AC60, MOH_PATCH_0017B210, MOH_PATCH_00196467,
    MOH_PATCH_001AD630, MOH_MANUAL_PATCH_COUNT
} MohManualPatchId;

typedef enum MohManualEnterId {
    MOH_ENTER_0018B480 = 0, MOH_ENTER_0018E150,
    MOH_ENTER_001C68C0, MOH_ENTER_002DE0C0, MOH_MANUAL_ENTER_COUNT
} MohManualEnterId;

typedef void (*MohManualHook)(MohAndroidGuest *guest, void *user_data);
typedef struct MohManualHookTable {
    MohManualHook patches[MOH_MANUAL_PATCH_COUNT];
    MohManualHook enters[MOH_MANUAL_ENTER_COUNT];
    void *user_data;
} MohManualHookTable;

bool moh_guest_init(MohAndroidGuest *, void *, size_t, uint32_t);
bool moh_guest_read(MohAndroidGuest *, uint32_t, void *, size_t);
bool moh_guest_write(MohAndroidGuest *, uint32_t, const void *, size_t);
bool moh_guest_read_u8(MohAndroidGuest *, uint32_t, uint8_t *);
bool moh_guest_read_u32(MohAndroidGuest *, uint32_t, uint32_t *);
bool moh_guest_write_u8(MohAndroidGuest *, uint32_t, uint8_t);
bool moh_guest_write_u32(MohAndroidGuest *, uint32_t, uint32_t);
void moh_manual_hooks_reset(MohManualHookTable *, void *);
bool moh_manual_register_patch(MohManualHookTable *, MohManualPatchId, MohManualHook);
bool moh_manual_register_enter(MohManualHookTable *, MohManualEnterId, MohManualHook);
bool moh_manual_invoke_patch(const MohManualHookTable *, MohManualPatchId, MohAndroidGuest *);
bool moh_manual_invoke_enter(const MohManualHookTable *, MohManualEnterId, MohAndroidGuest *);
#ifdef __cplusplus
}
#endif
#endif
