/*
 * Android framebuffer adapter.
 *
 * The game is displayed by the main EGL/GLES surface owned by native_entry.
 * This adapter preserves the guest framebuffer capture path used by the
 * pushbuffer executor without creating a second SDL/Win32 window. A requested
 * capture is copied from guest memory on the render thread, where the guest
 * surface is stable, and can be written as a BMP for diagnostics.
 */
#include "video_player.h"
#include "platform/xbox_winnt.h"

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define FB_W 640u
#define FB_H 480u

extern ptrdiff_t xbox_GetMemoryOffset(void);

/* The pushbuffer executor's F9 capture request is shared with this adapter. */
volatile LONG g_fb_capture_request = 0;

static uint32_t *s_pixels;
static uint32_t s_fb_va;
static uint32_t s_fb_pitch;
static uint32_t s_aa_sy = 1;
static int s_capture_enabled;
static uint64_t s_flips;
static uint32_t s_frame_draws;
static uint64_t s_gpu_shown_ms;
static uint16_t s_title[48];

static int env_enabled(const char *name)
{
    const char *v = getenv(name);
    return v && *v && *v != '0';
}

static uint64_t now_ms(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
        return 0;
    return (uint64_t)ts.tv_sec * 1000ull + (uint64_t)ts.tv_nsec / 1000000ull;
}

void xbox_FramebufferWindowSet(uint32_t fb_va, uint32_t pitch)
{
    s_fb_va = fb_va;
    if (pitch)
        s_fb_pitch = pitch;
}

void xbox_FramebufferWindowSetAA(uint32_t sy)
{
    s_aa_sy = sy ? sy : 1;
}

void xbox_FramebufferWindowStart(void)
{
    /* Android has one activity-owned display surface, not a second desktop
     * framebuffer window. Enable the real guest-memory capture path only
     * when a capture/debug mode is requested. */
    s_capture_enabled = env_enabled("RECOMP_FB_WINDOW")
                     || env_enabled("RECOMP_FB_DUMP_FLIPS")
                     || env_enabled("RECOMP_FB_CAPTURE");
}

static int capture_guest_surface(uint32_t fb_va, uint32_t pitch)
{
    const uint8_t *src;
    uint32_t bpp, x, y;
    const size_t count = (size_t)FB_W * FB_H;

    if (!fb_va || !pitch)
        return -1;
    bpp = pitch / FB_W;
    if (bpp != 2 && bpp != 4 && bpp != 8)
        return -1;

    if (!s_pixels) {
        s_pixels = (uint32_t *)malloc(count * sizeof(*s_pixels));
        if (!s_pixels)
            return -1;
    }

    src = (const uint8_t *)((uintptr_t)fb_va + xbox_GetMemoryOffset());
    for (y = 0; y < FB_H; y++) {
        const uint8_t *row = src + (size_t)y * pitch * (bpp == 8 ? s_aa_sy : 1);
        uint32_t *dst = s_pixels + (size_t)y * FB_W;

        if (bpp == 4) {
            const uint32_t *p = (const uint32_t *)row;
            for (x = 0; x < FB_W; x++)
                dst[x] = 0xFF000000u | (p[x] & 0x00FFFFFFu);
        } else if (bpp == 8) {
            const uint32_t *p = (const uint32_t *)row;
            for (x = 0; x < FB_W; x++)
                dst[x] = 0xFF000000u | (p[x * 2] & 0x00FFFFFFu);
        } else {
            const uint16_t *p = (const uint16_t *)row;
            for (x = 0; x < FB_W; x++) {
                const uint16_t v = p[x];
                const uint32_t r = ((v >> 11) & 0x1Fu) * 255u / 31u;
                const uint32_t g = ((v >> 5) & 0x3Fu) * 255u / 63u;
                const uint32_t b = (v & 0x1Fu) * 255u / 31u;
                dst[x] = 0xFF000000u | (r << 16) | (g << 8) | b;
            }
        }
    }
    return 0;
}

