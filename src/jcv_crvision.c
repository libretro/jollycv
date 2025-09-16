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
#include <string.h>

#include "jcv_crvision.h"
#include "jcv_mixer.h"
#include "jcv_serial.h"

#include "jcv_m6502.h"
#include "jcv_vdp.h"
#include "sn76489.h"

#define DIV_PSG 16          // PSG Clock Divider
#define M6502_CYC_LINE 128  // 6502 CPU cycles per scanline (127.79553)
#define NUM_SCANLINES 313   // Total number of video scanlines

static uint8_t (*jcv_crvision_rom_rd40)(uint16_t);
static uint8_t (*jcv_crvision_rom_rd80)(uint16_t);

static crvision_sys_t crvsys;   // CreatiVision System Context
static sn76489_t psg;           // PSG Context

static uint8_t *romdata = NULL;     // Game ROM
static size_t romsize = 0;          // Size of the ROM in bytes

static uint8_t bios_internal = 0;   // BIOS loaded internally
static uint8_t *biosdata = NULL;    // BIOS ROM
static size_t biossize = 0;         // Size of the BIOS in bytes


// Frame execution related variables
static size_t psgcycs = 0;

// Stub implementation of the PIA
typedef struct _pia_t {
    uint8_t ddr[2]; // Data Direction Register
    uint8_t or[2]; // Output Register
    uint8_t cr[2]; // Control Register
} pia_t;
static pia_t pia;

// Keyboard Table
static uint8_t kbtbl[16];

static uint8_t (*jcv_crvision_input_cb)(int); // Input poll callback

// Pointers for 18K ROMs (Chopper Rescue)
static uint8_t *rombank1_8k = NULL; // First 8K bank
static uint8_t *rombank2_8k = NULL; // Second 8K bank
static uint8_t *rombank_2k = NULL;  // 2K bank

static uint8_t jcv_crvision_rom_null_rd40(uint16_t addr) {
    (void)addr;
    return 0xff;
}

static uint8_t jcv_crvision_rom_4k_rd80(uint16_t addr) {
    return romdata[addr & 0xfff];
}

static uint8_t jcv_crvision_rom_8k_rd80(uint16_t addr) {
    return romdata[addr & 0x1fff];
}

static uint8_t jcv_crvision_rom_12k_rd40(uint16_t addr) {
    return romdata[SIZE_8K + (addr & 0xfff)];
}

static uint8_t jcv_crvision_rom_12k_rd80(uint16_t addr) {
    return romdata[addr & 0x1fff];
}

static uint8_t jcv_crvision_rom_18k_rd80(uint16_t addr) {
    if (addr >= 0x8000 && addr < 0xa000)
        return rombank2_8k[addr & 0x1fff]; // Second 8K bank
    else if (addr >= 0xa000 && addr < 0xc000)
        return rombank1_8k[addr & 0x1fff]; // First 8K bank
    return 0xff;
}

static uint8_t jcv_crvision_rom_18k_rd40(uint16_t addr) {
    return rombank_2k[addr & 0x7ff];
}

void jcv_crvision_input_set_callback(uint8_t (*cb)(int)) {
    jcv_crvision_input_cb = cb;
}

// Load the CreatiVision BIOS
int jcv_crvision_bios_load_file(const char *biospath) {
    FILE *file;
    long size;

    if (!(file = fopen(biospath, "rb")))
        return 0;

    // Find out the file's size
    fseek(file, 0, SEEK_END);
    size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Make sure it is the correct size before attempting to load it
    if (size != SIZE_CRVBIOS) {
        fclose(file);
        return 0;
    }

    // Allocate memory for the BIOS
    biosdata = (uint8_t*)calloc(SIZE_CRVBIOS, sizeof(uint8_t));

    if (!fread(biosdata, SIZE_CRVBIOS, 1, file)) {
        fclose(file);
        return 0;
    }

    fclose(file);

    bios_internal = 1;
    return 1;
}

// Load the CreatiVision BIOS from a memory buffer
int jcv_crvision_bios_load(void *data, size_t size) {
    biosdata = data;
    biossize = size;
    return 1;
}

