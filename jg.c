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

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <jg/jg.h>
#include <jg/jg_coleco.h>
#if JG_VERSION_NUMBER > 10000
#include <jg/jg_crvision.h>
#include <jg/jg_myvision.h>
#else
#include "jg_crvision_jcv.h"
#include "jg_myvision_jcv.h"
#endif

#include "jcv.h"

#include "jcv_crvision.h"
#include "jcv_myvision.h"

#include "version.h"

#define SAMPLERATE 48000
#define FRAMERATE 60
#define FRAMERATE_PAL 50
#define ASPECT_NTSC 4.0/3.0
#define ASPECT_PAL 1.4257812
#define CHANNELS 1
#define NUMINPUTS 2

static jg_cb_audio_t jg_cb_audio;
static jg_cb_frametime_t jg_cb_frametime;
static jg_cb_log_t jg_cb_log;
static jg_cb_rumble_t jg_cb_rumble;

static jg_coreinfo_t coreinfo_coleco = {
    "jollycv", "JollyCV", JG_VERSION, "coleco", NUMINPUTS, 0x00
};

static jg_coreinfo_t coreinfo_crvision = {
    "jollycv", "JollyCV", JG_VERSION, "crvision", NUMINPUTS, 0x00
};

static jg_coreinfo_t coreinfo_myvision = {
    "jollycv", "JollyCV", JG_VERSION, "myvision", NUMINPUTS, 0x00
};

static jg_videoinfo_t vidinfo = {
    JG_PIXFMT_XRGB8888,         // pixfmt
    JCV_VIDEO_WIDTH_MAX,        // wmax
    JCV_VIDEO_HEIGHT_MAX,       // hmax
    JCV_VIDEO_WIDTH_MAX - 12,   // w
    JCV_VIDEO_HEIGHT_MAX,       // h
    6,                          // x
    0,                          // y
    JCV_VIDEO_WIDTH_MAX,        // p
    ASPECT_NTSC,                // aspect
    NULL                        // buf
};

static jg_audioinfo_t audinfo = {
    JG_SAMPFMT_INT16,
    SAMPLERATE,
    CHANNELS,
    (SAMPLERATE / FRAMERATE) * CHANNELS,
    NULL
};

static jg_pathinfo_t pathinfo;
static jg_fileinfo_t biosinfo;
static jg_fileinfo_t gameinfo;
static jg_inputinfo_t inputinfo[NUMINPUTS];
static jg_inputstate_t *input_device[NUMINPUTS];

// Emulator settings
static jg_setting_t settings_jcv[] = {
    { "input_coleco", "Input Devices (ColecoVision)",
      "0 = Auto, 1 = ColecoVision Paddle, 2 = Roller Controller, "
      "3 = Super Action Controller, 4 = Super Sketch, 5 = Steering Wheel",
      "Select the desired input device(s)",
      0, 0, 3, JG_SETTING_INPUT
    },
    { "mask_overscan", "Mask Overscan",
      "0 = Show Overscan, 1 = Mask Overscan",
      "Show or Mask Overscan regions bordering the rendered gameplay area",
      0, 0, 1, JG_SETTING_RESTART
    },
    { "palette_tms9918", "Colour Palette (TMS9918)",
      "0 = TeaTime, 1 = SYoung, 2 = GCDatasheet",
      "Select a colour palette. TeaTime is a custom palette, SYoung is the "
      "palette widely available in many emulator, and GCDatasheet is based "
      "on the values from the datasheet, gamma corrected.",
      0, 0, 2, 0
    },
    { "rsqual", "Resampler Quality",
      "N = Resampler Quality",
      "Quality level for the internal resampler",
      3, 0, 10, JG_SETTING_RESTART
    },
    { "region", "Region",
      "0 = NTSC, 1 = PAL",
      "Select the region",
      JCV_REGION_NTSC, JCV_REGION_NTSC, JCV_REGION_PAL, JG_SETTING_RESTART
    }
};

enum {
    INPUT_COLECO,
    MASKOVERSCAN,
    PALETTE_TMS9918,
    RSQUAL,
    REGION,
};

// System being emulated
static int sys = JCV_SYS_COLECO;
static uint32_t dbflags = 0;

