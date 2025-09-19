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

#ifndef JCV_COLECO_H
#define JCV_COLECO_H

#include "jcv.h"

#define SIZE_CVBIOS SIZE_8K
#define SIZE_CVRAM SIZE_1K

#define COLECO_INPUT_U          0x00000001
#define COLECO_INPUT_D          0x00000002
#define COLECO_INPUT_L          0x00000004
#define COLECO_INPUT_R          0x00000008
#define COLECO_INPUT_FL         0x00000010
#define COLECO_INPUT_FR         0x00000020
#define COLECO_INPUT_1          0x00000040
#define COLECO_INPUT_2          0x00000080
#define COLECO_INPUT_3          0x00000100
#define COLECO_INPUT_4          0x00000200
#define COLECO_INPUT_5          0x00000400
#define COLECO_INPUT_6          0x00000800
#define COLECO_INPUT_7          0x00001000
#define COLECO_INPUT_8          0x00002000
#define COLECO_INPUT_9          0x00004000
#define COLECO_INPUT_0          0x00008000
#define COLECO_INPUT_STAR       0x00010000
#define COLECO_INPUT_POUND      0x00020000
#define COLECO_INPUT_Y          0x00040000
#define COLECO_INPUT_O          0x00080000
#define COLECO_INPUT_P          0x00100000
#define COLECO_INPUT_B          0x00200000
#define COLECO_INPUT_SP_PLUS    0x00400000
#define COLECO_INPUT_SP_MINUS   0x00800000
#define COLECO_INPUT_IRQ        0x01000000

typedef enum _cv_cart {
    CART_NORMAL,
    CART_MEGA,
    CART_ACTIVISION,
    CART_SRAM
} cv_cart;

typedef struct _cv_sys_t {
    uint8_t ram[SIZE_CVRAM]; // System RAM
    uint8_t sgmram[SIZE_32K]; // Super Game Module RAM
    uint8_t cseg; // Controller Strobe Segment
    uint16_t ctrl[2]; // Controller Input state
} cv_sys_t;

void jcv_coleco_input_set_callback(unsigned (*)(const void*, int), void*);

void jcv_coleco_init(void);
void jcv_coleco_deinit(void);

int jcv_coleco_bios_load_file(const char*);
int jcv_coleco_bios_load(void*, size_t);
int jcv_coleco_rom_load(void*, size_t);
void jcv_coleco_set_carttype(unsigned, unsigned);

int jcv_coleco_sram_load(const char*);
int jcv_coleco_sram_save(const char*);

void jcv_coleco_set_region(unsigned);

void jcv_coleco_exec(void);

#endif