// Load a CreatiVision ROM Image
int jcv_crvision_rom_load(void *data, size_t size) {
    romdata = (uint8_t*)data; // Assign internal ROM pointer
    romsize = size; // Record the size of the ROM data in bytes

    printf("ROM size: %ld, 0x%04lx\n", romsize, romsize);
    switch (romsize) {
        case SIZE_4K: {
            printf("Configured as 4K ROM\n");
            jcv_crvision_rom_rd40 = &jcv_crvision_rom_null_rd40;
            jcv_crvision_rom_rd80 = &jcv_crvision_rom_4k_rd80;
            break;
        }
        case SIZE_8K: {
            printf("Configured as 8K ROM\n");
            jcv_crvision_rom_rd40 = &jcv_crvision_rom_null_rd40;
            jcv_crvision_rom_rd80 = &jcv_crvision_rom_8k_rd80;
            break;
        }
        case SIZE_12K: {
            printf("Configured as 12K ROM\n");
            jcv_crvision_rom_rd40 = &jcv_crvision_rom_12k_rd40;
            jcv_crvision_rom_rd80 = &jcv_crvision_rom_12k_rd80;
            break;
        }
        case SIZE_18K: {
            printf("Configured as 18K ROM\n");
            /* Only one game is 18K, Chopper Rescue. Detect how the ROM chips
               are concatenated to form the file. There are three known
               configurations.
            */
            uint8_t *rom = (uint8_t*)data;
            uint32_t sig0000 = (rom[0] << 24) | (rom[1] << 16) |
                (rom[2] << 8) | rom[3];
            uint32_t sig2000 = (rom[0x2000] << 24) | (rom[0x2001] << 16) |
                (rom[0x2002] << 8) | rom[0x2003];
            // Check for a5 62 18 69 to determine the ROM configuration
            if (sig0000 == 0xa5621869) {
                // Standard format (2/1/3)
                rombank1_8k = romdata + 0x2000;
                rombank2_8k = romdata + 0x0000;
                rombank_2k = romdata + 0x4000;
            }
            else if (sig2000 == 0xa5621869) {
                // Manual concatenated format (1/2/3)
                rombank1_8k = romdata + 0x0000;
                rombank2_8k = romdata + 0x2000;
                rombank_2k = romdata + 0x4000;
            }
            else { // Alt 1 format (1/3/2)
                rombank1_8k = romdata + 0x0000;
                rombank2_8k = romdata + 0x2800;
                rombank_2k = romdata + 0x2000;
            }

            jcv_crvision_rom_rd40 = &jcv_crvision_rom_18k_rd40;
            jcv_crvision_rom_rd80 = &jcv_crvision_rom_18k_rd80;
            break;
        }
        default: {
            printf("Unsupported ROM size: %ld bytes\n", romsize);
            return 0;
        }
    }

    return 1;
}

/* CreatiVision Read Map
   0x0000 - 0x0fff: RAM (Mirrored every 1K)
   0x1000 - 0x1fff: PIA (Mirrored every 4 bytes)
   0x2000 - 0x2fff: VDP (Mirrored every other byte)
   0x4000 - 0x7fff: 8K ROM Bank 2
   0x8000 - 0xbfff: 8K ROM Bank 1
   0xe801         : Centronics Status
*/
uint8_t jcv_crvision_mem_rd(uint16_t addr) {
    if (addr < 0x1000) { // RAM (mirrored)
        return crvsys.ram[addr & 0x3ff];
    }
    else if (addr < 0x2000) { // PIA
        if ((addr & 0x03) == 0x02) { // Port B read
            if ((pia.cr[1] & 0x04) == 0) {
                // DDR selected, return DDR value
                return pia.ddr[1];
            }
            else {
                // Output register selected
                if (pia.ddr[1] == 0xff) {
                    // When all bits are output, return the output register
                    return pia.or[1];
                }
                else {
                    int keylatch = (~pia.or[0]) & 0x0f;
                    return jcv_crvision_input_cb(keylatch);
                }
            }
        }

        return 0xff; // Other PIA registers
    }
    else if (addr < 0x3000) { // VDP
        return addr & 1 ? jcv_vdp_rd_stat() : jcv_vdp_rd_data();
    }
    else if (addr < 0x4000) { // Empty
        return 0xff;
    }
    else if (addr < 0x8000) { // ROM bank 2
        return jcv_crvision_rom_rd40(addr);
    }
    else if (addr < 0xc000) { // ROM bank 1
        return jcv_crvision_rom_rd80(addr);
    }
    else if (addr == 0xe801) { // Centronics Status
        printf("Centronics Status\n");
        return 0xff;
    }
    else if (addr >= 0xf800) { // BIOS ROM
        return biosdata[addr & 0x7ff];
    }

    printf("rd: %04x\n", addr);
    return 0xff;
}

