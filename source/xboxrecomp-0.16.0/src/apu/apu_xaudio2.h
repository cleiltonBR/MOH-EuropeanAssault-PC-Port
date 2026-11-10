/**
 * Audio output backend contract shared by XAudio2, SDL2 and Android AAudio.
 */
#ifndef APU_XAUDIO2_H
#define APU_XAUDIO2_H

#include <stdint.h>

#if defined(_WIN32)
#define XA2_BACKEND_NAME "XAudio2"
#elif defined(__ANDROID__)
#define XA2_BACKEND_NAME "AAudio"
#else
#define XA2_BACKEND_NAME "SDL2"
#endif

/* Initialize the device backend. Returns 1 on success, 0 on failure. */
int xa2_init(void);
void xa2_shutdown(void);
int xa2_is_active(void);

/* Submit interleaved stereo 16-bit frames. Returns 1 if accepted. */
int xa2_submit_samples(const int16_t *samples, int num_samples);
int xa2_get_buffer_size(void);
int xa2_get_buffer_count(void);
int xa2_queued_samples(void);
void xa2_expect_drain(void);

/* Pacing marks in frames for count buffers of buf_samples, fed one period
 * (period frames) at a time. */
static inline void xa2_pacing_marks(int buf_samples, int count, int period,
                                    int *low, int *high)
{
    int h = buf_samples * count;
    int cap = buf_samples * (count + 1) - period;
    if (h > cap) h = cap;
    if (h < 1) h = 1;
    *high = h;
    *low = h / 2;
}

unsigned xa2_underruns(void);
uint64_t xa2_samples_played(void);

#endif /* APU_XAUDIO2_H */
