/*
Copyright (c) 2025 Rupert Carmichael

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <SDL.h>
#include <speex/speex_resampler.h>

#include <jollycv.h>

#define SCREEN_WIDTH JCV_VIDEO_WIDTH_MAX
#define SCREEN_HEIGHT JCV_VIDEO_HEIGHT_MAX
#define SAMPLERATE 48000
#define FRAMERATE 60 // Approximately 60Hz
#define CHANNELS 1

// Speex Resampler
static SpeexResamplerState *resampler = NULL;
static int err;

// SDL Audio
static SDL_AudioSpec spec, obtained;
static SDL_AudioDeviceID dev;

// SDL Video
static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *texture;

// ROM data
static uint8_t *romdata = NULL;
static size_t romsize = 0;

// Video rendering
static uint32_t *vbuf = NULL;
static unsigned scale = 4;

// Audio buffer
static int16_t abuf[1600];
static int16_t rsbuf[1600];

// Button state
unsigned buttons[18] = {0};

// Keep track of whether the emulator should be running or not
static int running = 1;

// Handle SDL input events
static void jcv_input_handler(SDL_Event event) {
    if (event.type == SDL_KEYUP || event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.scancode) {
            case SDL_SCANCODE_ESCAPE:
                running = 0;
                break;

            case SDL_SCANCODE_UP:
                buttons[0] = event.type == SDL_KEYDOWN;
                break;
            case SDL_SCANCODE_DOWN:
                buttons[1] = event.type == SDL_KEYDOWN;
                break;
            case SDL_SCANCODE_LEFT:
                buttons[2] = event.type == SDL_KEYDOWN;
                break;
            case SDL_SCANCODE_RIGHT:
                buttons[3] = event.type == SDL_KEYDOWN;
                break;

            case SDL_SCANCODE_A:
                buttons[4] = event.type == SDL_KEYDOWN;
                break;
            case SDL_SCANCODE_Z:
                buttons[5] = event.type == SDL_KEYDOWN;
                break;

            case SDL_SCANCODE_1:
                buttons[6] = event.type == SDL_KEYDOWN;
                break;
            case SDL_SCANCODE_2:
                buttons[7] = event.type == SDL_KEYDOWN;
                break;
            case SDL_SCANCODE_3:
                buttons[8] = event.type == SDL_KEYDOWN;
                break;
            case SDL_SCANCODE_4:
                buttons[9] = event.type == SDL_KEYDOWN;
                break;
            case SDL_SCANCODE_5:
                buttons[10] = event.type == SDL_KEYDOWN;
                break;
            case SDL_SCANCODE_6:
                buttons[11] = event.type == SDL_KEYDOWN;
                break;
            case SDL_SCANCODE_7:
                buttons[12] = event.type == SDL_KEYDOWN;
                break;
            case SDL_SCANCODE_8:
                buttons[13] = event.type == SDL_KEYDOWN;
                break;
            case SDL_SCANCODE_9:
                buttons[14] = event.type == SDL_KEYDOWN;
                break;
            case SDL_SCANCODE_0:
                buttons[15] = event.type == SDL_KEYDOWN;
                break;
            case SDL_SCANCODE_BACKSPACE:
                buttons[16] = event.type == SDL_KEYDOWN;
                break;
            case SDL_SCANCODE_RETURN:
                buttons[17] = event.type == SDL_KEYDOWN;
                break;
            default: break;
        }
    }
}

static int jcv_rom_load_file(const char *filename) {
    FILE *file;
    size_t filesize, result;

    file = fopen(filename, "rb");
    if (!file)
        return 0;

    fseek(file, 0, SEEK_END);
    filesize = ftell(file);
    fseek(file, 0, SEEK_SET);

    romdata = (uint8_t*)calloc(1, filesize);

    result = fread(romdata, sizeof(uint8_t), filesize, file);
    if (result != filesize) {
        fclose(file);
        free(romdata);
        return 0;
    }

    fclose(file);
    romsize = filesize;
    return 1;
}

static unsigned jcv_coleco_input_poll(const void *udata, int port) {
    (void)udata;
    unsigned b = 0;
    if (port == 0) {
        for (unsigned i = 0; i < 18; ++i)
            if (buttons[i]) b |= (1 << i);
    }
    return b;
}

static void jcv_cb_audio(const void *udata, size_t samps) {
    (void)udata;
    if (!samps)
        return;

    // Manage audio output queue size by resampling
    uint32_t qsamps = SDL_GetQueuedAudioSize(dev);
    unsigned esamps = 4; // Up to 4 extra samples

    if (qsamps > SAMPLERATE)
        SDL_ClearQueuedAudio(dev);
    else if (qsamps < 3200)
        esamps = (qsamps / 800);

    esamps = 4 - esamps;

    err = speex_resampler_set_rate_frac(resampler,
        SAMPLERATE, SAMPLERATE + (esamps * FRAMERATE),
        SAMPLERATE, SAMPLERATE + (esamps * FRAMERATE));

    uint32_t insamps = samps;
    uint32_t outsamps = samps + esamps;
    err = speex_resampler_process_int(resampler, 0,
        abuf, &insamps,
        rsbuf, &outsamps);

    SDL_QueueAudio(dev, rsbuf, outsamps << 1); // Sample number is in bytes
}

int main (int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: ./%s [FILE]\n", argv[0]);
        exit(1);
    }

    // Force DirectSound audio driver on Windows
#if defined(_WIN32) || defined(__MINGW32__) || defined(__MINGW64__)
    putenv("SDL_AUDIODRIVER=directsound");
#endif

    jcv_input_set_callback_coleco(jcv_coleco_input_poll, NULL);
    jcv_audio_set_callback(jcv_cb_audio, NULL);
    jcv_audio_set_rate(SAMPLERATE);
    jcv_audio_set_rsqual(2);
    jcv_audio_set_buffer(abuf);
    jcv_set_system(JCV_SYS_COLECO);
    jcv_set_region(JCV_REGION_NTSC);

    jcv_init();

    jcv_audio_set_rate(SAMPLERATE);

    if (!jcv_bios_load_file("coleco.rom")) {
        fprintf(stderr, "Failed to load BIOS at: coleco.rom\n");
        return 1;
    }

    if (jcv_rom_load_file(argv[argc - 1])) {
        jcv_media_load(romdata, romsize);
    }
    else {
        fprintf(stderr, "Failed to load ROM at: %s\n", argv[argc - 1]);
        return 1;
    }

    jcv_reset(2); // First boot reset

    // Allow joystick input when the window is not focused
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");

    // Keep window fullscreen if the window manager tries to iconify it
    SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0) {
        fprintf(stderr, "Failed to initialize SDL: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    // Set up the window
    Uint32 windowflags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE |
        SDL_WINDOW_ALLOW_HIGHDPI;

    int windowwidth = SCREEN_WIDTH * scale;
    int windowheight = SCREEN_HEIGHT * scale;

    window = SDL_CreateWindow("libjollycv-example",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        windowwidth, windowheight, windowflags);

    renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_SetRenderDrawColor(renderer, 0x0, 0x0, 0x0, 0xff);
    SDL_RenderSetLogicalSize(renderer, windowwidth, windowheight);

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);

    SDL_ShowCursor(false);

    // Get current display mode
    SDL_DisplayMode dm;
    SDL_GetCurrentDisplayMode(SDL_GetWindowDisplayIndex(window), &dm);

    // Allocate video buffer
    vbuf =
        (uint32_t*)calloc(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint32_t), 1);
    jcv_video_set_buffer(vbuf);

    // Set up SDL Audio
    spec.channels = CHANNELS;
    spec.freq = SAMPLERATE;
    spec.silence = 0;
    spec.samples = 512;
    spec.userdata = 0;
    #if SDL_BYTEORDER == SDL_BIG_ENDIAN
    spec.format = AUDIO_S16MSB;
    #else
    spec.format = AUDIO_S16LSB;
    #endif

    // Open the Audio Device
    dev = SDL_OpenAudioDevice(NULL, 0, &spec, &obtained,
        SDL_AUDIO_ALLOW_ANY_CHANGE);

    // Set up resampling
    resampler = speex_resampler_init(CHANNELS, SAMPLERATE, SAMPLERATE,
        0, &err);

    // Allow audio playback
    SDL_PauseAudioDevice(dev, 0);

    int runframes = 0;
    int collector = 0;

    SDL_Event event;

    while (running) {
        // Divide core framerate by screen framerate and collect remainders
        runframes = FRAMERATE / dm.refresh_rate;
        collector += FRAMERATE % dm.refresh_rate;

        if (collector >= dm.refresh_rate) {
            ++runframes;
            collector -= dm.refresh_rate;
        }

        for (int i = 0; i < runframes; ++i)
            jcv_exec();

        // Render the scene
        SDL_RenderClear(renderer);
        SDL_UpdateTexture(texture, NULL, vbuf,
            SCREEN_WIDTH * sizeof(uint32_t));
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        // Poll for events
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT: {
                    running = 0;
                    break;
                }
                default: {
                    jcv_input_handler(event);
                    break;
                }
            }
        }

    }

    // Bring down audio and video
    speex_resampler_destroy(resampler);

    if (vbuf)
        free(vbuf);

    if (romdata)
        free(romdata);

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    return 0;
}
