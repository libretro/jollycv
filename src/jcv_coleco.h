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

#include "jcv_sizes.h"

#define SIZE_CVBIOS SIZE_8K
#define SIZE_CVRAM SIZE_1K

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

void* jcv_coleco_get_ram_data(void);

size_t jcv_coleco_state_size(void);
void jcv_coleco_state_load_raw(const void*);
const void* jcv_coleco_state_save_raw(void);

void jcv_coleco_init(void);
void jcv_coleco_deinit(void);

int jcv_coleco_bios_load(void*, size_t);
int jcv_coleco_rom_load(void*, size_t);
void jcv_coleco_set_carttype(unsigned, unsigned);

uint8_t* jcv_coleco_get_save_data(void);
size_t jcv_coleco_get_save_size(void);

void jcv_coleco_set_region(unsigned);

void jcv_coleco_exec(void);

#endif
