#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define GUEST_BASE 0x00010000u
#define GUEST_SIZE 0x00400000u
#define DEVICE_PTR 0x002E9548u
#define DEVICE_VA  0x00100000u
#define SEMAPHORE_VA 0x00110000u
#define BUSY_EVENT 0x1DCCu

static uint8_t guest_memory[GUEST_SIZE];
static uint32_t registered_semaphore;
static uint32_t signaled_event;
static void (*registered_swmethod)(uint32_t);

__thread uint32_t g_esp;

ptrdiff_t xbox_GetMemoryOffset(void)
{
    return (ptrdiff_t)(uintptr_t)guest_memory - (ptrdiff_t)GUEST_BASE;
}

int xbox_IsXboxAddress(uintptr_t address)
{
    return address >= GUEST_BASE
        && address < (uintptr_t)GUEST_BASE + GUEST_SIZE;
}

void nv2a_pb_set_semaphore_target(uint32_t guest_va)
{
    registered_semaphore = guest_va;
}

void nv2a_pb_set_swmethod_handler(void (*fn)(uint32_t param))
{
    registered_swmethod = fn;
}

void xbox_KeSetEventGuest(uint32_t guest_va)
{
    signaled_event = guest_va;
}

void sub_0018B480_enter(void);
void sub_0018E150_enter(void);
void sub_001C68C0_enter(void);
void sub_002DE0C0_enter(void);

static void guest_write_u32(uint32_t va, uint32_t value)
{
    assert(xbox_IsXboxAddress(va));
    assert(xbox_IsXboxAddress((uintptr_t)va + 3u));
    memcpy(guest_memory + (va - GUEST_BASE), &value, sizeof(value));
}

int main(void)
{
    memset(guest_memory, 0, sizeof(guest_memory));

    /* The GPU hook must safely wait until the guest device is initialized. */
    sub_002DE0C0_enter();
    assert(registered_semaphore == 0);
    assert(registered_swmethod == NULL);

    /* Reject a corrupt device pointer, then retry once the guest is valid. */
    guest_write_u32(DEVICE_PTR, UINT32_MAX - 0x10u);
    sub_002DE0C0_enter();
    assert(registered_semaphore == 0);
    assert(registered_swmethod == NULL);

    guest_write_u32(DEVICE_PTR, DEVICE_VA);
    guest_write_u32(DEVICE_VA + 0x30u, SEMAPHORE_VA);
    sub_002DE0C0_enter();
    assert(registered_semaphore == SEMAPHORE_VA);
    assert(registered_swmethod != NULL);

    /* Only software method 5 is the D3D busy-event notification. */
    registered_swmethod(4u);
    assert(signaled_event == 0);
    registered_swmethod(5u);
    assert(signaled_event == DEVICE_VA + BUSY_EVENT);

    /* Android intentionally leaves the three optional desktop FOV hooks inert. */
    sub_0018B480_enter();
    sub_0018E150_enter();
    sub_001C68C0_enter();

    return 0;
}
