#include <stdio.h>
#include <unistd.h>
#include "support.h"


/*
 * Wave generation parameters for malloc calls.
 *
 * size - allocated memory size (input)
 * parameter 1 - wave frequency in Hertz; should be clamped to a sane range
 *   (e. g. 20-20000 Hz) with CLAMP_u(freq, min, max)
 * parameter 2 - wave duration in milliseconds
 * parameter 3 - wave amplitude (0.0-1.0)
 */
#define MALLOC_WAVE_PARAMS(size) \
  CLAMP_u(size, 20u, 10000u), 20, 0.7f

#define MALLOC_WAVE_TICKS_PARAMS(ticks) \
  CLAMP_u(ticks, 20u, 20000u), 10, 0.7f

#define READ_WAVE_PARAMS(requested, returned) \
  CLAMP_u(requested, 20u, 20000u), CLAMP_u(returned, 100, 1700), 0.7f


// foward declarations ================================================

void* malloc(size_t size);

ssize_t read(int fd, void * data, size_t count);

static void gen_square_wave(unsigned frequency, unsigned duration, float amplitude);

static void __gen_square_wave_impl(unsigned samples, unsigned tone_midpoint, sample_t sample);


// implementations ====================================================

void* malloc(size_t size)
{
  static volatile bool inside_malloc = false;

  void *p = __malloc_data.real_malloc(size);

  if (!__malloc_data.nomalloc && !inside_malloc)
  {
    inside_malloc = true;

    clock_t ticks_now = clock();
    if (ticks_now != (clock_t) -1) {
      gen_square_wave(MALLOC_WAVE_TICKS_PARAMS(ticks_now - __malloc_data.ticks_start));
    }

    gen_square_wave(MALLOC_WAVE_PARAMS(size));
    inside_malloc = false;
  }

  return p;
}


ssize_t read(int fd, void * data, size_t count)
{
  ssize_t p = __read_data.real_read(fd, data, count);
  if (!__read_data.noread) {
    gen_square_wave(READ_WAVE_PARAMS(count, p));
  }
  return p;
}


static inline
void gen_square_wave(unsigned frequency, unsigned duration, float amplitude)
{
  assert(amplitude >= 0.f && amplitude <= 1.f);

  __gen_square_wave_impl(
    SAMPLING_RATE * duration / 1000,
    MAX(SAMPLING_RATE / 2 / frequency, 1u),
    (sample_t)((float) -(1 << (13 - 1)) * amplitude));
}


static void __gen_square_wave_impl(unsigned samples, unsigned tone_midpoint, sample_t sample)
{
  assert(__wave_out != NULL);
  assert(samples == 0 || tone_midpoint != 0);

  for (unsigned i = 0; i != samples; i++)
  {
    if(i % tone_midpoint == 0)
      sample = (sample_t) -sample;

    fwrite(&sample, sizeof(sample), 1, __wave_out);
  }
  fflush(__wave_out);
}
