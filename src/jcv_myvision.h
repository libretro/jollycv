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

#ifndef JCV_MYVISION_H
#define JCV_MYVISION_H

#define MYV_INPUT_1     0x80
#define MYV_INPUT_2     0x80
#define MYV_INPUT_3     0x80
#define MYV_INPUT_4     0x80
#define MYV_INPUT_5     0x40
#define MYV_INPUT_6     0x40
#define MYV_INPUT_7     0x40
#define MYV_INPUT_8     0x40
#define MYV_INPUT_9     0x20
#define MYV_INPUT_10    0x20
#define MYV_INPUT_11    0x20
#define MYV_INPUT_12    0x20
#define MYV_INPUT_13    0x08
#define MYV_INPUT_14    0x08
#define MYV_INPUT_A     0x08
#define MYV_INPUT_B     0x08
#define MYV_INPUT_C     0x10
#define MYV_INPUT_D     0x10
#define MYV_INPUT_E     0x10

void jcv_myvision_input_set_callback(uint8_t (*)(int));

void jcv_myvision_init(void);
void jcv_myvision_deinit(void);
void jcv_myvision_exec(void);

int jcv_myvision_rom_load(void*, size_t);

#endif
