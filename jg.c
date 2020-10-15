/*
 * Copyright (c) 2020 Rupert Carmichael
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stdio.h>
#include <stdint.h>

#include <jg/jg.h>
#include <jg/jg_coleco.h>

#include "jcv.h"
#include "jcv_memio.h"
#include "jcv_mixer.h"
#include "jcv_vdp.h"

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
static jg_cb_settings_read_t jg_cb_settings_read;

static jg_coreinfo_t coreinfo = {
    "jollycv", "JollyCV", VERSION, "coleco", NUMINPUTS, 0x00
};

static jg_videoinfo_t vidinfo = {
    JG_PIXFMT_XRGB8888, // pixfmt
    CV_VDP_WIDTH,       // wmax
    CV_VDP_HEIGHT,      // hmax
    CV_VDP_WIDTH,       // w
    CV_VDP_HEIGHT,      // h
    0,                  // x
    0,                  // y
    CV_VDP_WIDTH,       // p
    4.0/3.0,            // aspect
    NULL                // buf
};

static jg_audioinfo_t audinfo = {
    JG_SAMPFMT_INT16,
    SAMPLERATE,
    CHANNELS,
    (SAMPLERATE / FRAMERATE) * CHANNELS,
    NULL
};

static jg_pathinfo_t pathinfo;
static jg_gameinfo_t gameinfo;
static jg_inputinfo_t inputinfo[NUMINPUTS];
static jg_inputstate_t *input_device[NUMINPUTS];

// Emulator settings
static jg_setting_t settings_jcv[] = {   // name, default, min, max
    { "palette", 0, 0, 1 }, // 0 = TeaTime, 1 = SYoung
    { "rsqual", 3, 0, 10 }, // N = Resampler Quality
    { "region", REGION_NTSC, REGION_NTSC, REGION_PAL }, // 0 = NTSC, 1 = PAL
};

enum {
    PALETTE,
    RSQUAL,
    REGION,
};

// There may be more PAL-only releases, but many dumps marked as "Europe" are
// actually worldwide releases - for now, don't use this list for anything
/*static const char *gamedb_pal[] = { // PAL games
    "3ae1ddd596faaced37274dd407d5523c", // Cosmic Crisis (Europe)
    "8f1f9e8267b51d8b9b29fb62f0c050ec", // Meteoric Shower (Europe)
    "8aabed060476fde3cc706c6463f02980", // Super Action Football (Europe)
    "a31facd8adc1134942d9f4102dd3fa9f", // Victory (Europe)
};*/

// Some games may have optional support (Centipede, Omega Race)
/*static const char *gamedb_roller[] = { // Roller Controller
    "7cdc148dff40389fa1ad012d4734ceed", // Slither (USA, Europe)
    "a31facd8adc1134942d9f4102dd3fa9f", // Victory (Europe)
    "200aa603996bfd2734e353098ebe8dd5", // Victory (USA)
};*/

// Some games may have optional support
/*static const char *gamedb_wheel[] = { // Steering Wheel (Driving Controller)
    "ec72a0e3bebe07ba631a8dcb750c1591", // Destructor (USA, Europe)
    "dbd4f21702be17775e84b2fb6c534c94", // Dukes of Hazzard, The (USA)
    "bd905555983c05456ab81ea154a570b1", // Fall Guy (Europe) (Proto)
    "6f146d9bd3f64bbc006a761f59e2a1cf", // Turbo (USA, Europe)
};*/

/*static const char *gamedb_sketchpad[] = { // Only one game seems to use this
    "a46d20d65533ed979933fc1cfe6c0ad7", // Super Sketch - Sketch Master (USA)
};*/

static void jcv_settings_read(void) {
    jg_cb_settings_read(settings_jcv,
        sizeof(settings_jcv) / sizeof(jg_setting_t));
}

static uint16_t cv_input_map[] = {
    CV_INPUT_U, CV_INPUT_D, CV_INPUT_L, CV_INPUT_R, CV_INPUT_FL, CV_INPUT_FR,
    CV_INPUT_1, CV_INPUT_2, CV_INPUT_3, CV_INPUT_4, CV_INPUT_5, CV_INPUT_6,
    CV_INPUT_7, CV_INPUT_8, CV_INPUT_9, CV_INPUT_0, CV_INPUT_STR, CV_INPUT_PND
};

static uint16_t jcv_input_poll(int port) {
    uint16_t b = 0x0000;
    
    for (int i = 0; i < NDEFS_COLECOPAD; i++)
        if (input_device[port]->button[i]) b |= cv_input_map[i];
    
    return b;
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

void jg_set_cb_settings_read(jg_cb_settings_read_t func) {
    jg_cb_settings_read = func;
}

int jg_init(void) {
    jcv_settings_read();
    jcv_input_set_callback(&jcv_input_poll);
    jcv_mixer_set_callback(jg_cb_audio);
    jcv_mixer_set_rate(SAMPLERATE);
    jcv_mixer_set_rsqual(settings_jcv[RSQUAL].value);
    jcv_vdp_set_palette(settings_jcv[PALETTE].value);
    jcv_set_region(settings_jcv[REGION].value);
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
    // Load the BIOS
    char biospath[256];
    snprintf(biospath, sizeof(biospath), "%s/coleco.rom", pathinfo.bios);
    if (!jcv_bios_load(biospath))
        jg_cb_log(JG_LOG_ERR, "Failed to load bios %s\n", biospath);
    
    // Load the ROM
    if (!jcv_rom_load(gameinfo.data, gameinfo.size))
        return 0;
    
    // Set up input devices
    inputinfo[0] = (jg_inputinfo_t){ JG_INPUT_CONTROLLER, 0,
        "colecopad1", "ColecoVision Paddle",
        defs_colecopad, 0, NDEFS_COLECOPAD };
    
    inputinfo[1] = (jg_inputinfo_t){ JG_INPUT_CONTROLLER, 1,
        "colecopad2", "ColecoVision Paddle",
        defs_colecopad, 0, NDEFS_COLECOPAD };
    
    // Set the samples per frame and frame timing depending on region
    if (settings_jcv[REGION].value) { // PAL mode
        vidinfo.aspect = ASPECT_PAL;
        audinfo.spf = (SAMPLERATE / FRAMERATE_PAL) * CHANNELS;
        jg_cb_frametime(FRAMERATE_PAL);
    }
    else { // NTSC mode
        jg_cb_frametime(FRAMERATE);
    }
    return 1;
}

int jg_game_unload(void) {
    return 1;
}

int jg_state_load(const char *filename) {
    return jcv_state_load(filename);
}

int jg_state_save(const char *filename) {
    return jcv_state_save(filename);
}

void jg_media_select(void) {
}

void jg_media_insert(void) {
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

void jg_setup_video(void) {
    jcv_vdp_set_buffer(vidinfo.buf);
}

void jg_setup_audio(void) {
    jcv_mixer_set_buffer(audinfo.buf);
}

void jg_set_inputstate(jg_inputstate_t *ptr, int port) {
    input_device[port] = ptr;
}

void jg_set_gameinfo(jg_gameinfo_t info) {
    gameinfo = info;
}

void jg_set_paths(jg_pathinfo_t paths) {
    pathinfo = paths;
}
