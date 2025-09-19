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

// An idiot admires complexity, a genius admires simplicity - Terry A. Davis

#ifndef JCV_H
#define JCV_H

#ifdef __cplusplus
extern "C" {
#endif

enum jcv_loglevel {
    JCV_LOG_DBG,
    JCV_LOG_INF,
    JCV_LOG_WRN,
    JCV_LOG_ERR,
    JCV_LOG_SCR
};

void jcv_log_set_callback(void (*cb)(int, const char*, ...));
void jcv_log(int level, const char *fmt, ...);

#define JCV_REGION_NTSC             0
#define JCV_REGION_PAL              1

unsigned jcv_get_region(void);
void jcv_set_region(unsigned r);

#define JCV_SYS_COLECO              0x00
#define JCV_SYS_CRVISION            0x01
#define JCV_SYS_MYVISION            0x02

unsigned jcv_get_system(void);
void jcv_set_system(unsigned s);

#define JCV_DB_COLECO_PAD           0x00000001
#define JCV_DB_COLECO_ROLLER        0x00000002
#define JCV_DB_COLECO_SAC           0x00000004
#define JCV_DB_COLECO_SKETCH        0x00000008
#define JCV_DB_COLECO_WHEEL         0x00000010

uint32_t jcv_get_dbflags(const char *md5);

#define JCV_COLECO_INPUT_U          0x00000001
#define JCV_COLECO_INPUT_D          0x00000002
#define JCV_COLECO_INPUT_L          0x00000004
#define JCV_COLECO_INPUT_R          0x00000008
#define JCV_COLECO_INPUT_FL         0x00000010
#define JCV_COLECO_INPUT_FR         0x00000020
#define JCV_COLECO_INPUT_1          0x00000040
#define JCV_COLECO_INPUT_2          0x00000080
#define JCV_COLECO_INPUT_3          0x00000100
#define JCV_COLECO_INPUT_4          0x00000200
#define JCV_COLECO_INPUT_5          0x00000400
#define JCV_COLECO_INPUT_6          0x00000800
#define JCV_COLECO_INPUT_7          0x00001000
#define JCV_COLECO_INPUT_8          0x00002000
#define JCV_COLECO_INPUT_9          0x00004000
#define JCV_COLECO_INPUT_0          0x00008000
#define JCV_COLECO_INPUT_STAR       0x00010000
#define JCV_COLECO_INPUT_POUND      0x00020000
#define JCV_COLECO_INPUT_Y          0x00040000
#define JCV_COLECO_INPUT_O          0x00080000
#define JCV_COLECO_INPUT_P          0x00100000
#define JCV_COLECO_INPUT_B          0x00200000
#define JCV_COLECO_INPUT_SP_PLUS    0x00400000
#define JCV_COLECO_INPUT_SP_MINUS   0x00800000
#define JCV_COLECO_INPUT_IRQ        0x01000000

void jcv_audio_set_buffer(void *buf);
void jcv_audio_set_callback(void (*cb)(const void*, size_t), void *udata);
void jcv_audio_set_rate(size_t rate);
void jcv_audio_set_rsqual(unsigned rsqual);

#define JCV_VIDEO_WIDTH_MAX        272
#define JCV_VIDEO_HEIGHT_MAX       208

void jcv_video_set_buffer(uint32_t *buf);
void jcv_video_set_palette_tms9918(unsigned p);

void jcv_init(void);
void jcv_deinit(void);
void jcv_reset(int hard);

void jcv_exec(void);

int jcv_state_load(const char *filename);
int jcv_state_save(const char *filename);
size_t jcv_state_size(void);
void jcv_state_load_raw(const void *s);
const void* jcv_state_save_raw(void);

enum jcv_saveresult {
    JCV_SAVE_FAIL,
    JCV_SAVE_SUCCEED,
    JCV_SAVE_NONE
};

int jcv_savedata_load(const char *filename);
int jcv_savedata_save(const char *filename);
uint8_t* jcv_get_savedata(void);
size_t jcv_get_savesize(void);

#ifdef __cplusplus
}
#endif
#endif