void xbox_FramebufferWindowPresent(uint32_t fb_va, uint32_t pitch)
{
    if (!s_capture_enabled)
        return;
    if (!fb_va)
        fb_va = s_fb_va;
    if (!pitch)
        pitch = s_fb_pitch;
    (void)capture_guest_surface(fb_va, pitch);
}

int xbox_FramebufferDumpBmp(const char *path)
{
    uint8_t header[54] = {0};
    uint8_t *row;
    const uint32_t row_bytes = FB_W * 3u;
    const uint32_t padded = (row_bytes + 3u) & ~3u;
    const uint32_t image_bytes = padded * FB_H;
    FILE *f;

    if (!path || !*path || !s_pixels)
        return -1;
    f = fopen(path, "wb");
    if (!f)
        return -1;

    header[0] = 'B'; header[1] = 'M';
    header[2] = (uint8_t)(54u + image_bytes);
    header[3] = (uint8_t)((54u + image_bytes) >> 8);
    header[4] = (uint8_t)((54u + image_bytes) >> 16);
    header[5] = (uint8_t)((54u + image_bytes) >> 24);
    header[10] = 54;
    header[14] = 40;
    header[18] = (uint8_t)FB_W;
    header[19] = (uint8_t)(FB_W >> 8);
    header[20] = (uint8_t)(FB_W >> 16);
    header[21] = (uint8_t)(FB_W >> 24);
    header[22] = (uint8_t)FB_H;
    header[23] = (uint8_t)(FB_H >> 8);
    header[24] = (uint8_t)(FB_H >> 16);
    header[25] = (uint8_t)(FB_H >> 24);
    header[26] = 1;
    header[28] = 24;
    header[34] = (uint8_t)image_bytes;
    header[35] = (uint8_t)(image_bytes >> 8);
    header[36] = (uint8_t)(image_bytes >> 16);
    header[37] = (uint8_t)(image_bytes >> 24);

    row = (uint8_t *)calloc(1, padded);
    if (!row) {
        fclose(f);
        return -1;
    }
    if (fwrite(header, 1, sizeof(header), f) != sizeof(header)) {
        free(row);
        fclose(f);
        return -1;
    }
    for (uint32_t y = 0; y < FB_H; y++) {
        const uint32_t *src = s_pixels + (size_t)(FB_H - 1u - y) * FB_W;
        for (uint32_t x = 0; x < FB_W; x++) {
            const uint32_t px = src[x];
            row[x * 3u + 0u] = (uint8_t)px;
            row[x * 3u + 1u] = (uint8_t)(px >> 8);
            row[x * 3u + 2u] = (uint8_t)(px >> 16);
        }
        if (fwrite(row, 1, padded, f) != padded) {
            free(row);
            fclose(f);
            return -1;
        }
    }
    free(row);
    if (fclose(f) != 0)
        return -1;
    return 0;
}

void xbox_FramebufferWindowSetTitle(const uint16_t *name, int max_chars)
{
    int i;
    if (!name || max_chars <= 0)
        return;
    for (i = 0; i < max_chars && i < 47 && name[i]; i++)
        s_title[i] = name[i];
    s_title[i] = 0;
}

void xbox_FramebufferWindowFrameStats(uint32_t draws)
{
    __atomic_add_fetch(&s_flips, 1, __ATOMIC_RELAXED);
    __atomic_store_n(&s_frame_draws, draws, __ATOMIC_RELAXED);
}

/* Android has no Win32 HWND or secondary GPU-present window. The main game
 * image is presented by the D3D8/GLES device on the activity's EGL surface. */
void *xbox_FramebufferWindowHwnd(void)
{
    return NULL;
}

void xbox_FramebufferWindowLogicalSize(uint32_t *w, uint32_t *h)
{
    if (w) *w = FB_W;
    if (h) *h = FB_H;
}

void xbox_FramebufferWindowGpuShown(void)
{
    s_gpu_shown_ms = now_ms();
}

int xbox_FramebufferWindowPvideoActive(void)
{
    /* There is no secondary GPU window to suppress while PVIDEO is active. */
    return 0;
}

/* The Android activity already owns the render thread and its lifecycle. */
int xbox_HostWindowMain(int (*guest_main)(void))
{
    return guest_main ? guest_main() : -1;
}
