/*
Copyright (c) 2020-2022 Rupert Carmichael
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

#include "jcv.h"
#include "jcv_memio.h"
#include "jcv_mixer.h"
#include "jcv_vdp.h"
#include "jcv_z80.h"

#define SAMPLERATE 48000
#define FRAMERATE 60
#define FRAMERATE_PAL 50
#define ASPECT_PAL 1.4257812
#define CHANNELS 1
#define NUMINPUTS 2

static jg_cb_audio_t jg_cb_audio;
static jg_cb_frametime_t jg_cb_frametime;
static jg_cb_log_t jg_cb_log;
static jg_cb_rumble_t jg_cb_rumble;

static jg_coreinfo_t coreinfo = {
    "jollycv", "JollyCV", VERSION, "coleco", NUMINPUTS, 0x00
};

static jg_videoinfo_t vidinfo = {
    JG_PIXFMT_XRGB8888,         // pixfmt
    CV_VDP_WIDTH_OVERSCAN,      // wmax
    CV_VDP_HEIGHT_OVERSCAN,     // hmax
    CV_VDP_WIDTH_OVERSCAN - 12, // w
    CV_VDP_HEIGHT_OVERSCAN,     // h
    6,                          // x
    0,                          // y
    CV_VDP_WIDTH_OVERSCAN,      // p
    4.0/3.0,                    // aspect
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
    { "input", "Input Devices",
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
    { "palette", "Colour Palette",
      "0 = TeaTime, 1 = SYoung",
      "Select either the beautiful or widely used colour palette",
      0, 0, 1, 0
    },
    { "rsqual", "Resampler Quality",
      "N = Resampler Quality",
      "Quality level for the internal resampler",
      3, 0, 10, JG_SETTING_RESTART
    },
    { "region", "Region",
      "0 = NTSC, 1 = PAL",
      "Select the region",
      REGION_NTSC, REGION_NTSC, REGION_PAL, JG_SETTING_RESTART
    }
};

enum {
    INPUT,
    MASKOVERSCAN,
    PALETTE,
    RSQUAL,
    REGION,
};

typedef struct _dbentry_t {
    const char *md5; int type;
} dbentry_t;

static dbentry_t gamedb[] = {
    // Super Action Controller
    // Front Line (USA, Europe)
        { "4520ee5d8d0fcf151a3332966f7ebda0", JG_COLECO_SAC },
    // Front Line (USA, Europe) (Green Version)
        { "d145de191e3f694c7f0920787ccbda48", JG_COLECO_SAC },
    // Rocky - Super Action Boxing (USA, Europe)
        { "d35fdb81f4a733925b0a33dfb53d9d78", JG_COLECO_SAC },
    // Spy Hunter (USA)
        { "f96a21f920e889d1e21abbf00f4d381d", JG_COLECO_SAC },
    // Spy Hunter (USA) (Beta)
        { "7da9f2fda17e1e34a41b180d1ceb0c37", JG_COLECO_SAC },
    // Star Trek - Strategic Operations Simulator (USA)
        { "45006eaf52ee16ddcadd1dca68b265c8", JG_COLECO_SAC },
    // Super Action Baseball (USA)
        { "4c4b25a93301e59b86decb0df7a0ee51", JG_COLECO_SAC },
    // Super Action Football (Europe)
        { "8aabed060476fde3cc706c6463f02980", JG_COLECO_SAC },
    // Super Action Football (USA)
        { "bee90a110d14b29d2e64f0ff0f303bc6", JG_COLECO_SAC },

    // Roller Controller
    // Slither (USA, Europe)
        { "7cdc148dff40389fa1ad012d4734ceed", JG_COLECO_ROLLER },
    // Victory (Europe)
        { "a31facd8adc1134942d9f4102dd3fa9f", JG_COLECO_ROLLER },
    // Victory (USA)
        { "200aa603996bfd2734e353098ebe8dd5", JG_COLECO_ROLLER },

    // Steering Wheel
    // Destructor (USA, Europe)
        { "ec72a0e3bebe07ba631a8dcb750c1591", JG_COLECO_WHEEL },
    // Dukes of Hazzard, The (USA)
        { "dbd4f21702be17775e84b2fb6c534c94", JG_COLECO_WHEEL },
    // Turbo (USA, Europe)
        { "6f146d9bd3f64bbc006a761f59e2a1cf", JG_COLECO_WHEEL },

    // Super Sketch
    //Super Sketch - Sketch Master (USA)
        { "a46d20d65533ed979933fc1cfe6c0ad7", JG_COLECO_SKETCH },
};

// There may be more PAL-only releases, but many dumps marked as "Europe" are
// actually worldwide releases - for now, don't use this list for anything
/*static const char *gamedb_pal[] = { // PAL games
    "3ae1ddd596faaced37274dd407d5523c", // Cosmic Crisis (Europe)
    "8f1f9e8267b51d8b9b29fb62f0c050ec", // Meteoric Shower (Europe)
    "8aabed060476fde3cc706c6463f02980", // Super Action Football (Europe)
    "a31facd8adc1134942d9f4102dd3fa9f", // Victory (Europe)
};*/

