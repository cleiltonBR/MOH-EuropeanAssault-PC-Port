#include "android_manual_hooks.h"
#include <string.h>

static bool guest_range(const MohAndroidGuest *g, uint32_t va, size_t size, size_t *offset)
{
    uint64_t start, end;
    if (!g || !g->memory || !offset || va < g->memory_guest_base) return false;
    start = (uint64_t)va - (uint64_t)g->memory_guest_base;
    end = start + (uint64_t)size;
    if (end < start || end > (uint64_t)g->memory_size) return false;
    *offset = (size_t)start;
    return true;
}

bool moh_guest_init(MohAndroidGuest *g, void *memory, size_t size, uint32_t base)
{
    if (!g || (!memory && size)) return false;
    g->memory = (uint8_t *)memory;
    g->memory_size = size;
    g->memory_guest_base = base;
    memset(&g->regs, 0, sizeof(g->regs));
    return true;
}
bool moh_guest_read(MohAndroidGuest *g, uint32_t va, void *out, size_t size)
{
    size_t off;
    if ((!out && size) || !guest_range(g, va, size, &off)) return false;
    if (size) memcpy(out, g->memory + off, size);
    return true;
}
bool moh_guest_write(MohAndroidGuest *g, uint32_t va, const void *value, size_t size)
{
    size_t off;
    if ((!value && size) || !guest_range(g, va, size, &off)) return false;
    if (size) memcpy(g->memory + off, value, size);
    return true;
}
bool moh_guest_read_u8(MohAndroidGuest *g, uint32_t va, uint8_t *out)
{ return moh_guest_read(g, va, out, sizeof(*out)); }

/* Xbox guest dwords are little-endian regardless of the host ABI. Decode
 * explicitly rather than relying on host byte order or alignment. */
bool moh_guest_read_u32(MohAndroidGuest *g, uint32_t va, uint32_t *out)
{
    uint8_t b[4];
    if (!out || !moh_guest_read(g, va, b, sizeof(b))) return false;
    *out = (uint32_t)b[0]
         | ((uint32_t)b[1] << 8)
         | ((uint32_t)b[2] << 16)
         | ((uint32_t)b[3] << 24);
    return true;
}
bool moh_guest_write_u8(MohAndroidGuest *g, uint32_t va, uint8_t value)
{ return moh_guest_write(g, va, &value, sizeof(value)); }
bool moh_guest_write_u32(MohAndroidGuest *g, uint32_t va, uint32_t value)
{
    const uint8_t b[4] = {
        (uint8_t)value,
        (uint8_t)(value >> 8),
        (uint8_t)(value >> 16),
        (uint8_t)(value >> 24)
    };
    return moh_guest_write(g, va, b, sizeof(b));
}

void moh_manual_hooks_reset(MohManualHookTable *t, void *data)
{ if (t) { memset(t, 0, sizeof(*t)); t->user_data = data; } }
bool moh_manual_register_patch(MohManualHookTable *t, MohManualPatchId id, MohManualHook h)
{ if (!t || (unsigned)id >= MOH_MANUAL_PATCH_COUNT) return false; t->patches[id] = h; return true; }
bool moh_manual_register_enter(MohManualHookTable *t, MohManualEnterId id, MohManualHook h)
{ if (!t || (unsigned)id >= MOH_MANUAL_ENTER_COUNT) return false; t->enters[id] = h; return true; }
bool moh_manual_invoke_patch(const MohManualHookTable *t, MohManualPatchId id, MohAndroidGuest *g)
{
    MohManualHook h;
    if (!t || !g || (unsigned)id >= MOH_MANUAL_PATCH_COUNT || !(h = t->patches[id])) return false;
    h(g, t->user_data); return true;
}
bool moh_manual_invoke_enter(const MohManualHookTable *t, MohManualEnterId id, MohAndroidGuest *g)
{
    MohManualHook h;
    if (!t || !g || (unsigned)id >= MOH_MANUAL_ENTER_COUNT || !(h = t->enters[id])) return false;
    h(g, t->user_data); return true;
}
