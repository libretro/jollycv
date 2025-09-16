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

#ifndef JCV_CRVISION_H
#define JCV_CRVISION_H

#include "jcv.h"

#define SIZE_CRVBIOS SIZE_2K

// Cardinal and Diagonal directions
#define CRV_INPUT_UP        0x08
#define CRV_INPUT_DOWN      0x02
#define CRV_INPUT_LEFT      0x20
#define CRV_INPUT_RIGHT     0x04
#define CRV_INPUT_UPLEFT    0x30
#define CRV_INPUT_UPRIGHT   0x44
#define CRV_INPUT_DOWNLEFT  0x42
#define CRV_INPUT_DOWNRIGHT 0x03

#define CRV_INPUT_FIRE      0x80 // All Fire buttons are on different strobes

// These keys are shared with the Fire buttons
#define CRV_INPUT_CNTL      0x80 // PA0
#define CRV_INPUT_SHIFT     0x80 // PA1
#define CRV_INPUT_TAB       0x80 // PA2
#define CRV_INPUT_MINUS     0x80 // PA3

#define CRV_INPUT_BACKSPACE 0x09
#define CRV_INPUT_SPACE     0x0c
#define CRV_INPUT_COLON     0x0a
#define CRV_INPUT_SEMICOLON 0x18
#define CRV_INPUT_RETN      0x09
#define CRV_INPUT_COMMA     0x28
#define CRV_INPUT_PERIOD    0x60
#define CRV_INPUT_SLASH     0x30

#define CRV_INPUT_1         0x0c
#define CRV_INPUT_2         0x30
#define CRV_INPUT_3         0x60
#define CRV_INPUT_4         0x28
#define CRV_INPUT_5         0x48
#define CRV_INPUT_6         0x50
#define CRV_INPUT_7         0x06
#define CRV_INPUT_8         0x42
#define CRV_INPUT_9         0x22
#define CRV_INPUT_0         0x12

#define CRV_INPUT_Q         0x18
#define CRV_INPUT_W         0x0c
#define CRV_INPUT_E         0x14
#define CRV_INPUT_R         0x24
#define CRV_INPUT_T         0x44
#define CRV_INPUT_Y         0x05
#define CRV_INPUT_U         0x03
#define CRV_INPUT_I         0x41
#define CRV_INPUT_O         0x21
#define CRV_INPUT_P         0x11
#define CRV_INPUT_A         0x11
#define CRV_INPUT_S         0x21
#define CRV_INPUT_D         0x41
#define CRV_INPUT_F         0x03
#define CRV_INPUT_G         0x05
#define CRV_INPUT_H         0x44
#define CRV_INPUT_J         0x24
#define CRV_INPUT_K         0x14
#define CRV_INPUT_L         0x0c
#define CRV_INPUT_Z         0x0a
#define CRV_INPUT_X         0x12
#define CRV_INPUT_C         0x22
#define CRV_INPUT_V         0x42
#define CRV_INPUT_B         0x06
#define CRV_INPUT_N         0x50
#define CRV_INPUT_M         0x48

void jcv_crvision_input_set_callback(uint8_t (*)(int));

int jcv_crvision_bios_load_file(const char*);
int jcv_crvision_bios_load(void*, size_t);
int jcv_crvision_rom_load(void*, size_t);

uint8_t jcv_crvision_mem_rd(uint16_t);
void jcv_crvision_mem_wr(uint16_t, uint8_t);

void jcv_crvision_exec(void);
void jcv_crvision_init(void);
void jcv_crvision_deinit(void);

#endif
