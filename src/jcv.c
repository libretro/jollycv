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

#include <stdint.h>
#include <stddef.h>

#include "jcv.h"
#include "jcv_coleco.h"
#include "jcv_mixer.h"
#include "jcv_vdp.h"
#include "jcv_z80.h"
#include "ay38910.h"
#include "sn76489.h"

/* NTSC Timing
Z80 cycles per audio sample at 48000Hz (16 CPU cycles per PSG cycle):
    (3579545 / x) / 16 = 48000
    x = 4.6608659

Z80 cycles per frame (2 CPU cycles per 3 VDP cycles):
    89603.5 * 2/3 = 59735.66667

Z80 cycles per scanline:
    59735.66667 / 262 = 227.99873 (~228)

VDP cycles per frame:
    342 cycles per line, 262 lines, skip final cycle of frame every other frame
    89604 or 89603 (89603.5)

PSG cycles per frame:
    59735.66667 / 16 = 3733.4792 (~224KHz)
*/

void (*jcv_exec)(void);

// Set the region
void jcv_set_region(unsigned region) {
    // 313 scanlines for PAL, 262 scanlines for NTSC (192 visible for both)
    jcv_coleco_set_region(region);
    jcv_mixer_set_region(region);
    jcv_vdp_set_region(region);
}

// Initialize
void jcv_init(void) {
    jcv_coleco_init();
    jcv_mixer_init();
    jcv_vdp_init();
    jcv_z80_init();

    jcv_exec = &jcv_coleco_exec;
}

// Deinitialize
void jcv_deinit(void) {
    jcv_coleco_deinit();
    jcv_mixer_deinit();
}

// Reset the system
void jcv_reset(int hard) {
    (void)hard; // Currently unused
    jcv_coleco_init(); // Init does the same thing reset needs to do
    jcv_vdp_init();
    jcv_z80_reset();
}
