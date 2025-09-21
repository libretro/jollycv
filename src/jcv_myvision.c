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

// Nichibutsu My Vision

#include <stdint.h>
#include <stddef.h>

#include "jollycv.h"

#include "jcv_myvision.h"
#include "jcv_mixer.h"
#include "jcv_serial.h"

#include "jcv_z80.h"

#include "tms9918.h"

#define DIV_PSG 21          // PSG Clock Divider - (CPU Frequency) / 3 / 7
#define NUM_SCANLINES 262   // Total number of video scanlines
#define Z80_CYC_LINE 228    // Z80 CPU cycles per scanline (227.99873)

#define SIZE_STATE 18558
static uint8_t state[SIZE_STATE];
static uint32_t state_version = ('J' << 24) | ('C' << 16) | ('V' << 8) | 0x00;

static ay38910_t psg;       // PSG Context

static uint8_t ram[SIZE_MYVRAM];    // System RAM
static uint8_t *romdata = NULL;     // Game ROM
static size_t romsize = 0;          // Size of the ROM in bytes

// Frame execution related variables
static size_t psgcycs = 0;

static unsigned (*jcv_myvision_input_cb)(const void*); // Input callback
static void *udata_input = NULL; // Input callback userdata

void jcv_myvision_input_set_callback(unsigned (*cb)(const void*), void *u) {
    jcv_myvision_input_cb = cb;
    udata_input = u;
}

// My Vision strobes in columns
static unsigned myv_input_map[5] = {
          // 0x80  0x40   0x20  0x10
    0x80, //    1,    4,    2,    3
    0x40, //    5,    8,    6,    7
    0x20, //    9,    12,   10,   11
    0x08, //   13,    B,    14,   A
    0x10, //    C,           D,   E
};

static unsigned jcv_myvision_input_rd(int column) {
    unsigned pstate = jcv_myvision_input_cb(udata_input);
    unsigned bits = 0xff; // Active Low

    switch (column) {
        case 0x80: {
            if (pstate & MYV_INPUT_1) bits &= ~myv_input_map[0];
            if (pstate & MYV_INPUT_5) bits &= ~myv_input_map[1];
            if (pstate & MYV_INPUT_9) bits &= ~myv_input_map[2];
            if (pstate & MYV_INPUT_13) bits &= ~myv_input_map[3];
            if (pstate & MYV_INPUT_C) bits &= ~myv_input_map[4];
            break;
        }
        case 0x40: {
            if (pstate & MYV_INPUT_4) bits &= ~myv_input_map[0];
            if (pstate & MYV_INPUT_8) bits &= ~myv_input_map[1];
            if (pstate & MYV_INPUT_12) bits &= ~myv_input_map[2];
            if (pstate & MYV_INPUT_B) bits &= ~myv_input_map[3];
            break;
        }
        case 0x20: {
            if (pstate & MYV_INPUT_2) bits &= ~myv_input_map[0];
            if (pstate & MYV_INPUT_6) bits &= ~myv_input_map[1];
            if (pstate & MYV_INPUT_10) bits &= ~myv_input_map[2];
            if (pstate & MYV_INPUT_14) bits &= ~myv_input_map[3];
            if (pstate & MYV_INPUT_D) bits &= ~myv_input_map[4];
            break;
        }
        case 0x10: {
            if (pstate & MYV_INPUT_3) bits &= ~myv_input_map[0];
            if (pstate & MYV_INPUT_7) bits &= ~myv_input_map[1];
            if (pstate & MYV_INPUT_11) bits &= ~myv_input_map[2];
            if (pstate & MYV_INPUT_A) bits &= ~myv_input_map[3];
            if (pstate & MYV_INPUT_E) bits &= ~myv_input_map[4];
            break;
        }
    }

    return bits;
}

void* jcv_myvision_get_ram_data(void) {
    return &ram[0];
}

// Return the size of a state
size_t jcv_myvision_state_size(void) {
    return SIZE_STATE;
}

// Load raw state data into the running system
void jcv_myvision_state_load_raw(const void *sstate) {
    uint8_t *st = (uint8_t*)sstate;
    jcv_serial_begin();
    uint32_t ver = jcv_serial_pop32(st);
    (void)ver;
    jcv_serial_popblk(ram, st, SIZE_2K);
    ay38910_state_load(&psg, st);
    tms9918_state_load(st);
    jcv_z80_state_load(st);
}