// ColecoVision Paddle
static uint16_t cv_input_map[] = {
    CV_INPUT_U, CV_INPUT_D, CV_INPUT_L, CV_INPUT_R, CV_INPUT_FL, CV_INPUT_FR,
    CV_INPUT_1, CV_INPUT_2, CV_INPUT_3, CV_INPUT_4, CV_INPUT_5, CV_INPUT_6,
    CV_INPUT_7, CV_INPUT_8, CV_INPUT_9, CV_INPUT_0, CV_INPUT_STR, CV_INPUT_PND
};

static uint16_t jcv_input_poll(int port) {
    uint16_t b = 0x8080; // Always preset bit 7 for both segments

    for (int i = 0; i < NDEFS_COLECOPAD; ++i)
        if (input_device[port]->button[i]) b |= cv_input_map[i];

    return b;
}

// Unconnected Port
static uint16_t jcv_input_poll_null(int port) {
    if (port) { }
    return 0x8080;
}

// ColecoVision Roller Controller
static uint16_t jcv_input_poll_roller(int port) {
    uint16_t b = 0x8080; // Always preset bit 7 for both segments

    for (int i = 0; i < NDEFS_COLECOPAD; ++i)
        if (input_device[port]->button[i]) b |= cv_input_map[i];

    int rel = input_device[0]->rel[port] / 4;
    input_device[0]->rel[port] -= rel;

    if (rel < 0) {
        b |= port ? CV_INPUT_SP : CV_INPUT_SM;
        jcv_z80_irq(0);
    }
    else if (rel > 0) {
        b |= port ? CV_INPUT_SM : CV_INPUT_SP;
        jcv_z80_irq(0);
    }

    return b;
}

// ColecoVision Super Action Controller
static uint16_t cv_input_map_sac[] = {
    CV_INPUT_U, CV_INPUT_D, CV_INPUT_L, CV_INPUT_R,
    CV_INPUT_Y, CV_INPUT_O, CV_INPUT_P, CV_INPUT_B,
    CV_INPUT_1, CV_INPUT_2, CV_INPUT_3, CV_INPUT_4, CV_INPUT_5, CV_INPUT_6,
    CV_INPUT_7, CV_INPUT_8, CV_INPUT_9, CV_INPUT_0, CV_INPUT_STR, CV_INPUT_PND,
};

static uint16_t jcv_input_poll_sac(int port) {
    uint16_t b = 0x8080; // Always preset bit 7 for both segments

    for (int i = 0; i < NDEFS_COLECOSAC - 2; ++i)
        if (input_device[port]->button[i]) b |= cv_input_map_sac[i];

    // Speed Rollers
    if (input_device[port]->button[20]) {
        b |= CV_INPUT_SP;
        if (input_device[port]->button[20]++ > 1) {
            input_device[port]->button[20] = 1;
            jcv_z80_irq(0);
        }
    }
    if (input_device[port]->button[21]) {
        b |= CV_INPUT_SM;
        if (input_device[port]->button[21]++ > 1) {
            input_device[port]->button[21] = 1;
            jcv_z80_irq(0);
        }
    }

    return b;
}

// ColecoVision Expansion Module #2 - Steering Wheel
static uint16_t cv_input_map_wheel[] = {
    CV_INPUT_U, CV_INPUT_D, CV_INPUT_L, CV_INPUT_R,
    CV_INPUT_1, CV_INPUT_2, CV_INPUT_3, CV_INPUT_4, CV_INPUT_5, CV_INPUT_6,
    CV_INPUT_7, CV_INPUT_8, CV_INPUT_9, CV_INPUT_0, CV_INPUT_STR, CV_INPUT_PND,
};

static uint16_t jcv_input_poll_wheel(int port) {
    uint16_t b = 0x8080; // Always preset bit 7 for both segments

    if (port == 0) { // Steering Wheel and Pedal on first port
        int rel = input_device[0]->rel[port] / 3;
        input_device[0]->rel[0] -= rel;

        if (rel < 0) {
            b |= CV_INPUT_SM;
            jcv_z80_irq(0);
        }
        else if (rel > 0) {
            b |= CV_INPUT_SP;
            jcv_z80_irq(0);
        }

        // Pedal uses the same signal as the FireL on the standard paddle
        if (input_device[0]->button[0]) b |= CV_INPUT_FL;
    }
    else if (port == 1) { // Numpad and Stick on second port
        for (int i = 1; i < NDEFS_COLECOWHEEL; ++i)
            // Hardcode to input device 0 as defined in the frontend
            if (input_device[0]->button[i]) b |= cv_input_map_wheel[i - 1];
    }

    return b;
}