static void jcv_cb_audio(const void *udata, size_t samps) {
    (void)udata;
    jg_cb_audio(samps);
}

// Unconnected Port
static unsigned jcv_coleco_input_poll_null(const void *udata, int port) {
    (void)udata;
    (void)port;
    return 0x0000;
}

// ColecoVision Paddle
static unsigned jcv_coleco_input_poll(const void *udata, int port) {
    (void)udata;
    unsigned b = 0;
    for (unsigned i = 0; i < NDEFS_COLECOPAD; ++i)
        if (input_device[port]->button[i]) b |= (1 << i);
    return b;
}

// ColecoVision Roller Controller
static unsigned jcv_coleco_input_poll_roller(const void *udata, int port) {
    (void)udata;
    unsigned b = 0;

    for (unsigned i = 0; i < NDEFS_COLECOPAD; ++i)
        if (input_device[port]->button[i]) b |= (1 << i);

    int rel = input_device[0]->rel[port] / 4;
    input_device[0]->rel[port] -= rel;

    if (rel < 0) {
        b |= port ? JCV_COLECO_INPUT_SP_PLUS : JCV_COLECO_INPUT_SP_MINUS;
        b |= JCV_COLECO_INPUT_IRQ;
    }
    else if (rel > 0) {
        b |= port ? JCV_COLECO_INPUT_SP_MINUS : JCV_COLECO_INPUT_SP_PLUS;
        b |= JCV_COLECO_INPUT_IRQ;
    }

    return b;
}

// ColecoVision Super Action Controller
static unsigned jcv_coleco_input_poll_sac(const void *udata, int port) {
    (void)udata;
    unsigned b = 0;

    if (input_device[port]->button[0x00]) b |= JCV_COLECO_INPUT_U;
    if (input_device[port]->button[0x01]) b |= JCV_COLECO_INPUT_D;
    if (input_device[port]->button[0x02]) b |= JCV_COLECO_INPUT_L;
    if (input_device[port]->button[0x03]) b |= JCV_COLECO_INPUT_R;
    if (input_device[port]->button[0x04]) b |= JCV_COLECO_INPUT_Y;
    if (input_device[port]->button[0x05]) b |= JCV_COLECO_INPUT_O;
    if (input_device[port]->button[0x06]) b |= JCV_COLECO_INPUT_P;
    if (input_device[port]->button[0x07]) b |= JCV_COLECO_INPUT_B;
    if (input_device[port]->button[0x08]) b |= JCV_COLECO_INPUT_1;
    if (input_device[port]->button[0x09]) b |= JCV_COLECO_INPUT_2;
    if (input_device[port]->button[0x0a]) b |= JCV_COLECO_INPUT_3;
    if (input_device[port]->button[0x0b]) b |= JCV_COLECO_INPUT_4;
    if (input_device[port]->button[0x0c]) b |= JCV_COLECO_INPUT_5;
    if (input_device[port]->button[0x0d]) b |= JCV_COLECO_INPUT_6;
    if (input_device[port]->button[0x0e]) b |= JCV_COLECO_INPUT_7;
    if (input_device[port]->button[0x0f]) b |= JCV_COLECO_INPUT_8;
    if (input_device[port]->button[0x10]) b |= JCV_COLECO_INPUT_9;
    if (input_device[port]->button[0x11]) b |= JCV_COLECO_INPUT_0;
    if (input_device[port]->button[0x12]) b |= JCV_COLECO_INPUT_STAR;
    if (input_device[port]->button[0x13]) b |= JCV_COLECO_INPUT_POUND;

    // Speed Rollers
    if (input_device[port]->button[0x14]) {
        b |= JCV_COLECO_INPUT_SP_PLUS;
        if (input_device[port]->button[0x14]++ > 1) {
            input_device[port]->button[0x14] = 1;
            b |= JCV_COLECO_INPUT_IRQ;
        }
    }
    if (input_device[port]->button[0x15]) {
        b |= JCV_COLECO_INPUT_SP_MINUS;
        if (input_device[port]->button[0x15]++ > 1) {
            input_device[port]->button[0x15] = 1;
            b |= JCV_COLECO_INPUT_IRQ;
        }
    }

    return b;
}

