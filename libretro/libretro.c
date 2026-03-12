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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "jollycv.h"

#include "libretro.h"
#include "libretro_core_options.h"
#include "gamedb.h"
#include "md5.h"

#define ASPECT_NTSC 4.0 / 3.0
#define ASPECT_PAL 1.4257812

#if defined(_WIN32)
   static const char pss = '\\';
#else
   static const char pss = '/';
#endif

// Audio/Video buffers
static int16_t  *abuf = NULL;
static int16_t  *abuf_out = NULL;
static uint32_t *vbuf = NULL;
static size_t numsamps = 0;

// Copy of the ROM data passed in by the frontend
static void *romdata = NULL;

// Save directory
static const char *savedir;

// Variables for managing the libretro port
static int systype = JCV_SYS_COLECO;
static int region = JCV_REGION_NTSC;

static int crvision_autoreset = 0;
static int crvision_reset_counter = 0;

static int video_crop_t = 0;
static int video_crop_b = 0;
static int video_crop_l = 0;
static int video_crop_r = 0;
static int video_width_visible = JCV_VIDEO_WIDTH_MAX;
static int video_height_visible = JCV_VIDEO_HEIGHT_MAX;
static float video_aspect = (JCV_VIDEO_WIDTH_MAX * 1.0) / JCV_VIDEO_HEIGHT_MAX;

// libretro callbacks
static retro_log_printf_t log_cb = NULL;
static retro_video_refresh_t video_cb = NULL;
static retro_audio_sample_t audio_cb = NULL;
static retro_audio_sample_batch_t audio_batch_cb = NULL;
static retro_environment_t environ_cb = NULL;
static retro_input_poll_t input_poll_cb = NULL;
static retro_input_state_t input_state_cb = NULL;

struct retro_input_descriptor desc_null[] = {
{ 0, 0, 0, 0, NULL }
};

struct retro_input_descriptor desc_colecopad[] = {
{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,     "Joystick Up" },
{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,   "Joystick Down" },
{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,   "Joystick Left" },
{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  "Joystick Right" },
{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "Yellow (L)/1/5" },
{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,      "Orange (R)/2/6" },
{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,      "Purple/3/7" },
{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,      "Blue/4/8" },
{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,      "Spinner-" },
{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,      "Spinner+" },
{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,     "Shift 1" },
{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,     "Shift 2" },
{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Keypad */9/0" },
{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "Keypad #" },

{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,     "Joystick Up" },
{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,   "Joystick Down" },
{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,   "Joystick Left" },
{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  "Joystick Right" },
{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "Yellow (L)/1/5" },
{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,      "Orange (R)/2/6" },
{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,      "Purple/3/7" },
{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,      "Blue/4/8" },
{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,      "Spinner-" },
{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,      "Spinner+" },
{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,     "Shift 1" },
{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,     "Shift 2" },
{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Keypad */9/0" },
{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "Keypad #" },

{ 0, 0, 0, 0, NULL }
};

struct retro_input_descriptor desc_crvision[] = {
{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,     "Joystick Up" },
{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,   "Joystick Down" },
{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,   "Joystick Left" },
{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  "Joystick Right" },
{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "Fire (L)" },
{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,      "Fire (R)" },
{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,     "Shift" },
{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Start 1/Reset" },
{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "Start 2/Reset" },

{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,     "Joystick Up" },
{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,   "Joystick Down" },
{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,   "Joystick Left" },
{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  "Joystick Right" },
{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "Fire (L)" },
{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,      "Fire (R)" },
{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,     "Shift" },
{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Start 1/Reset" },
{ 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "Start 2/Reset" },

{ 0, 0, 0, 0, NULL }
};

struct retro_input_descriptor desc_myvision[] = {
{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,     "B (Up)" },
{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,   "C (Down)" },
{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,   "A (Left)" },
{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  "D (Right)" },
{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "1/5/9" },
{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,      "2/6/10" },
{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,      "3/7/11" },
{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,      "4/8/12" },
{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,      "13" },
{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,     "Shift 1" },
{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,     "Shift 2" },
{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "E" },
{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "14" },

{ 0, 0, 0, 0, NULL }
};

// Convert a nybble's hexadecimal representation to a lower case ASCII char
static inline char jcv_nyb_hexchar(unsigned nyb) {
    nyb &= 0xf; // Lower nybble only
    if (nyb >= 10)
        nyb += 'a' - 10;
    else
        nyb += '0';
    return (char)nyb;
}

static void jcv_hash_md5(char *md5, const void *data, size_t md5len) {
    MD5_CTX c;
    uint8_t *dataptr = (uint8_t*)data;
    uint8_t digest[16];
    MD5_Init(&c);
    MD5_Update(&c, dataptr, md5len);
    MD5_Final(digest, &c);

    // Convert the digest to a string without dodgy calls to snprintf
    for (size_t i = 0; i < 16; ++i) {
        md5[i * 2] = jcv_nyb_hexchar(digest[i] >> 4);
        md5[(i * 2) + 1] = jcv_nyb_hexchar(digest[i]);
    }
    md5[32] = '\0';
}

static void jcv_retro_log(int level, const char *fmt, ...) {
    if (log_cb == NULL)
        return;

    char outbuf[512];
    va_list va;
    va_start(va, fmt);
    vsnprintf(outbuf, sizeof(outbuf), fmt, va);
    va_end(va);
    log_cb(level, outbuf);
}

// Port Unconnected
static unsigned jcv_input_poll_none(const void *udata, int port) {
    (void)udata;
    (void)port;
    return 0xff;
}

static int roller[2] = {0,0};
// ColecoVision Paddle -- Use the Super Action Controller in all cases
static unsigned jcv_input_poll_colecopad(const void *udata, int port) {
    (void)udata;
    input_poll_cb();
    unsigned b = 0;

    if (input_state_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP))
        b |= COLECO_INPUT_U;
    if (input_state_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN))
        b |= COLECO_INPUT_D;
    if (input_state_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT))
        b |= COLECO_INPUT_L;
    if (input_state_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT))
        b |= COLECO_INPUT_R;