// Snapshot the running state and return the address of the raw data
const void* jcv_myvision_state_save_raw(void) {
    jcv_serial_begin();
    jcv_serial_push32(state, state_version);
    jcv_serial_pushblk(state, ram, SIZE_2K);
    ay38910_state_save(&psg, state);
    tms9918_state_save(state);
    jcv_z80_state_save(state);
    return (const void*)state;
}

/* My Vision IO Map
   0x00 - PSG Register Select
   0x01 - PSG Register Write
   0x02 - PSG Register Read
*/
static uint8_t jcv_myvision_io_rd(uint16_t port) {
    if (port == 0x02) {
        /* The PSG's Port A/Port B are how input is handled. Port A (register
           index 14) is connected to the input circuitry, and will return
           values based on what "column" is set low in one (and only one) of
           the top 4 bits of Port B (register index 15). The input callback is
           called here for simplicity's sake.
        */
        if (psg.rlatch == 14) // Hijack the read in this case
            return jcv_myvision_input_rd(~psg.reg[15] & 0xf0);
        return ay38910_rd(&psg);
    }
    return 0xff;
}

static void jcv_myvision_io_wr(uint16_t port, uint8_t data) {
    if (port == 0x00)
        ay38910_set_reg(&psg, data);
    if (port == 0x01)
        ay38910_wr(&psg, data);
}

/* My Vision Memory Map
   0x0000 - 0x5fff: ROM
   0x6000 - 0x9fff: Unmapped
   0xa000 - 0xa7ff: RAM
   0xa800 - 0xdfff: Unmapped
   0xe000         : VDP Data Port (Read/Write)
   0xe001         : Unmapped
   0xe002         : VDP Control Port
   0xe003 - 0xffff: Unmapped
*/
static uint8_t jcv_myvision_mem_rd(uint16_t addr) {
    if (addr < 0x6000)
        return romdata[addr];
    else if (addr >= 0xa000 && addr < 0xa800)
        return ram[addr & 0x7ff];
    else if (addr == 0xe000)
        return tms9918_rd_data();
    else if (addr == 0xe002)
        return tms9918_rd_stat();
    return 0xff;
}

static void jcv_myvision_mem_wr(uint16_t addr, uint8_t data) {
    if (addr < 0xa000)
        return;
    else if (addr < 0xa800)
        ram[addr & 0x7ff] = data;
    else if (addr == 0xe000)
        tms9918_wr_data(data);
    else if (addr == 0xe002)
        tms9918_wr_ctrl(data);
}

int jcv_myvision_rom_load(void *data, size_t size) {
    romdata = data;
    romsize = size;
    return 1;
}

void jcv_myvision_init(void) {
    // Initialize sound chip
    ay38910_init(&psg);
    jcv_mixer_set_ay38910(&psg);

    // Clear RAM
    for (unsigned i = 0; i < sizeof(ram); ++i)
        ram[i] = 0xff;

    // Set Z80 function pointers
    jcv_z80_io_rd = jcv_myvision_io_rd;
    jcv_z80_io_wr = jcv_myvision_io_wr;
    jcv_z80_mem_rd = jcv_myvision_mem_rd;
    jcv_z80_mem_wr = jcv_myvision_mem_wr;
}

// Deinitialize any allocated memory
void jcv_myvision_deinit(void) {
}

void jcv_myvision_exec(void) {
    // Restore the leftover cycle count
    uint32_t extcycs = jcv_z80_cyc_restore();

    // Run scanline-based iterations of emulation until a frame is complete
    for (size_t i = 0; i < NUM_SCANLINES; ++i) {
        // Set the number of cycles required to complete this scanline
        size_t reqcycs = Z80_CYC_LINE - extcycs;

        // Count cycles for an iteration (usually one instruction)
        size_t itercycs = 0;

        // Count the total cycles run in a scanline
        size_t linecycs = 0;

        // Run CPU instructions until enough have been run for one scanline
        while (linecycs < reqcycs) {
            itercycs = jcv_z80_exec(); // Run a single CPU instruction
            linecycs += itercycs; // Add the number of cycles to the total

            for (size_t s = 0; s < itercycs; ++s) { // Catch PSG up to the CPU
                if (++psgcycs % DIV_PSG == 0) {
                    ay38910_exec(&psg);
                    psgcycs = 0;
                }
            }
        }

        extcycs = linecycs - reqcycs; // Store extra cycle count

        tms9918_exec(); // Draw a scanline of pixel data
    }

    // Resample audio and push to the frontend
    jcv_mixer_resamp();

    // Store the leftover cycle count
    jcv_z80_cyc_store(extcycs);
}
