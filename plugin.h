#ifndef INCLUDE_PLUGIN_H_
#define INCLUDE_PLUGIN_H_

#include <complex.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <raylib.h>

#define N (1<<8) // 1<<15 is close to 20kHz

#define FFT(out, in, n) fft(out, in, n, 1)

#define BACKGROUND_COLOR (Color){0x18, 0x18, 0x18, 0}

typedef struct {
    char filename[100];
    float in[N];
    float complex out[N];
    Music music;
    float time_played;
    unsigned int frame_counter;
    // Updating *out* every other frame instead of every frame
    // makes the animation a bit more smooth
    unsigned int smooth_factor;
} State;

typedef struct {
    float left;
    float right;
} Frame;

typedef void(*audio_callback_t)(void*, unsigned int);

#endif // INCLUDE_PLUGIN_H_