    if (input_state_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B)) {
        if (input_state_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2))
            b |= COLECO_INPUT_1;
        else if (input_state_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2))
            b |= COLECO_INPUT_5;
        else
            b |= COLECO_INPUT_FL;
    }
    if (input_state_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A)) {
        if (input_state_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2))
            b |= COLECO_INPUT_2;
        else if (input_state_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2))
            b |= COLECO_INPUT_6;
        else
            b |= COLECO_INPUT_FR;
    }
    if (input_state_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y)) {
        if (input_state_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2))
            b |= COLECO_INPUT_3;
        else if (input_state_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2))
            b |= COLECO_INPUT_7;
        else
            b |= COLECO_INPUT_P;
    }
    if (input_state_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X)) {
        if (input_state_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2))
            b |= COLECO_INPUT_4;
        else if (input_state_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2))
            b |= COLECO_INPUT_8;
        else
            b |= COLECO_INPUT_B;
    }

    if (input_state_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT)) {
        if (input_state_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2))
            b |= COLECO_INPUT_9;
        else if (input_state_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2))
            b |= COLECO_INPUT_0;
        else
            b |= COLECO_INPUT_STAR;
    }

    if (input_state_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START))
        b |= COLECO_INPUT_POUND;

    if (input_state_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L)) {
        b |= COLECO_INPUT_SP_PLUS;
        if (roller[port]++ >= 1) {
            b |= COLECO_INPUT_IRQ;
            roller[port] = 0;
        }
    }

    if (input_state_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R)) {
        b |= COLECO_INPUT_SP_MINUS;
        if (roller[port]++ >= 1) {
            b |= COLECO_INPUT_IRQ;
            roller[port] = 0;
        }
    }

    return b;
}

static unsigned jcv_input_poll_crvision(const void *udata, int port) {
    (void)udata;
    input_poll_cb();
    unsigned b = 0;

    if (input_state_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP))
        b |= CRV_INPUT_UP;
    if (input_state_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN))
        b |= CRV_INPUT_DOWN;
    if (input_state_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT))
        b |= CRV_INPUT_LEFT;
    if (input_state_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT))
        b |= CRV_INPUT_RIGHT;

    if (input_state_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT))
        b |= (port ? CRV_INPUT_7 : CRV_INPUT_B);
    if (input_state_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START))
        b |= (port ? CRV_INPUT_N : CRV_INPUT_6);

    if (input_state_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B))
        b |= CRV_INPUT_FIRE1;
    if (input_state_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A))
        b |= CRV_INPUT_FIRE2;

    return b;
}

static void jcv_input_poll_crvision_reset(void) {
    if (crvision_reset_counter > 0 && --crvision_reset_counter == 0) {
        jcv_reset(0);
        return;
    }

    if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2)) {
        if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT))
            jcv_reset(0);
        if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START))
            jcv_reset(0);
    }
}