// ColecoVision Expansion Module #2 - Steering Wheel
static unsigned jcv_coleco_input_poll_wheel(const void *udata, int port) {
    (void)udata;
    unsigned b = 0;

    if (port == 0) { // Steering Wheel and Pedal on first port
        int rel = input_device[0]->rel[port] / 3;
        input_device[0]->rel[0] -= rel;

        if (rel < 0)
            b |= (JCV_COLECO_INPUT_SP_MINUS | JCV_COLECO_INPUT_IRQ);
        else if (rel > 0)
            b |= (JCV_COLECO_INPUT_SP_PLUS | JCV_COLECO_INPUT_IRQ);

        // Pedal uses the same signal as the FireL on the standard paddle
        if (input_device[0]->button[0]) b |= JCV_COLECO_INPUT_FL;
    }
    else if (port == 1) { // Numpad and Stick on second port
        // Hardcode to frontend input device 0
        for (unsigned i = 0; i < 4; ++i) // Stick U/D/L/R
            if (input_device[0]->button[i + 1]) b |= (1 << i);
        for (unsigned i = 0; i < 12; ++i) // Numpad
            if (input_device[0]->button[i + 5]) b |= (JCV_COLECO_INPUT_1 << i);
    }

    return b;
}

static uint8_t jcv_crvision_input_poll(int keylatch) {
    uint8_t b = 0xff;

    // Check joystick directional inputs to handle special diagonal bits
    int p1_u = input_device[0]->button[0];
    int p1_d = input_device[0]->button[1];
    int p1_l = input_device[0]->button[2];
    int p1_r = input_device[0]->button[3];
    int p2_u = input_device[1]->button[0];
    int p2_d = input_device[1]->button[1];
    int p2_l = input_device[1]->button[2];
    int p2_r = input_device[1]->button[3];

    switch (keylatch) {
        case 0x01: { // PA0 - Left Joystick
            // Check for diagonal combinations
            if (p1_d && p1_r)
                b &= ~CRV_INPUT_DOWNRIGHT;
            else if (p1_u && p1_r)
                b &= ~CRV_INPUT_UPRIGHT;
            else if (p1_u && p1_l)
                b &= ~CRV_INPUT_UPLEFT;
            else if (p1_d && p1_l)
                b &= ~CRV_INPUT_DOWNLEFT;
            else {
                if (p1_u) b &= ~CRV_INPUT_UP;
                if (p1_d) b &= ~CRV_INPUT_DOWN;
                if (p1_l) b &= ~CRV_INPUT_LEFT;
                if (p1_r) b &= ~CRV_INPUT_RIGHT;
            }
            // Cntl and Fire 2 are the same
            if (input_device[0]->button[12]) b &= ~CRV_INPUT_CNTL;
            if (input_device[0]->button[5]) b &= ~CRV_INPUT_FIRE; // Fire 2
            if (input_device[0]->button[6]) b &= ~CRV_INPUT_1;
            break;
        }
        case 0x02: { // PA1 - Left Keyboard
            if (input_device[0]->button[7]) b &= ~CRV_INPUT_2;
            if (input_device[0]->button[8]) b &= ~CRV_INPUT_3;
            if (input_device[0]->button[9]) b &= ~CRV_INPUT_4;
            if (input_device[0]->button[10]) b &= ~CRV_INPUT_5;
            if (input_device[0]->button[11]) b &= ~CRV_INPUT_6;
            if (input_device[0]->button[13]) b &= ~CRV_INPUT_Q;
            if (input_device[0]->button[14]) b &= ~CRV_INPUT_W;
            if (input_device[0]->button[15]) b &= ~CRV_INPUT_E;
            if (input_device[0]->button[16]) b &= ~CRV_INPUT_R;
            if (input_device[0]->button[17]) b &= ~CRV_INPUT_T;
            if (input_device[0]->button[18]) b &= ~CRV_INPUT_BACKSPACE;
            if (input_device[0]->button[19]) b &= ~CRV_INPUT_A;
            if (input_device[0]->button[20]) b &= ~CRV_INPUT_S;
            if (input_device[0]->button[21]) b &= ~CRV_INPUT_D;
            if (input_device[0]->button[22]) b &= ~CRV_INPUT_F;
            if (input_device[0]->button[23]) b &= ~CRV_INPUT_G;
            if (input_device[0]->button[24]) b &= ~CRV_INPUT_SHIFT;
            if (input_device[0]->button[25]) b &= ~CRV_INPUT_Z;
            if (input_device[0]->button[26]) b &= ~CRV_INPUT_X;
            if (input_device[0]->button[27]) b &= ~CRV_INPUT_C;
            if (input_device[0]->button[28]) b &= ~CRV_INPUT_V;
            if (input_device[0]->button[29]) b &= ~CRV_INPUT_B;

            if (input_device[0]->button[4]) b &= ~CRV_INPUT_FIRE; // Fire 1
            break;
        }
        case 0x04: { // PA2 - Right Joystick
            // Check for diagonal combinations
            if (p2_d && p2_r)
                b &= ~CRV_INPUT_DOWNRIGHT;
            else if (p2_u && p2_r)
                b &= ~CRV_INPUT_UPRIGHT;
            else if (p2_u && p2_l)
                b &= ~CRV_INPUT_UPLEFT;
            else if (p2_d && p2_l)
                b &= ~CRV_INPUT_DOWNLEFT;
            else {
                if (p2_u) b &= ~CRV_INPUT_UP;
                if (p2_d) b &= ~CRV_INPUT_DOWN;
                if (p2_l) b &= ~CRV_INPUT_LEFT;
                if (p2_r) b &= ~CRV_INPUT_RIGHT;
            }
            if (input_device[1]->button[5]) b &= ~CRV_INPUT_FIRE; // Fire 2
            if (input_device[1]->button[23]) b &= ~CRV_INPUT_TAB;
            if (input_device[1]->button[29]) b &= ~CRV_INPUT_SPACE;
            break;
        }
        case 0x08: { // PA3 - Right Keyboard
            if (input_device[1]->button[6]) b &= ~CRV_INPUT_7;
            if (input_device[1]->button[7]) b &= ~CRV_INPUT_8;
            if (input_device[1]->button[8]) b &= ~CRV_INPUT_9;
            if (input_device[1]->button[9]) b &= ~CRV_INPUT_0;
            if (input_device[1]->button[10]) b &= ~CRV_INPUT_COLON;
            if (input_device[1]->button[11]) b &= ~CRV_INPUT_MINUS;
            if (input_device[1]->button[12]) b &= ~CRV_INPUT_Y;
            if (input_device[1]->button[13]) b &= ~CRV_INPUT_U;
            if (input_device[1]->button[14]) b &= ~CRV_INPUT_I;
            if (input_device[1]->button[15]) b &= ~CRV_INPUT_O;
            if (input_device[1]->button[16]) b &= ~CRV_INPUT_P;
            if (input_device[1]->button[17]) b &= ~CRV_INPUT_RETN;
            if (input_device[1]->button[18]) b &= ~CRV_INPUT_H;
            if (input_device[1]->button[19]) b &= ~CRV_INPUT_J;
            if (input_device[1]->button[20]) b &= ~CRV_INPUT_K;
            if (input_device[1]->button[21]) b &= ~CRV_INPUT_L;
            if (input_device[1]->button[22]) b &= ~CRV_INPUT_SEMICOLON;
            if (input_device[1]->button[24]) b &= ~CRV_INPUT_N;
            if (input_device[1]->button[25]) b &= ~CRV_INPUT_M;
            if (input_device[1]->button[26]) b &= ~CRV_INPUT_COMMA;
            if (input_device[1]->button[27]) b &= ~CRV_INPUT_PERIOD;
            if (input_device[1]->button[28]) b &= ~CRV_INPUT_SLASH;

            if (input_device[1]->button[4]) b &= ~CRV_INPUT_FIRE; // Fire 1
            break;
        }
        case 0x0f: {
            break;
        }
        default: {
            // Multiple PA lines low - special scanning mode
            break;
        }
    }

    return b;
}

