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

#define JCV_COLECO_INPUT_U          (1 << 0)
#define JCV_COLECO_INPUT_D          (1 << 1)
#define JCV_COLECO_INPUT_L          (1 << 2)
#define JCV_COLECO_INPUT_R          (1 << 3)
#define JCV_COLECO_INPUT_FL         (1 << 4)
#define JCV_COLECO_INPUT_FR         (1 << 5)
#define JCV_COLECO_INPUT_1          (1 << 6)
#define JCV_COLECO_INPUT_2          (1 << 7)
#define JCV_COLECO_INPUT_3          (1 << 8)
#define JCV_COLECO_INPUT_4          (1 << 9)
#define JCV_COLECO_INPUT_5          (1 << 10)
#define JCV_COLECO_INPUT_6          (1 << 11)
#define JCV_COLECO_INPUT_7          (1 << 12)
#define JCV_COLECO_INPUT_8          (1 << 13)
#define JCV_COLECO_INPUT_9          (1 << 14)
#define JCV_COLECO_INPUT_0          (1 << 15)
#define JCV_COLECO_INPUT_STAR       (1 << 16)
#define JCV_COLECO_INPUT_POUND      (1 << 17)
#define JCV_COLECO_INPUT_Y          (1 << 18)
#define JCV_COLECO_INPUT_O          (1 << 19)
#define JCV_COLECO_INPUT_P          (1 << 20)
#define JCV_COLECO_INPUT_B          (1 << 22)
#define JCV_COLECO_INPUT_SP_PLUS    (1 << 22)
#define JCV_COLECO_INPUT_SP_MINUS   (1 << 23)
#define JCV_COLECO_INPUT_IRQ        (1 << 24)

void jcv_input_set_callback_coleco(unsigned (*)(const void*, int), void*);

#define MYV_INPUT_1                 (1 << 0)
#define MYV_INPUT_2                 (1 << 1)
#define MYV_INPUT_3                 (1 << 2)
#define MYV_INPUT_4                 (1 << 3)
#define MYV_INPUT_5                 (1 << 4)
#define MYV_INPUT_6                 (1 << 5)
#define MYV_INPUT_7                 (1 << 6)
#define MYV_INPUT_8                 (1 << 7)
#define MYV_INPUT_9                 (1 << 8)
#define MYV_INPUT_10                (1 << 9)
#define MYV_INPUT_11                (1 << 10)
#define MYV_INPUT_12                (1 << 11)
#define MYV_INPUT_13                (1 << 12)
#define MYV_INPUT_14                (1 << 13)
#define MYV_INPUT_A                 (1 << 14)
#define MYV_INPUT_B                 (1 << 15)
#define MYV_INPUT_C                 (1 << 16)
#define MYV_INPUT_D                 (1 << 17)
#define MYV_INPUT_E                 (1 << 18)

void jcv_input_set_callback_myvision(unsigned (*)(const void*), void*);

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
    JCV_SAVE_SUCCESS,
    JCV_SAVE_NONE
};

int jcv_savedata_load(const char *filename);
int jcv_savedata_save(const char *filename);

int jcv_bios_load(void *data, size_t size);
int jcv_bios_load_file(const char *biospath);

int jcv_media_load(void *data, size_t size);

enum jcv_memtype {
    JCV_MEM_SYSTEM,
    JCV_MEM_VRAM,
    JCV_MEM_SAVEDATA
};

void* jcv_memory_get_data(unsigned type);
size_t jcv_memory_get_size(unsigned type);

#ifdef __cplusplus
}
#endif
#endif