static unsigned jcv_input_poll_myvision(const void *udata) {
    (void)udata;
    input_poll_cb();
    unsigned b = 0;

    if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP))
        b |= MYV_INPUT_B;
    if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN))
        b |= MYV_INPUT_C;
    if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT))
        b |= MYV_INPUT_A;
    if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT))
        b |= MYV_INPUT_D;
    if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT))
        b |= MYV_INPUT_E;
    if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START))
        b |= MYV_INPUT_14;
    if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R))
        b |= MYV_INPUT_13;

    if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B)) {
        if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2))
            b |= MYV_INPUT_5;
        else if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2))
            b |= MYV_INPUT_9;
        else
            b |= MYV_INPUT_1;
    }
    if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A)) {
        if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2))
            b |= MYV_INPUT_6;
        else if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2))
            b |= MYV_INPUT_10;
        else
            b |= MYV_INPUT_2;
    }
    if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y)) {
        if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2))
            b |= MYV_INPUT_7;
        else if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2))
            b |= MYV_INPUT_11;
        else
            b |= MYV_INPUT_3;
    }
    if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y)) {
        if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2))
            b |= MYV_INPUT_8;
        else if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2))
            b |= MYV_INPUT_12;
        else
            b |= MYV_INPUT_4;
    }

    return b;
}

static void jcv_cb_audio(const void *udata, size_t samps) {
    (void)udata;
    numsamps = samps;
    for (size_t i = 0; i < numsamps; ++i)
        abuf_out[i << 1] = abuf_out[(i << 1) + 1] = abuf[i];
}

static void jcv_geom_refresh(void) {
    struct retro_system_av_info avinfo;
    retro_get_system_av_info(&avinfo);
    avinfo.geometry.base_width = video_width_visible;
    avinfo.geometry.base_height = video_height_visible;
    avinfo.geometry.aspect_ratio = video_aspect;
    environ_cb(RETRO_ENVIRONMENT_SET_GEOMETRY, &avinfo);
}

static float jcv_aspect_ratio(int aspect_ratio_mode) {
    float aspect_ratio = ASPECT_NTSC;

    switch (aspect_ratio_mode) {
        default: case 0: { // Auto
            if (systype == JCV_SYS_CRVISION)
                aspect_ratio = ASPECT_PAL;
            else
                return ASPECT_NTSC;
            break;
        }
        case 1: { // 1:1 PAR
            aspect_ratio = 1.0;
            break;
        }
        case 2: { // NTSC
            return ASPECT_NTSC;
        }
        case 3: { // PAL
            aspect_ratio = ASPECT_PAL;
            break;
        }
    }

    return (video_width_visible * aspect_ratio) / video_height_visible;
}

static void check_variables(bool first_run) {
    struct retro_variable var = {0};

    // Palette
    var.key   = "jollycv_tmspalette";
    var.value = NULL;

    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        if (!strcmp(var.value, "TeaTime"))
            jcv_video_set_palette_tms9918(0);
        else if (!strcmp(var.value, "SYoung"))
            jcv_video_set_palette_tms9918(1);
        else if (!strcmp(var.value, "GCDatasheet"))
            jcv_video_set_palette_tms9918(2);
    }

    // Overscan
    var.key   = "jollycv_overscan_t";
    var.value = NULL;

    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        char *endptr;
        video_crop_t = strtol(var.value, &endptr, 10);
    }

    var.key   = "jollycv_overscan_b";
    var.value = NULL;

    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        char *endptr;
        video_crop_b = strtol(var.value, &endptr, 10);
    }

    var.key   = "jollycv_overscan_l";
    var.value = NULL;

    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        char *endptr;
        video_crop_l = strtol(var.value, &endptr, 10);
    }

    var.key   = "jollycv_overscan_r";
    var.value = NULL;

    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        char *endptr;
        video_crop_r = strtol(var.value, &endptr, 10);
    }

    // Recalculate visible video dimensions as they may have changed
    video_width_visible = JCV_VIDEO_WIDTH_MAX - (video_crop_l + video_crop_r);
    video_height_visible = JCV_VIDEO_HEIGHT_MAX - (video_crop_t + video_crop_b);

    // Aspect Ratio -- this MUST stay below the overscan settings
    var.key   = "jollycv_aspect";
    var.value = NULL;

    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        if (!strcmp(var.value, "1:1"))
            video_aspect = jcv_aspect_ratio(1);
        else if (!strcmp(var.value, "NTSC"))
            video_aspect = jcv_aspect_ratio(2);
        else if (!strcmp(var.value, "PAL"))
            video_aspect = jcv_aspect_ratio(3);
        else
            video_aspect = jcv_aspect_ratio(0);
    }

    // No sprite limit
    var.key   = "jollycv_nosprlimit";
    var.value = NULL;

    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
        jcv_video_set_nosprlimit_tms9918(strcmp(var.value, "off"));

    // CreatiVision Auto-Reset
    var.key   = "jollycv_autoreset";
    var.value = NULL;

    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        crvision_autoreset = !strcmp(var.value, "on");
        if (first_run && crvision_autoreset)
            crvision_reset_counter = 60;
    }
}

