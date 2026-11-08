#include "android_manual_hooks.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static void probe(MohAndroidGuest *g, void *data)
{ ++*(uint32_t *)data; g->regs.eax = 0x12345678u; }
int main(void)
{
    uint8_t mem[32] = {0}; uint32_t value = 0, calls = 0;
    MohAndroidGuest guest; MohManualHookTable table;
    assert(moh_guest_init(&guest, mem, sizeof(mem), 0x1000u));
    assert(moh_guest_write_u32(&guest, 0x1001u, 0xA1B2C3D4u));
    assert(mem[1] == 0xD4 && mem[2] == 0xC3 && mem[3] == 0xB2 && mem[4] == 0xA1);
    assert(moh_guest_read_u32(&guest, 0x1001u, &value) && value == 0xA1B2C3D4u);
    assert(!moh_guest_read_u8(&guest, 0x0FFFu, (uint8_t *)&value));
    assert(!moh_guest_read_u32(&guest, 0x101Eu, &value));
    assert(!moh_guest_read_u32(&guest, UINT32_MAX - 1u, &value));
    moh_manual_hooks_reset(&table, &calls);
    assert(moh_manual_register_patch(&table, MOH_PATCH_00196467, probe));
    assert(moh_manual_invoke_patch(&table, MOH_PATCH_00196467, &guest));
    assert(guest.regs.eax == 0x12345678u && calls == 1u);
    assert(!moh_manual_invoke_enter(&table, MOH_ENTER_0018B480, &guest));
    assert(!moh_manual_register_patch(&table, (MohManualPatchId)99, probe));
    puts("android manual hook contract tests: PASS");
    return 0;
}
