/*
Copyright (c) 2020-2025 Rupert Carmichael
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdint.h>
#include <stdlib.h>

#include <speex/speex_resampler.h>

#include "jollycv.h"

#include "jcv_mixer.h"

#define SAMPLERATE_COLECO 224009 // PSG sample rate (ColecoVision)
#define SAMPLERATE_CRV 125121 // PSG sample rate (CreatiVision)
#define SAMPLERATE_MYV 170673 // PSG sample rate (MyVision)
#define SIZE_PSGBUF 4800 // Size of the PSG buffers

static int16_t *abuf = NULL; // Buffer to output resampled data into

static sn76489_t *sn76489 = NULL;
static int16_t *sn76489buf = NULL;

static ay38910_t *ay38910 = NULL;
static int16_t *ay38910buf = NULL;

static size_t samplerate = 48000; // Default sample rate is 48000Hz
static unsigned framerate = 60; // Default to 60 for NTSC
static unsigned rsq = 3; // Default resampler quality is 3

static int sys = JCV_SYS_COLECO;
static int raw = 0;

// Speex
static SpeexResamplerState *resampler = NULL;
static int err;

// Callback to notify the fronted that N samples are ready
static void (*jcv_mixer_cb)(const void*, size_t);
static void *udata_mixer = NULL;

// ColecoVision (SN76489 and AY-3-8910)
static void jcv_mixer_resamp_coleco(size_t in_psg) {
    // Reset buffer position for both chips
    sn76489->bufpos = 0;
    ay38910->bufpos = 0;

    if (raw) {
        for (size_t i = 0; i < in_psg; ++i)
            abuf[i] = sn76489buf[i] + ay38910buf[i];
        jcv_mixer_cb(udata_mixer, in_psg);
        return;
    }

    spx_uint32_t in_len = in_psg;

    // Mix in the SGM samples
    for (size_t i = 0; i < in_len; ++i)
        sn76489buf[i] += ay38910buf[i];

    spx_uint32_t outsamps = samplerate / framerate;
    err = speex_resampler_process_int(resampler, 0, (spx_int16_t*)sn76489buf,
        &in_len, (spx_int16_t*)abuf, &outsamps);
    jcv_mixer_cb(udata_mixer, outsamps);
}

// CreatiVision (SN76489 PSG only)
static void jcv_mixer_resamp_crvision(size_t in_psg) {
    // Reset buffer position
    sn76489->bufpos = 0;

    if (raw) {
        for (size_t i = 0; i < in_psg; ++i)
            abuf[i] = sn76489buf[i];
        jcv_mixer_cb(udata_mixer, in_psg);
        return;
    }

    spx_uint32_t in_len = in_psg;

    spx_uint32_t outsamps = samplerate / framerate;
    err = speex_resampler_process_int(resampler, 0, (spx_int16_t*)sn76489buf,
        &in_len, (spx_int16_t*)abuf, &outsamps);
    jcv_mixer_cb(udata_mixer, outsamps);
}

// My Vision (AY-3-8910 PSG only)
static void jcv_mixer_resamp_myvision(size_t in_psg) {
    // Reset buffer position
    ay38910->bufpos = 0;

    if (raw) {
        for (size_t i = 0; i < in_psg; ++i)
            abuf[i] = ay38910buf[i];
        jcv_mixer_cb(udata_mixer, in_psg);
        return;
    }

    spx_uint32_t in_len = in_psg;

    spx_uint32_t outsamps = samplerate / framerate;
    err = speex_resampler_process_int(resampler, 0, (spx_int16_t*)ay38910buf,
        &in_len, (spx_int16_t*)abuf, &outsamps);
    jcv_mixer_cb(udata_mixer, outsamps);
}

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

void jcv_mixer_set_raw(int r) {
    raw = r;
}

// Set the region
void jcv_mixer_set_region(unsigned region) {
    framerate = region ? 50 : 60; // 50 for PAL, 60 for NSTC
}

// Set the resampler quality
void jcv_mixer_set_rsqual(unsigned qual) {
    if (qual <= 10)
        rsq = qual;
}

// Set the pointer to the output audio buffer
void jcv_mixer_set_buffer(int16_t *ptr) {
    abuf = ptr;
}

// Set the callback that notifies the frontend that N audio samples are ready
void jcv_mixer_set_callback(void (*cb)(const void*, size_t), void *u) {
    jcv_mixer_cb = cb;
    udata_mixer = u;
}

void jcv_mixer_set_sn76489(sn76489_t *ptr) {
    sn76489 = ptr;
}

void jcv_mixer_set_ay38910(ay38910_t *ptr) {
    ay38910 = ptr;
}

// Deinitialize the resampler
void jcv_mixer_deinit(void) {
    if (resampler) {
        speex_resampler_destroy(resampler);
        resampler = NULL;
    }

    if (sn76489buf)
        free(sn76489buf);

    if (ay38910buf)
        free(ay38910buf);
}

// Bring up the Speex resampler
void jcv_mixer_init(unsigned s) {
    sys = s;
    if (sys == JCV_SYS_CRVISION) { // CreatiVision
        resampler = speex_resampler_init(1, SAMPLERATE_CRV, samplerate,
            rsq, &err);
        sn76489buf = (int16_t*)calloc(1, SIZE_PSGBUF * sizeof(int16_t));
        sn76489->buf = sn76489buf;
        return;
    }
    else if (sys == JCV_SYS_MYVISION) { // MyVision
        resampler = speex_resampler_init(1, SAMPLERATE_MYV, samplerate,
            rsq, &err);
        ay38910buf = (int16_t*)calloc(1, SIZE_PSGBUF * sizeof(int16_t));
        ay38910->buf = ay38910buf;
        return;
    }

    // ColecoVision
    resampler = speex_resampler_init(1, SAMPLERATE_COLECO, samplerate,
        rsq, &err);
    sn76489buf = (int16_t*)calloc(1, SIZE_PSGBUF * sizeof(int16_t));
    ay38910buf = (int16_t*)calloc(1, SIZE_PSGBUF * sizeof(int16_t));
    sn76489->buf = sn76489buf;
    ay38910->buf = ay38910buf;
}

void jcv_mixer_resamp(void) {
    switch (sys) {
        default: case JCV_SYS_COLECO:
            jcv_mixer_resamp_coleco(sn76489->bufpos);
            break;
        case JCV_SYS_CRVISION: jcv_mixer_resamp_crvision(sn76489->bufpos);
            break;
        case JCV_SYS_MYVISION: jcv_mixer_resamp_myvision(ay38910->bufpos);
            break;
    }
}