static uint8_t jcv_myvision_input_poll(int column) {
    uint8_t b = 0xff;

    switch (column) {
        case 0x80: {
            if (input_device[0]->button[12]) b &= ~MYV_INPUT_13;
            if (input_device[0]->button[16]) b &= ~MYV_INPUT_C;
            if (input_device[0]->button[8]) b &= ~MYV_INPUT_9;
            if (input_device[0]->button[4]) b &= ~MYV_INPUT_5;
            if (input_device[0]->button[0]) b &= ~MYV_INPUT_1;
            break;
        }
        case 0x40: {
            if (input_device[0]->button[15]) b &= ~MYV_INPUT_B;
            if (input_device[0]->button[11]) b &= ~MYV_INPUT_12;
            if (input_device[0]->button[7]) b &= ~MYV_INPUT_8;
            if (input_device[0]->button[3]) b &= ~MYV_INPUT_4;
            break;
        }
        case 0x20: {
            if (input_device[0]->button[13]) b &= ~MYV_INPUT_14;
            if (input_device[0]->button[17]) b &= ~MYV_INPUT_D;
            if (input_device[0]->button[9]) b &= ~MYV_INPUT_10;
            if (input_device[0]->button[5]) b &= ~MYV_INPUT_6;
            if (input_device[0]->button[1]) b &= ~MYV_INPUT_2;
            break;
        }
        case 0x10: {
            if (input_device[0]->button[14]) b &= ~MYV_INPUT_A;
            if (input_device[0]->button[18]) b &= ~MYV_INPUT_E;
            if (input_device[0]->button[10]) b &= ~MYV_INPUT_11;
            if (input_device[0]->button[6]) b &= ~MYV_INPUT_7;
            if (input_device[0]->button[2]) b &= ~MYV_INPUT_3;
            break;
        }
    }

    return b;
}

