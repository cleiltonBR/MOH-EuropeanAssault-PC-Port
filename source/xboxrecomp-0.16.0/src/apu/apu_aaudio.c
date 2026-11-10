/*
 * Android AAudio output backend for the MCPX APU mixer.
 *
 * The APU producer submits 256-frame stereo periods. AAudio pulls from a
 * bounded single-producer/single-consumer ring on its real-time callback.
 * The callback never allocates, locks, or logs; a short queue is padded with
 * silence and counted as an underrun.
 */
#include <aaudio/AAudio.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "apu_xaudio2.h"

#define AA2_SAMPLE_RATE       48000
#define AA2_CHANNELS          2
#define AA2_BUFFER_COUNT      4
#define AA2_RING_FRAMES       16384u
#define AA2_RING_MASK         (AA2_RING_FRAMES - 1u)
#define AA2_DEFAULT_BURST     256

#if (AA2_RING_FRAMES & AA2_RING_MASK) != 0
#error "AAudio ring capacity must be a power of two"
#endif

static AAudioStream *g_stream;
static _Atomic int g_initialized;
static _Atomic uint_fast64_t g_write_pos;
static _Atomic uint_fast64_t g_read_pos;
static _Atomic uint_fast64_t g_frames_submitted;
static _Atomic uint_fast64_t g_samples_played;
static _Atomic unsigned g_underruns;
static _Atomic int g_dry = 1;
static int16_t g_ring[AA2_RING_FRAMES * AA2_CHANNELS];
static int g_buffer_samples = AA2_DEFAULT_BURST;

static aaudio_data_callback_result_t aa2_data_callback(
    AAudioStream *stream, void *user_data, void *audio_data, int32_t num_frames)
{
    int16_t *output = (int16_t *)audio_data;
    uint64_t read_pos;
    uint64_t write_pos;
    uint64_t available;
    uint64_t copy_frames;
    int32_t i;

    (void)stream;
    (void)user_data;

    if (num_frames <= 0 || !output)
        return AAUDIO_CALLBACK_RESULT_CONTINUE;

    read_pos = atomic_load_explicit(&g_read_pos, memory_order_relaxed);
    write_pos = atomic_load_explicit(&g_write_pos, memory_order_acquire);
    available = write_pos - read_pos;
    copy_frames = available < (uint64_t)num_frames
        ? available : (uint64_t)num_frames;

    for (i = 0; i < (int32_t)copy_frames; ++i) {
        uint32_t slot = (uint32_t)(read_pos + (uint64_t)i) & AA2_RING_MASK;
        output[i * AA2_CHANNELS] = g_ring[slot * AA2_CHANNELS];
        output[i * AA2_CHANNELS + 1] = g_ring[slot * AA2_CHANNELS + 1];
    }

    if (copy_frames < (uint64_t)num_frames) {
        memset(output + copy_frames * AA2_CHANNELS, 0,
               ((uint64_t)num_frames - copy_frames) *
                   AA2_CHANNELS * sizeof(int16_t));
        if (atomic_load_explicit(&g_frames_submitted, memory_order_relaxed) > 0
                && !atomic_exchange_explicit(&g_dry, 1, memory_order_relaxed)) {
            atomic_fetch_add_explicit(&g_underruns, 1, memory_order_relaxed);
        }
    } else if (copy_frames > 0) {
        atomic_store_explicit(&g_dry, 0, memory_order_relaxed);
    }

    atomic_store_explicit(&g_read_pos, read_pos + copy_frames,
                          memory_order_release);
    atomic_fetch_add_explicit(&g_samples_played, copy_frames,
                              memory_order_relaxed);
    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

int xa2_init(void)
{
    AAudioStreamBuilder *builder = NULL;
    aaudio_result_t result;
    int32_t burst;

    if (atomic_load_explicit(&g_initialized, memory_order_acquire))
        return 1;

    atomic_store(&g_write_pos, 0);
    atomic_store(&g_read_pos, 0);
    atomic_store(&g_frames_submitted, 0);
    atomic_store(&g_samples_played, 0);
    atomic_store(&g_underruns, 0);
    atomic_store(&g_dry, 1);

    result = AAudio_createStreamBuilder(&builder);
    if (result != AAUDIO_OK || !builder) {
        fprintf(stderr, "[AAudio] stream builder creation failed: %s\n",
                AAudio_convertResultToText(result));
        return 0;
    }

    AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_OUTPUT);
    AAudioStreamBuilder_setSharingMode(builder, AAUDIO_SHARING_MODE_SHARED);
    AAudioStreamBuilder_setPerformanceMode(builder, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
    AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_I16);
    AAudioStreamBuilder_setChannelCount(builder, AA2_CHANNELS);
    AAudioStreamBuilder_setSampleRate(builder, AA2_SAMPLE_RATE);
    /* Usage and content type default to MEDIA and MUSIC. The explicit
     * setters were added in Android API 28, while this backend targets API 26. */
    AAudioStreamBuilder_setDataCallback(builder, aa2_data_callback, NULL);

    result = AAudioStreamBuilder_openStream(builder, &g_stream);
    AAudioStreamBuilder_delete(builder);
    if (result != AAUDIO_OK || !g_stream) {
        fprintf(stderr, "[AAudio] opening 48 kHz stereo output failed: %s\n",
                AAudio_convertResultToText(result));
        g_stream = NULL;
        return 0;
    }

    if (AAudioStream_getFormat(g_stream) != AAUDIO_FORMAT_PCM_I16
            || AAudioStream_getChannelCount(g_stream) != AA2_CHANNELS
            || AAudioStream_getSampleRate(g_stream) != AA2_SAMPLE_RATE) {
        fprintf(stderr, "[AAudio] device did not accept 48 kHz stereo PCM16\n");
        AAudioStream_close(g_stream);
        g_stream = NULL;
        return 0;
    }

    burst = AAudioStream_getFramesPerBurst(g_stream);
    if (burst > 0)
        g_buffer_samples = burst;
    else
        g_buffer_samples = AA2_DEFAULT_BURST;

    if (g_buffer_samples < 64)
        g_buffer_samples = 64;
    if (g_buffer_samples > 4096)
        g_buffer_samples = 4096;

    /* Request four hardware bursts of buffering; the software ring remains
     * separately bounded and is what xa2_queued_samples reports. */
    AAudioStream_setBufferSizeInFrames(
        g_stream, g_buffer_samples * AA2_BUFFER_COUNT);

    result = AAudioStream_requestStart(g_stream);
    if (result != AAUDIO_OK) {
        fprintf(stderr, "[AAudio] starting output failed: %s\n",
                AAudio_convertResultToText(result));
        AAudioStream_close(g_stream);
        g_stream = NULL;
        return 0;
    }

    atomic_store_explicit(&g_initialized, 1, memory_order_release);
    fprintf(stderr, "[AAudio] output active: 48 kHz stereo PCM16, %d-frame burst, %d bursts\n",
            g_buffer_samples, AA2_BUFFER_COUNT);
    return 1;
}

