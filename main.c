#include <complex.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <raylib.h>

#define N (1<<15) // close to 20kHz
float in[N];
float complex out[N];

typedef struct {
    float left;
    float right;
} Frame;

#define FFT(out, in, n) fft(out, in, n, 1)

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

void audio_callback(void* bufferData, unsigned int frames) {

    Frame* framesData = bufferData;

    for (size_t i = 0; i < frames; ++i) {
        memmove(in, in + 1, (N - 1) * sizeof(in[0]));
        in[N - 1] = framesData[i].left;
    }
}

float amplitude(float complex c) {
    return sqrt(creal(c)*creal(c) + cimag(c)*cimag(c));
}

float max_amplitude(float complex cs[], size_t n) {
    float max_amp = 0.0f;
    for (size_t i = 0; i < n; ++i) {
        float amp = amplitude(cs[i]);
        if (max_amp < amp) max_amp = amp;
    }

    return max_amp;
}

void draw_freqs() {

    int w = GetScreenWidth();
    int h = GetScreenHeight();

    BeginDrawing();
        ClearBackground((Color){0x18, 0x18, 0x18, 0});

        FFT(out, in, N);
        // fftshift(out, N);

        // calculate number of bins to have a logarithmic freq scale
        float step = 1.059463094359; // from: https://pages.mtu.edu/~suits/NoteFreqCalcs.html
        size_t m = 0; // number of bins
        for (float f = 20.0f; (size_t)f < N; f *= step) {
            m++;
        }

        float cell_w = (float)w/m;
        float max_amp = max_amplitude(out, N);

        m = 0;
        for (float f = 20.0f; (size_t)f < N; f *= step) {
            float f1 = f*step;
            float acc = 0.0f;
            // accumulate the freqs in the bin (between current f and f*step)
            for (size_t q = (size_t) f; q < N && q < (size_t) f1; ++q) {
                acc += amplitude(out[q]);
            }
            acc /= (size_t)f1 - (size_t)f + 1;
            float t = acc/max_amp;
            DrawRectangle(m*cell_w, h/2 - h/2*t, cell_w, h/2*t, RED);
            DrawRectangle(m*cell_w, h/2        , cell_w, h/2*t, RED);
            m++;
        }
    EndDrawing();
}

int main(int argc, char** argv) {

    const int screenWidth = 800;
    const int scrrenHeight = 450;

    InitWindow(screenWidth, scrrenHeight, "MUVIZ");
    InitAudioDevice();

    Music music = {0};

    if (argc == 2) {
        music = LoadMusicStream(argv[1]);
        music.looping = false;
        AttachAudioStreamProcessor(music.stream, audio_callback);
        SetMusicVolume(music, 0.5);
        PlayMusicStream(music);
    }

    SetTargetFPS(60);

    while (!WindowShouldClose()) {

        if (IsFileDropped()) {
            FilePathList droppedFiles = LoadDroppedFiles();
            StopMusicStream(music);
            UnloadMusicStream(music);
            music = LoadMusicStream(droppedFiles.paths[0]);
            music.looping = false;
            AttachAudioStreamProcessor(music.stream, audio_callback);
            SetMusicVolume(music, 0.5);
            PlayMusicStream(music);
            UnloadDroppedFiles(droppedFiles);
        }

        if (IsMusicReady(music)) {

            UpdateMusicStream(music);

            if (IsKeyPressed(KEY_SPACE)) {
                if (IsMusicStreamPlaying(music)) {
                    PauseMusicStream(music);
                } else {
                    ResumeMusicStream(music);
                }
            }

            draw_freqs();

        } else {

            int w = GetScreenWidth();
            int h = GetScreenHeight();

            BeginDrawing();
                ClearBackground((Color){0x18, 0x18, 0x18, 0});
                const char* text = "No music file supplied.";
                const int fontSize = 40;
                int textWidth = MeasureText(text, fontSize);
                DrawText(text, w/2 - textWidth/2, h/3, fontSize, GRAY);
                text = "Drag and drop to start playing.";
                textWidth = MeasureText(text, fontSize);
                DrawText(text, w/2 - textWidth/2, h/3 + fontSize, fontSize, GRAY);
            EndDrawing();
        }
    }

    UnloadMusicStream(music);
    CloseAudioDevice();
    CloseWindow();

    return 0;
}