static void jcv_input_setup(void) {
    if (sys == JCV_SYS_CRVISION) {
        inputinfo[0] = jg_crvision_inputinfo(0, JG_CRVISION_LPAD);
        inputinfo[1] = jg_crvision_inputinfo(1, JG_CRVISION_RPAD);
        return;
    }
    else if (sys == JCV_SYS_MYVISION) {
        inputinfo[0] = jg_myvision_inputinfo(0, JG_MYVISION_SYSTEM);
        return;
    }

    int itype = settings_jcv[INPUT_COLECO].val;

    if (!itype) {
        switch (dbflags) {
            case JCV_DB_COLECO_ROLLER: itype = JG_COLECO_ROLLER; break;
            case JCV_DB_COLECO_SAC: itype = JG_COLECO_SAC; break;
            case JCV_DB_COLECO_SKETCH: itype = JG_COLECO_SKETCH; break;
            case JCV_DB_COLECO_WHEEL: itype = JG_COLECO_WHEEL; break;
        }
    }

    switch (itype) {
        default: case 0: case JG_COLECO_PAD: {
            inputinfo[0] = jg_coleco_inputinfo(0, JG_COLECO_PAD);
            inputinfo[1] = jg_coleco_inputinfo(1, JG_COLECO_PAD);
            jcv_input_set_callback_coleco(&jcv_coleco_input_poll, NULL);
            break;
        }
        case JG_COLECO_ROLLER: {
            inputinfo[0] = jg_coleco_inputinfo(0, JG_COLECO_ROLLER);
            inputinfo[1] = jg_coleco_inputinfo(1, JG_COLECO_ROLLER);
            jcv_input_set_callback_coleco(&jcv_coleco_input_poll_roller, NULL);
            break;
        }
        case JG_COLECO_SAC: {
            inputinfo[0] = jg_coleco_inputinfo(0, JG_COLECO_SAC);
            inputinfo[1] = jg_coleco_inputinfo(1, JG_COLECO_SAC);
            jcv_input_set_callback_coleco(&jcv_coleco_input_poll_sac, NULL);
            break;
        }
        case JG_COLECO_SKETCH: {
            inputinfo[0] = jg_coleco_inputinfo(0, JG_COLECO_UNCONNECTED);
            inputinfo[1] = jg_coleco_inputinfo(1, JG_COLECO_UNCONNECTED);
            jcv_input_set_callback_coleco(&jcv_coleco_input_poll_null, NULL);
            jg_cb_log(JG_LOG_WRN, "Super Sketch not supported\n");
            break;
        }
        case JG_COLECO_WHEEL: {
            inputinfo[0] = jg_coleco_inputinfo(0, JG_COLECO_WHEEL);
            inputinfo[1] = jg_coleco_inputinfo(1, JG_COLECO_UNCONNECTED);
            jcv_input_set_callback_coleco(&jcv_coleco_input_poll_wheel, NULL);
            break;
        }
    }
}

