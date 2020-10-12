/*
 * Copyright (c) 2020 Rupert Carmichael
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "resample.h" // Speex Resampler

#include "jcv_mixer.h"
#include "jcv_psg.h"
#include "jcv_sgmpsg.h"

#define SAMPLERATE_PSG 224000 // Approximate PSG sample rate (Hz)
#define SAMPLERATE_SGMPSG 448000 // Approximate SGM PSG sample rate (Hz)

static int16_t *abuf = NULL; // Buffer to output resampled data into
static size_t samplerate = 48000; // Default sample rate is 48000Hz
static uint8_t framerate = 60; // Default to 60 for NTSC
static uint8_t rsq = 3; // Default resampler quality is 3

// Speex
static SpeexResamplerState *resampler = NULL;
static int err;

// Callback to notify the fronted that N samples are ready
static void (*jcv_mixer_cb)(size_t);

// Set the output sample rate
void jcv_mixer_set_rate(size_t rate) {
    switch (rate) {
        case 44100: case 48000: case 96000: case 192000:
            samplerate = rate;
            break;
        default:
            break;
    }
}

// Set the region
void jcv_mixer_set_region(uint8_t region) {
    framerate = region ? 50 : 60; // 50 for PAL, 60 for NSTC
}

// Set the resampler quality
void jcv_mixer_set_rsqual(uint8_t qual) {
    if (qual <= 10)
        rsq = qual;
}

// Set the pointer to the output audio buffer
void jcv_mixer_set_buffer(int16_t *ptr) {
    abuf = ptr;
}

// Set the callback that notifies the frontend that N audio samples are ready
void jcv_mixer_set_callback(void (*cb)(size_t)) {
    jcv_mixer_cb = cb;
}

// Deinitialize the resampler
void jcv_mixer_deinit(void) {
    if (resampler) {
        speex_resampler_destroy(resampler);
        resampler = NULL;
    }
}

// Bring up the Speex resampler
void jcv_mixer_init(void) {
    resampler = speex_resampler_init(1, SAMPLERATE_PSG, samplerate, rsq, &err);
}

// Resample raw audio and execute the callback
void jcv_mixer_resamp(size_t in_psg, size_t in_sgmpsg) {
    // Get buffers from both chips
    int16_t *psgbuf = jcv_psg_get_buffer();
    int16_t *sgmpsgbuf = jcv_sgmpsg_get_buffer();
    
    spx_uint32_t in_len = in_psg;
    
    // If the SGM is active, mix it in
    if (in_sgmpsg) {
        for (size_t i = 0; i < in_len; i++)
            psgbuf[i] += sgmpsgbuf[i];
    }
    
    spx_uint32_t outsamps = samplerate / framerate;
    err = speex_resampler_process_int(resampler, 0, (spx_int16_t*)psgbuf,
        &in_len, (spx_int16_t*)abuf, &outsamps);
    jcv_mixer_cb(outsamps);
}
