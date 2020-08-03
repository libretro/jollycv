/*
 * Copyright (c) 2020 Rupert Carmichael
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stdint.h>
#include <stddef.h>

#include "jcv.h"
#include "jcv_memio.h"
#include "jcv_psg.h"
#include "jcv_vdp.h"
#include "jcv_z80.h"

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

#define Z80_CYC_LINE 228 // Z80 CPU cycles per scanline (227.99873)

static uint16_t numscanlines = CV_VDP_SCANLINES;

// Set the region
void jcv_set_region(uint8_t region) {
    // 313 scanlines for PAL, 262 scanlines for NTSC (192 visible for both)
    numscanlines = region ? CV_VDP_SCANLINES_PAL : CV_VDP_SCANLINES;
    jcv_psg_set_region(region);
    jcv_vdp_set_region(region);
}

// Initialize
void jcv_init(void) {
    jcv_memio_init();
    jcv_psg_init();
    jcv_vdp_init();
    jcv_z80_init();
}

// Deinitialize
void jcv_deinit(void) {
    jcv_memio_deinit();
    jcv_psg_deinit();
}

// Reset the system
void jcv_reset(int hard) {
    if (hard) { } // Currently unused
    jcv_memio_init(); // Init does the same thing reset needs to do
    jcv_psg_reset();
    jcv_vdp_init(); // Init does the same thing reset needs to do
    jcv_z80_reset();
}

// Run emulation for one frame
void jcv_exec(void) {
    size_t numsamps = 0; // Variable to record the number of samples generated
    size_t extcycs = jcv_z80_cyc_restore(); // Restore the leftover cycle count
    
    // Run scanline-based iterations of emulation until a frame is complete
    for (size_t i = 0; i < numscanlines; i++) {
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
            
            for (size_t s = 0; s < itercycs; s++) // Catch the PSG up to the CPU
                numsamps += jcv_psg_exec();
        }
        
        extcycs = linecycs - reqcycs; // Store extra cycle count
        
        jcv_vdp_exec(); // Draw a scanline of pixel data
    }
    
    jcv_psg_resamp(numsamps); // Resample audio and push to the frontend
    
    // Store the leftover cycle count
    jcv_z80_cyc_store(extcycs);
}