void jg_set_cb_audio(jg_cb_audio_t func) {
    jg_cb_audio = func;
}

void jg_set_cb_frametime(jg_cb_frametime_t func) {
    jg_cb_frametime = func;
}

void jg_set_cb_log(jg_cb_log_t func) {
    jg_cb_log = func;
    jcv_log_set_callback(func);
}

void jg_set_cb_rumble(jg_cb_rumble_t func) {
    jg_cb_rumble = func;
}

int jg_init(void) {
    jcv_input_set_callback_coleco(&jcv_coleco_input_poll, NULL);
    jcv_crvision_input_set_callback(&jcv_crvision_input_poll);
    jcv_myvision_input_set_callback(&jcv_myvision_input_poll);

    jcv_audio_set_callback(jcv_cb_audio, NULL);
    jcv_audio_set_rate(SAMPLERATE);
    jcv_audio_set_rsqual(settings_jcv[RSQUAL].val);

    jcv_video_set_palette_tms9918(settings_jcv[PALETTE_TMS9918].val);

    jcv_set_system(sys);

    if (sys == JCV_SYS_CRVISION)
        jcv_set_region(1); // force PAL
    else
        jcv_set_region(settings_jcv[REGION].val);

    jcv_init();

    return 1;
}

void jg_deinit(void) {
    jcv_deinit();
}

void jg_reset(int hard) {
    jcv_reset(hard);
}

void jg_exec_frame(void) {
    jcv_exec();
}

int jg_game_load(void) {
    // Try to load the BIOS as an auxiliary file
    if (biosinfo.size) {
        jcv_bios_load(biosinfo.data, biosinfo.size);
    }
    else {
        char bpath[256];
        if (sys == JCV_SYS_COLECO) {
            snprintf(bpath, sizeof(bpath), "%s/coleco.rom", pathinfo.bios);
            if (!jcv_bios_load_file(bpath))
                jg_cb_log(JG_LOG_ERR, "Failed to load bios %s\n", bpath);
        }
        else if (sys == JCV_SYS_CRVISION) {
            snprintf(bpath, sizeof(bpath), "%s/bioscv.rom", pathinfo.bios);
            if (!jcv_bios_load_file(bpath))
                jg_cb_log(JG_LOG_ERR, "Failed to load bios %s\n", bpath);
        }
    }

    // Load the ROM
    if (sys == JCV_SYS_COLECO) {
        dbflags = jcv_get_dbflags(gameinfo.md5);
        if (!jcv_media_load(gameinfo.data, gameinfo.size))
            return 0;

        char savename[292];
        snprintf(savename, sizeof(savename),
            "%s/%s.srm", pathinfo.save, gameinfo.name);
        int sramstat = jcv_savedata_load((const char*)savename);

        if (sramstat == JCV_SAVE_SUCCEED)
            jg_cb_log(JG_LOG_DBG, "SRAM/EEPROM Loaded: %s\n", savename);
        else if (sramstat == JCV_SAVE_NONE)
            jg_cb_log(JG_LOG_DBG, "SRAM/EEPROM File Missing: %s\n", savename);
        else
            jg_cb_log(JG_LOG_DBG, "SRAM/EEPROM Load Failed: %s\n", savename);

        // Set the samples per frame and frame timing depending on region
        if (settings_jcv[REGION].val) { // PAL mode
            vidinfo.aspect = ASPECT_PAL;
            audinfo.spf = (SAMPLERATE / FRAMERATE_PAL) * CHANNELS;
            jg_cb_frametime(FRAMERATE_PAL);
        }
        else { // NTSC mode
            vidinfo.aspect = ASPECT_NTSC;
            audinfo.spf = (SAMPLERATE / FRAMERATE) * CHANNELS;
            jg_cb_frametime(FRAMERATE);
        }
    }
    else if (sys == JCV_SYS_CRVISION) {
        if (!jcv_media_load(gameinfo.data, gameinfo.size))
            return 0;
        vidinfo.aspect = ASPECT_PAL;
        audinfo.spf = (SAMPLERATE / FRAMERATE_PAL) * CHANNELS;
        jg_cb_frametime(FRAMERATE_PAL);
    }
    else if (sys == JCV_SYS_MYVISION) {
        if (!jcv_media_load(gameinfo.data, gameinfo.size))
            return 0;
        vidinfo.aspect = ASPECT_NTSC;
        audinfo.spf = (SAMPLERATE / FRAMERATE) * CHANNELS;
        jg_cb_frametime(FRAMERATE);
    }

    jcv_input_setup();

    return 1;
}