static void jcv_input_setup(void) {
    int itype = settings_jcv[INPUT].val;

    if (!itype) {
        for (size_t i = 0; i < (sizeof(gamedb) / sizeof(dbentry_t)); ++i) {
            if (!strcmp(gamedb[i].md5, gameinfo.md5)) {
                itype = gamedb[i].type;
                break;
            }
        }
    }

    switch (itype) {
        default: case 0: case JG_COLECO_PAD: {
            inputinfo[0] = jg_coleco_inputinfo(0, JG_COLECO_PAD);
            inputinfo[1] = jg_coleco_inputinfo(1, JG_COLECO_PAD);
            jcv_input_set_callback(&jcv_input_poll);
            break;
        }
        case JG_COLECO_ROLLER: {
            inputinfo[0] = jg_coleco_inputinfo(0, JG_COLECO_ROLLER);
            inputinfo[1] = jg_coleco_inputinfo(1, JG_COLECO_ROLLER);
            jcv_input_set_callback(&jcv_input_poll_roller);
            break;
        }
        case JG_COLECO_SAC: {
            inputinfo[0] = jg_coleco_inputinfo(0, JG_COLECO_SAC);
            inputinfo[1] = jg_coleco_inputinfo(1, JG_COLECO_SAC);
            jcv_input_set_callback(&jcv_input_poll_sac);
            break;
        }
        case JG_COLECO_SKETCH: {
            inputinfo[0] = jg_coleco_inputinfo(0, JG_COLECO_UNCONNECTED);
            inputinfo[1] = jg_coleco_inputinfo(1, JG_COLECO_UNCONNECTED);
            jcv_input_set_callback(&jcv_input_poll_null);
            jg_cb_log(JG_LOG_WRN, "Super Sketch not supported\n");
            break;
        }
        case JG_COLECO_WHEEL: {
            inputinfo[0] = jg_coleco_inputinfo(0, JG_COLECO_WHEEL);
            inputinfo[1] = jg_coleco_inputinfo(1, JG_COLECO_UNCONNECTED);
            jcv_input_set_callback(&jcv_input_poll_wheel);
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
}

void jg_set_cb_rumble(jg_cb_rumble_t func) {
    jg_cb_rumble = func;
}

int jg_init(void) {
    jcv_input_set_callback(&jcv_input_poll);
    jcv_mixer_set_callback(jg_cb_audio);
    jcv_mixer_set_rate(SAMPLERATE);
    jcv_mixer_set_rsqual(settings_jcv[RSQUAL].val);
    jcv_vdp_set_palette(settings_jcv[PALETTE].val);
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
        char biospath[256];
        snprintf(biospath, sizeof(biospath), "%s/coleco.rom", pathinfo.bios);
        if (!jcv_bios_load_file(biospath))
            jg_cb_log(JG_LOG_ERR, "Failed to load bios %s\n", biospath);
    }

    // Load the ROM
    if (!jcv_rom_load(gameinfo.data, gameinfo.size))
        return 0;

    // Set the samples per frame and frame timing depending on region
    if (settings_jcv[REGION].val) { // PAL mode
        vidinfo.aspect = ASPECT_PAL;
        audinfo.spf = (SAMPLERATE / FRAMERATE_PAL) * CHANNELS;
        jg_cb_frametime(FRAMERATE_PAL);
    }
    else { // NTSC mode
        jg_cb_frametime(FRAMERATE);
    }

    jcv_input_setup();

    return 1;
}

int jg_game_unload(void) {
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
    if (code) { }
}

void jg_rehash(void) {
    jcv_vdp_set_palette(settings_jcv[PALETTE].val);
    jcv_input_setup();
}

void jg_data_push(uint32_t type, int port, const void *ptr, size_t size) {
    if (type || port || ptr || size) { }
}

jg_coreinfo_t* jg_get_coreinfo(const char *sys) {
    if (sys) { } // Unused
    return &coreinfo;
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
    jcv_vdp_set_buffer(vidinfo.buf);

    if (settings_jcv[MASKOVERSCAN].val) {
        vidinfo.w = CV_VDP_WIDTH;
        vidinfo.h = CV_VDP_HEIGHT;
        vidinfo.x = vidinfo.y = CV_VDP_OVERSCAN;
    }
}

void jg_setup_audio(void) {
    jcv_mixer_set_buffer(audinfo.buf);
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