/* CreatiVision Write Map
   0x0000 - 0x0fff: RAM (Mirrored every 1K)
   0x1000 - 0x1fff: PIA (Mirrored every 4 bytes)
   0x3000 - 0x3fff: VDP (Mirrored every other byte)
   0xe800         : Centronics Data
   0xe801         : Centronics Control
*/
void jcv_crvision_mem_wr(uint16_t addr, uint8_t data) {
    if (addr < 0x1000) { // RAM
        crvsys.ram[addr & 0x3ff] = data;
        return;
    }

    switch (addr & 0xf000) {
        case 0x1000: {
            switch (addr & 0x03) {
                case 0:
                    if ((pia.cr[0] & 0x04) == 0) {
                        pia.ddr[0] = data;
                    }
                    else {
                        pia.or[0] = data;
                    }
                    return;
                case 1:
                    pia.cr[0] = data;
                    return;
                case 2:
                    if ((pia.cr[1] & 0x04) == 0) {
                        pia.ddr[1] = data;
                    }
                    else {
                        pia.or[1] = data;
                        sn76489_wr(&psg, data);
                    }
                    return;
                case 3:
                    pia.cr[1] = data;
                    return;
            }
            break;
        }
        case 0x3000: { // VDP
            addr & 1 ? jcv_vdp_wr_ctrl(data) : jcv_vdp_wr_data(data);
            return;
        }
        case 0xe000: {
            printf("Centronics\n");
            return;
        }
    }

    printf("wr: %04x, %02x\n", addr, data);
}

void jcv_crvision_init(void) {
    // Initialize sound chip
    sn76489_init(&psg);
    jcv_mixer_set_psg(&psg);

    // Clear RAM
    memset(crvsys.ram, 0, sizeof(crvsys.ram));

    // Initialize PIA registers
    pia.or[0] = pia.ddr[0] = pia.cr[0] = 0;
    pia.or[1] = pia.ddr[1] = pia.cr[1] = 0;

    // Initialize keyboard table
    for (int i = 0; i < 16; i++)
        kbtbl[i] = 0xff;
}

// Deinitialize any allocated memory
void jcv_crvision_deinit(void) {
    if (biosdata && bios_internal)
        free(biosdata);
}

/*
 * 2 000 000 Hz / 50Hz = 40,000 CPU cycles per frame
 * 2 000 000 Hz / 50Hz / 313 lines = 127.79553 CPU cycles per line
*/
void jcv_crvision_exec(void) {
    // Restore the leftover cycle count
    uint32_t extcycs = jcv_m6502_cyc_restore();

    // Run scanline-based iterations of emulation until a frame is complete
    for (size_t i = 0; i < NUM_SCANLINES; ++i) {
        // Set the number of cycles required to complete this scanline
        size_t reqcycs = M6502_CYC_LINE - extcycs;

        // Count cycles for an iteration (usually one instruction)
        size_t itercycs = 0;

        // Count the total cycles run in a scanline
        size_t linecycs = 0;

        // Run CPU instructions until enough have been run for one scanline
        while (linecycs < reqcycs) {
            itercycs = jcv_m6502_exec(); // Run a single CPU instruction
            linecycs += itercycs; // Add the number of cycles to the total

            jcv_vdp_intchk(); // Keep IRQ line asserted if necessary

            for (size_t s = 0; s < itercycs; ++s) { // Catch PSG up to the CPU
                if (++psgcycs % DIV_PSG == 0) {
                    sn76489_exec(&psg);
                    psgcycs = 0;
                }
            }
        }

        extcycs = linecycs - reqcycs; // Store extra cycle count

        jcv_vdp_exec(); // Draw a scanline of pixel data
    }

    // Resample audio and push to the frontend
    jcv_mixer_resamp_crvision(psg.bufpos);

    // Store the leftover cycle count
    jcv_m6502_cyc_store(extcycs);
}