int jg_game_unload(void) {
    char savename[292];
    snprintf(savename, sizeof(savename),
        "%s/%s.srm", pathinfo.save, gameinfo.name);
    int srmstat = jcv_savedata_save((const char*)savename);

    if (srmstat == JCV_SAVE_SUCCEED)
        jg_cb_log(JG_LOG_DBG, "SRAM Saved: %s\n", savename);
    else if (srmstat == JCV_SAVE_NONE)
        jg_cb_log(JG_LOG_DBG, "Cartridge does not contain SRAM\n");
    else
        jg_cb_log(JG_LOG_DBG, "SRAM Save Failed: %s\n", savename);

    return 1;
}

int jg_state_load(const char *filename) {
    return jcv_state_load(filename);
}

void jg_state_load_raw(const void *data) {
    jcv_state_load_raw(data);
}

int jg_state_save(const char *filename) {
    return jcv_state_save(filename);
}

const void* jg_state_save_raw(void) {
    return jcv_state_save_raw();
}

size_t jg_state_size(void) {
    return jcv_state_size();
}

void jg_media_select(void) {
}

void jg_media_insert(void) {
}

void jg_cheat_clear(void) {
}

void jg_cheat_set(const char *code) {
    (void)code;
}

void jg_rehash(void) {
    jcv_video_set_palette_tms9918(settings_jcv[PALETTE_TMS9918].val);
    jcv_input_setup();
}

void jg_data_push(uint32_t type, int port, const void *ptr, size_t size) {
    if (type || port || ptr || size) { }
}

jg_coreinfo_t* jg_get_coreinfo(const char *subsys) {
    if (!strcmp(subsys, "crvision")) {
        sys = JCV_SYS_CRVISION;
        return &coreinfo_crvision;
    }
    else if (!strcmp(subsys, "myvision")) {
        sys = JCV_SYS_MYVISION;
        return &coreinfo_myvision;
    }
    return &coreinfo_coleco;
}

jg_videoinfo_t* jg_get_videoinfo(void) {
    return &vidinfo;
}

jg_audioinfo_t* jg_get_audioinfo(void) {
    return &audinfo;
}

jg_inputinfo_t* jg_get_inputinfo(int port) {
    return &inputinfo[port];
}

jg_setting_t* jg_get_settings(size_t *numsettings) {
    *numsettings = sizeof(settings_jcv) / sizeof(jg_setting_t);
    return settings_jcv;
}

void jg_setup_video(void) {
    jcv_video_set_buffer(vidinfo.buf);

    if (settings_jcv[MASKOVERSCAN].val) {
        vidinfo.w = 256;
        vidinfo.h = 192;
        vidinfo.x = vidinfo.y = 8;
    }
}

void jg_setup_audio(void) {
    jcv_audio_set_buffer(audinfo.buf);
}

void jg_set_inputstate(jg_inputstate_t *ptr, int port) {
    input_device[port] = ptr;
}

void jg_set_gameinfo(jg_fileinfo_t info) {
    gameinfo = info;
}

void jg_set_auxinfo(jg_fileinfo_t info, int index) {
    if (index)
        return;
    biosinfo = info;
}

void jg_set_paths(jg_pathinfo_t paths) {
    pathinfo = paths;
}
