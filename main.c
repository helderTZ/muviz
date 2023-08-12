#include <stdio.h>
#include <dlfcn.h>

#include "plugin.h"

typedef State* (*plugin_init_t)(void);
typedef void   (*plugin_deinit_t)(State*);
typedef void   (*plugin_pre_reload_t)(State*, audio_callback_t);
typedef void   (*plugin_post_reload_t)(State*, audio_callback_t);
typedef void   (*draw_progress_bar_t)(State*);
typedef void   (*draw_text_t)(void);
typedef void   (*draw_freqs_t)(State*);

plugin_init_t        plugin_init = NULL;
plugin_deinit_t      plugin_deinit = NULL;
plugin_pre_reload_t  plugin_pre_reload = NULL;
plugin_post_reload_t plugin_post_reload = NULL;
draw_progress_bar_t  draw_progress_bar = NULL;
draw_text_t          draw_text = NULL;
draw_freqs_t         draw_freqs = NULL;

State *G_state = NULL;

void audio_callback(void* bufferData, unsigned int frames) {
    Frame* framesData = bufferData;
    State* state = G_state;

    for (size_t i = 0; i < frames; ++i) {
        memmove(state->in, state->in + 1, (N - 1) * sizeof(state->in[0]));
        state->in[N - 1] = (framesData[i].left + framesData[i].right)/2;
    }
}

void* load_plugin(const char* libplugin_filename) {
    void *libplugin = dlopen(libplugin_filename, RTLD_NOW);
    if (libplugin == NULL) {
        fprintf(stderr, "ERROR: could not load %s: %s\n",
            libplugin_filename, dlerror());
        exit(EXIT_FAILURE);
    }

    plugin_init = dlsym(libplugin, "plugin_init");
    if (plugin_init == NULL) {
        fprintf(stderr, "ERROR: Could not find symbol \"plugin_init\" in %s: %s\n",
            libplugin_filename, dlerror());
        exit(EXIT_FAILURE);
    }

    plugin_deinit = dlsym(libplugin, "plugin_deinit");
    if (plugin_deinit == NULL) {
        fprintf(stderr, "ERROR: Could not find symbol \"plugin_deinit\" in %s: %s\n",
            libplugin_filename, dlerror());
        exit(EXIT_FAILURE);
    }

    plugin_pre_reload = dlsym(libplugin, "plugin_pre_reload");
    if (plugin_pre_reload == NULL) {
        fprintf(stderr, "ERROR: Could not find symbol \"plugin_pre_reload\" in %s: %s\n",
            libplugin_filename, dlerror());
        exit(EXIT_FAILURE);
    }

    plugin_post_reload = dlsym(libplugin, "plugin_post_reload");
    if (plugin_post_reload == NULL) {
        fprintf(stderr, "ERROR: Could not find symbol \"plugin_post_reload\" in %s: %s\n",
            libplugin_filename, dlerror());
        exit(EXIT_FAILURE);
    }

    draw_progress_bar = dlsym(libplugin, "draw_progress_bar");
    if (draw_progress_bar == NULL) {
        fprintf(stderr, "ERROR: Could not find symbol \"draw_progress_bar\" in %s: %s\n",
            libplugin_filename, dlerror());
        exit(EXIT_FAILURE);
    }

    draw_text = dlsym(libplugin, "draw_text");
    if (draw_text == NULL) {
        fprintf(stderr, "ERROR: Could not find symbol \"draw_text\" in %s: %s\n",
            libplugin_filename, dlerror());
        exit(EXIT_FAILURE);
    }

    draw_freqs = dlsym(libplugin, "draw_freqs");
    if (draw_freqs == NULL) {
        fprintf(stderr, "ERROR: Could not find symbol \"draw_freqs\" in %s: %s\n",
            libplugin_filename, dlerror());
        exit(EXIT_FAILURE);
    }

    return libplugin;
}

void* reload_plugin(void* handle, const char* libplugin_filename) {
    dlclose(handle);
    return load_plugin(libplugin_filename);
}

int main(int argc, char** argv) {

    const char *libplugin_filename = "./libplugin.so";
    void* libplugin = load_plugin(libplugin_filename);

    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "MUVIZ");
    InitAudioDevice();
    SetTargetFPS(60);

    State *state = plugin_init();
    G_state = state;

    if (argc == 2) {
        strncpy(state->filename, argv[1], 100);
        state->music = LoadMusicStream(argv[1]);
        state->music.looping = false;
        AttachAudioStreamProcessor(state->music.stream, audio_callback);
        SetMusicVolume(state->music, 0.5);
        PlayMusicStream(state->music);
        state->time_played = 0.0f;
    }

    while (!WindowShouldClose()) {

        if (IsFileDropped()) {
            FilePathList droppedFiles = LoadDroppedFiles();
            StopMusicStream(state->music);
            UnloadMusicStream(state->music);
            state->music = LoadMusicStream(droppedFiles.paths[0]);
            strncpy(state->filename, droppedFiles.paths[0], 100);
            state->music.looping = false;
            AttachAudioStreamProcessor(state->music.stream, audio_callback);
            SetMusicVolume(state->music, 0.5);
            PlayMusicStream(state->music);
            UnloadDroppedFiles(droppedFiles);
            state->time_played = 0.0f;
        }

        if (IsMusicReady(state->music)) {

            UpdateMusicStream(state->music);

            if (IsKeyPressed(KEY_SPACE)) {
                if (IsMusicStreamPlaying(state->music)) {
                    PauseMusicStream(state->music);
                } else {
                    ResumeMusicStream(state->music);

                    // if finished, replay
                    if (GetMusicTimePlayed(state->music) == 0.0f) {
                        PlayMusicStream(state->music);
                    }
                }
            }

            if (IsKeyPressed(KEY_R)) {
                plugin_pre_reload(state, audio_callback);
                libplugin = reload_plugin(libplugin, libplugin_filename);
                if (libplugin == NULL) {
                    fprintf(stderr, "ERROR: Failed to hot-reload plugin.\n");
                    break;
                }
                plugin_post_reload(state, audio_callback);
            }

            draw_freqs(state);

        } else {

            draw_text();

        }
    }

    plugin_deinit(state);
    CloseWindow();

    return EXIT_SUCCESS;
}