void retro_init(void) {
    // Set up log callback
    struct retro_log_callback log;
    if (environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log))
        log_cb = log.log;

    // Set initial core options
    check_variables(true);

    // Set up logging
    jcv_log_set_callback(jcv_retro_log);

    // Allocate and pass the video buffer into the emulator
    vbuf = (uint32_t*)calloc(1,
        JCV_VIDEO_WIDTH_MAX * JCV_VIDEO_HEIGHT_MAX * sizeof(uint32_t));
    jcv_video_set_buffer(vbuf);

    // Allocate and pass the audio buffer into the emulator
    abuf = (int16_t*)calloc(1, 8192 * sizeof(int16_t));
    abuf_out = (int16_t*)calloc(1, 8192 * sizeof(int16_t));
    jcv_audio_set_buffer(abuf);

    // Set up audio callback
    jcv_audio_set_callback(&jcv_cb_audio, NULL);
}

void retro_deinit(void) {
    jcv_deinit();

    if (vbuf) {
        free(vbuf);
        vbuf = NULL;
    }

    if (abuf) {
        free(abuf);
        abuf = NULL;
    }

    if (abuf_out) {
        free(abuf_out);
        abuf_out = NULL;
    }
}

unsigned retro_api_version(void) {
    return RETRO_API_VERSION;
}

void retro_set_controller_port_device(unsigned port, unsigned device) {
    log_cb(RETRO_LOG_INFO, "Plugging device %u into port %u.\n", device, port);
}

void retro_get_system_info(struct retro_system_info *info) {
    memset(info, 0, sizeof(*info));
    info->library_name     = "JollyCV";
    info->library_version  = "2.0.0";
    info->need_fullpath    = false;
    info->valid_extensions = "col|rom|myv|bin";
}

void retro_get_system_av_info(struct retro_system_av_info *info) {
    info->timing = (struct retro_system_timing) {
        .fps = systype == JCV_SYS_CRVISION ? 50 : 60,
        .sample_rate = 48000
    };

    info->geometry = (struct retro_game_geometry) {
        .base_width   = video_width_visible,
        .base_height  = video_height_visible,
        .max_width    = JCV_VIDEO_WIDTH_MAX,
        .max_height   = JCV_VIDEO_HEIGHT_MAX,
        .aspect_ratio = jcv_aspect_ratio(0)
    };
}

void retro_set_environment(retro_environment_t cb) {
    environ_cb = cb;
    libretro_set_core_options(environ_cb);
}

void retro_set_audio_sample(retro_audio_sample_t cb) {
    audio_cb = cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) {
    audio_batch_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb) {
    input_poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb) {
    input_state_cb = cb;
}

void retro_set_video_refresh(retro_video_refresh_t cb) {
    video_cb = cb;
}

void retro_reset(void) {
    jcv_reset(0);
}

void retro_run(void) {
    input_poll_cb();
    if (systype == JCV_SYS_CRVISION)
        jcv_input_poll_crvision_reset();

    jcv_exec();

    bool update = false;
    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &update) && update) {
        check_variables(false);
        jcv_geom_refresh();
    }

    video_cb(vbuf + (JCV_VIDEO_WIDTH_MAX * video_crop_t) + video_crop_l,
        video_width_visible,
        video_height_visible,
        JCV_VIDEO_WIDTH_MAX << 2);

    audio_batch_cb(abuf_out, numsamps);
}

