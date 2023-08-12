#include "plugin.h"

// Updating *out* every other frame instead of every frame
// makes the animation a bit more smooth
#define SMOOTH_FACTOR 1

// Factor applied to the height of the frequency bars
// 1 means height of the window
// 2 means half the window, and so on
#define SCALE_FACTOR 3

void fft(float complex out[], float in[], size_t n, size_t stride);
void fftshift(float complex z[], size_t n);
float amplitude(float complex c);
float max_amplitude(float complex cs[], size_t n);
float get_time_played(Music music);
void draw_text();
void draw_progress_bar(State *state);
void draw_freqs(State *state);

State* plugin_init();
void plugin_deinit(State* state);

void fft(float complex out[], float in[], size_t n, size_t stride) {
    if (n <= 1) {
        out[0] = in[0];
        return;
    }

    fft(out,       in,          n/2, stride*2);
    fft(out + n/2, in + stride, n/2, stride*2);

    for (size_t k = 0; k < n/2; ++k) {
        float t = (float)k/n;
        float complex v = cexp(-2*I*PI*t) * out[k + n/2];
        float complex e = out[k];
        out[k      ] = e + v;
        out[k + n/2] = e - v;
    }
}

void fftshift(float complex z[], size_t n) {
    for (size_t i = 0; i < n/2; ++i) {
        float complex temp = z[i];
        z[i      ] = z[i + n/2];
        z[i + n/2] = temp;
    }
}

void hann(float out[], float in[], size_t n) {
    for (size_t i = 0; i < n; ++i) {
        float t = (float)i/n-1;
        float hann = 0.5 - 0.5*cosf(2*PI*t);
        out[i] = in[i] * hann;
    }
}

float amplitude(float complex c) {
    float a = creal(c);
    float b = cimag(c);
    // return sqrt(a*a + b*b);
    // return (a*a + b*b);
    return logf(sqrt(a*a + b*b));
}

float max_amplitude(float complex cs[], size_t n) {
    float max_amp = 0.0f;
    for (size_t i = 0; i < n; ++i) {
        float amp = amplitude(cs[i]);
        if (max_amp < amp) max_amp = amp;
    }

    return max_amp;
}

float get_time_played(Music music) {
    return GetMusicTimePlayed(music)/GetMusicTimeLength(music);
}

void draw_text() {

    int w = GetScreenWidth();
    int h = GetScreenHeight();

    BeginDrawing();
        ClearBackground((Color){0x18, 0x18, 0x18, 0});
        const char* text = "No music file supplied.";
        const int font_size = 40;
        int text_width = MeasureText(text, font_size);
        DrawText(text, w/2 - text_width/2, h/3, font_size, GRAY);
        text = "Drag and drop to start playing.";
        text_width = MeasureText(text, font_size);
        DrawText(text, w/2 - text_width/2, h/3 + font_size, font_size, GRAY);
    EndDrawing();
}

void draw_progress_bar(State *state) {

    int w = GetScreenWidth();
    int h = GetScreenHeight();

    state->time_played = get_time_played(state->music);

    if (state->time_played > 1.0f) {
        state->time_played = 1.0f;
    }

    float bar_w = w/4*2;
    float bar_h = 20;
    float bar_x = w/4;
    float bar_y = h/8*7;
    DrawRectangle(bar_x, bar_y, bar_w, bar_h, LIGHTGRAY);
    DrawRectangle(bar_x, bar_y, (int)(state->time_played*bar_w), bar_h, MAROON);
    DrawRectangleLines(bar_x, bar_y, bar_w, bar_h, GRAY);

    const int font_size = 20;
    const char *text = "Now playing";
    int text_w = MeasureText(text, font_size);
    int filename_w = MeasureText(state->filename, font_size);
    int text_x = w/2 - text_w/2;
    int text_y = bar_y - bar_h - font_size - 20;
    int filename_x = w/2 - filename_w/2 < 0 ? 0 : w/2 - filename_w/2;
    int filename_y = bar_y - bar_h - 10;
    DrawText(text,            text_x,     text_y,     font_size, GRAY);
    DrawText(state->filename, filename_x, filename_y, font_size, GRAY);
}

void DrawSimRect(int posX, int posY, int width, int height, Color color) {
    DrawRectangle(posX, posY - height, width, height, color);
    DrawRectangle(posX, posY         , width, height, color);
}

void draw_freqs(State *state) {

    int w = GetScreenWidth();
    int h = GetScreenHeight();

    BeginDrawing();
        ClearBackground(BACKGROUND_COLOR);

        if (state->frame_counter++ % state->smooth_factor == 0) {
            // apply Hann window to remove phantom frequencies
            hann(state->in_windowed, state->in, N);
            FFT(state->out, state->in_windowed, N);
            // fftshift(state->out, N);
        }

        // calculate number of bins to have a logarithmic freq scale
        float step = 1.059463094359; // from: https://pages.mtu.edu/~suits/NoteFreqCalcs.html
        float lowf = 1.0f;
        size_t m = 0; // number of bins
        for (float f = lowf; (size_t)f < N/2; f = ceil(f*step)) {
            m++;
        }

        float cell_w = (float)w/m > 1 ? (float)w/m : 1;
        float max_amp = max_amplitude(state->out, N);

        m = 0;
        for (float f = lowf; (size_t)f < N/2; f = ceil(f*step)) {
            float f1 = ceil(f*step);
            float a = 0.0f;
            // accumulate the freqs in the bin (between current f and f*step)
            for (size_t q = (size_t) f; q < N/2 && q < (size_t) f1; ++q) {
                float b = amplitude(state->out[q]);
                if (a < b) a = b;
            }
            float t = a/max_amp/SCALE_FACTOR;
            float cell_h = h/2*t > 1 ? h/2*t : 1;
            DrawSimRect(m*cell_w, h/2, cell_w, cell_h, BLUE);
            // DrawCircle(m*cell_w, h/2, cell_h, RED);
            m++;
        }

        draw_progress_bar(state);

    EndDrawing();
}

State* plugin_init() {
    State *state = (State*)calloc(1, sizeof(State));
    state->smooth_factor = SMOOTH_FACTOR;

    return state;
}

void plugin_pre_reload(State* state, audio_callback_t audio_callback) {
    PauseMusicStream(state->music);
    DetachAudioStreamProcessor(state->music.stream, audio_callback);
}

void plugin_post_reload(State* state, audio_callback_t audio_callback) {
    AttachAudioStreamProcessor(state->music.stream, audio_callback);
    state->smooth_factor = SMOOTH_FACTOR;
    PlayMusicStream(state->music);
}

void plugin_deinit(State* state) {
    UnloadMusicStream(state->music);
    free(state);
}