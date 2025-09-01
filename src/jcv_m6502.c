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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

//#include "jcv_crvision.h"
#include "jcv_serial.h"
#include "jcv_m6502.h"

#include "m6502.h"

static m6502 m65ctx;

static uint32_t extracycs = 0;
static uint32_t delaycycs = 0;

// Memory Read
static uint8_t read_byte(void *userdata, uint16_t addr) {
    (void)userdata;
    //return jcv_crvision_mem_rd(addr);
    return 0xff;
}

// Memory Write
static void write_byte(void *userdata, uint16_t addr, uint8_t data) {
    (void)userdata;
    //jcv_crvision_mem_wr(addr, data);
}

// Store extra cycle count
void jcv_m6502_cyc_store(uint32_t cycs) {
    extracycs = cycs;
}

// Retrieve stored extra cycle count
uint32_t jcv_m6502_cyc_restore(void) {
    uint32_t ret = extracycs;
    extracycs = 0;
    return ret;
}

// Initialize the 6502
void jcv_m6502_init(void) {
    m6502_init(&m65ctx);
    m65ctx.read_byte = &read_byte;
    m65ctx.write_byte = &write_byte;
}

// Reset the 6502
void jcv_m6502_reset(void) {
    m6502_gen_res(&m65ctx);
}

// Generate an Interrupt
void jcv_m6502_irq(void) {
    m6502_gen_irq(&m65ctx);
}

// Generate a Non-Maskable Interrupt
void jcv_m6502_nmi(void) {
    m6502_gen_nmi(&m65ctx);
}

// Delay the Z80's execution by a requested number of cycles
void jcv_m6502_delay(uint32_t delay) {
    delaycycs += delay;
}

uint32_t jcv_m6502_exec(void) {
    unsigned oldcyc = m65ctx.cyc;
    m6502_step(&m65ctx);
    //m6502_debug_output(&m65ctx);
    uint32_t retcyc = m65ctx.cyc - oldcyc;

    if (delaycycs) {
        retcyc += delaycycs;
        delaycycs = 0;
    }

    return retcyc;
}