bool retro_load_game(const struct retro_game_info *info) {
    // Step 0: Get the MD5 checksum
    char md5[33];
    jcv_hash_md5(md5, info->data, info->size);

    enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;
    if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt)) {
        log_cb(RETRO_LOG_INFO, "XRGB8888 unsupported\n");
        return false;
    }

    // Attempt to figure out what system needs to be emulated
    const char *extptr = strrchr(info->path, '.');
    if (extptr != NULL)
        extptr++;
    else
        return false;

    // Convert extension to lower case
    char ext[4];
    snprintf(ext, sizeof(ext), "%s", extptr);
    for (size_t i = 0; i < strlen(extptr); ++i)
        ext[i] = tolower(ext[i]);

    systype = JCV_SYS_COLECO; // Default to ColecoVision
    if (!strcmp(ext, "myv")) {
        systype = JCV_SYS_MYVISION;
    }
    else if (!strcmp(ext, "rom") || !strcmp(ext, "bin")) { // CreatiVision?
        for (size_t i = 0; i < sizeof(gamedb_crvision) / sizeof(char*); ++i) {
            if (!strcmp(md5, gamedb_crvision[i])) {
                systype = JCV_SYS_CRVISION;
                break;
            }
        }
    }

    // Set the region and system type
    jcv_set_region(systype == JCV_SYS_CRVISION ? JCV_REGION_PAL : JCV_REGION_NTSC);
    jcv_set_system(systype);

    const char *sysdir;
    if (!environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &sysdir) || !sysdir)
        return false;

    // Perform any special internal operations for this game
    jcv_process_hash(md5);

    // Use these flags to detect input device etc -- later though
    uint32_t flags = jcv_get_dbflags();
    (void)flags;

    char biospath[256];
    if (systype == JCV_SYS_COLECO) {
        snprintf(biospath, sizeof(biospath), "%s%c%s", sysdir, pss, "coleco.rom");

        if (!jcv_bios_load_file(biospath)) {
            log_cb(RETRO_LOG_ERROR, "Failed to load bios at: %s\n", biospath);
            return false;
        }

        environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc_colecopad);
    }
    else if (systype == JCV_SYS_CRVISION) {
        snprintf(biospath, sizeof(biospath), "%s%c%s", sysdir, pss, "bioscv.rom");

        if (!jcv_bios_load_file(biospath)) {
            log_cb(RETRO_LOG_ERROR, "Failed to load bios at: %s\n", biospath);
            return false;
        }

        environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc_crvision);
    }
    else if (systype == JCV_SYS_MYVISION) {
        environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc_myvision);
    }

    /* Keep an internal copy of the ROM to avoid relying on the frontend
       keeping it persistently.
    */
    romdata = (void*)calloc(1, info->size);
    memcpy(romdata, info->data, info->size);

    if (!jcv_media_load(romdata, info->size)) {
        log_cb(RETRO_LOG_ERROR, "Failed to load ROM\n");
        retro_unload_game();
        return false;
    }

    // Grab the libretro frontend's save directory
    if (!environ_cb(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &savedir) || !savedir)
        return false;

    jcv_input_set_callback_coleco(&jcv_input_poll_colecopad, NULL);
    jcv_input_set_callback_crvision(&jcv_input_poll_crvision, NULL);
    jcv_input_set_callback_myvision(&jcv_input_poll_myvision, NULL);

    jcv_init();

    jcv_reset(2);
    return true;
}

void retro_unload_game(void) {
    if (romdata)
        free(romdata);
}

unsigned retro_get_region(void) {
    return RETRO_REGION_NTSC;
}

bool retro_load_game_special(unsigned type, const struct retro_game_info *info,
                             size_t num) {
    (void)type;
    (void)info;
    (void)num;
    return false; // I wish I was special, but I'm a creep, I'm a weirdo
}

size_t retro_serialize_size(void) {
    return jcv_state_size();
}

bool retro_serialize(void *data, size_t size) {
    memcpy(data, jcv_state_save_raw(), size);
    return true;
}

bool retro_unserialize(const void *data, size_t size) {
    (void)size;
    jcv_state_load_raw(data);
    return true;
}

void *retro_get_memory_data(unsigned id) {
    switch (id) {
        case RETRO_MEMORY_SAVE_RAM:
            return jcv_memory_get_data(JCV_MEM_SAVEDATA);
        case RETRO_MEMORY_SYSTEM_RAM:
            return jcv_memory_get_data(JCV_MEM_SYSTEM);
        case RETRO_MEMORY_VIDEO_RAM:
            return jcv_memory_get_data(JCV_MEM_VRAM);
        default:
            return NULL;
    }
}

size_t retro_get_memory_size(unsigned id) {
    switch (id) {
        case RETRO_MEMORY_SAVE_RAM:
            return jcv_memory_get_size(JCV_MEM_SAVEDATA);
        case RETRO_MEMORY_SYSTEM_RAM:
            return jcv_memory_get_size(JCV_MEM_SYSTEM);
        case RETRO_MEMORY_VIDEO_RAM:
            return jcv_memory_get_size(JCV_MEM_VRAM);
        default:
            return 0;
    }
}

void retro_cheat_reset(void) {
}

void retro_cheat_set(unsigned index, bool enabled, const char *code) {
    (void)index;
    (void)enabled;
    (void)code;
}