void xa2_shutdown(void)
{
    AAudioStream *stream = g_stream;

    atomic_store_explicit(&g_initialized, 0, memory_order_release);
    g_stream = NULL;
    if (stream) {
        AAudioStream_requestStop(stream);
        AAudioStream_close(stream);
    }
    atomic_store(&g_write_pos, 0);
    atomic_store(&g_read_pos, 0);
    atomic_store(&g_frames_submitted, 0);
    atomic_store(&g_samples_played, 0);
    atomic_store(&g_dry, 1);
}

int xa2_is_active(void)
{
    return atomic_load_explicit(&g_initialized, memory_order_acquire);
}

/* num_samples is the number of stereo frames, matching the existing SDL and
 * XAudio2 backends and the APU's 256-frame encode period. */
int xa2_submit_samples(const int16_t *samples, int num_samples)
{
    uint64_t write_pos;
    uint64_t read_pos;
    uint64_t queued;
    int i;

    if (!xa2_is_active() || !samples || num_samples <= 0)
        return 0;

    write_pos = atomic_load_explicit(&g_write_pos, memory_order_relaxed);
    read_pos = atomic_load_explicit(&g_read_pos, memory_order_acquire);
    queued = write_pos - read_pos;
    if ((uint64_t)num_samples > AA2_RING_FRAMES - queued)
        return 0;

    for (i = 0; i < num_samples; ++i) {
        uint32_t slot = (uint32_t)(write_pos + (uint64_t)i) & AA2_RING_MASK;
        g_ring[slot * AA2_CHANNELS] = samples[i * AA2_CHANNELS];
        g_ring[slot * AA2_CHANNELS + 1] = samples[i * AA2_CHANNELS + 1];
    }

    atomic_store_explicit(&g_write_pos, write_pos + (uint64_t)num_samples,
                          memory_order_release);
    atomic_fetch_add_explicit(&g_frames_submitted, (uint64_t)num_samples,
                              memory_order_relaxed);
    atomic_store_explicit(&g_dry, 0, memory_order_relaxed);
    return 1;
}

int xa2_get_buffer_size(void)
{
    return g_buffer_samples;
}

int xa2_get_buffer_count(void)
{
    return AA2_BUFFER_COUNT;
}

int xa2_queued_samples(void)
{
    uint64_t write_pos;
    uint64_t read_pos;
    uint64_t queued;

    if (!xa2_is_active())
        return 0;
    write_pos = atomic_load_explicit(&g_write_pos, memory_order_acquire);
    read_pos = atomic_load_explicit(&g_read_pos, memory_order_acquire);
    queued = write_pos - read_pos;
    return queued > INT_MAX ? INT_MAX : (int)queued;
}

void xa2_expect_drain(void)
{
    atomic_store_explicit(&g_dry, 1, memory_order_relaxed);
}

unsigned xa2_underruns(void)
{
    return atomic_load_explicit(&g_underruns, memory_order_relaxed);
}

uint64_t xa2_samples_played(void)
{
    return atomic_load_explicit(&g_samples_played, memory_order_relaxed);
}
