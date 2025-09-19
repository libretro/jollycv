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
#include <time.h>

#include "jcv_coleco.h"
#include "jcv_mixer.h"
#include "jcv_serial.h"
#include "jcv_z80.h"

#include "tms9918.h"
#include "eep24cxx.h"

#define DIV_PSG 16 // PSG Clock Divider
#define Z80_CYC_LINE 228 // Z80 CPU cycles per scanline (227.99873)

#define SIZE_STATE 50392
static uint8_t state[SIZE_STATE + SIZE_32K];

static uint16_t (*jcv_coleco_input_cb)(const void*, int); // Input callback
static void *udata_input = NULL; // Input callback userdata

static uint8_t *cvbios = NULL; // BIOS ROM
static uint8_t bios_internal = 0; // BIOS loaded internally
static uint8_t *romdata = NULL; // Game ROM
static size_t romsize = 0; // Size of the ROM in bytes
static uint8_t rompages = 0; // Number of 8K ROM pages
static uint32_t rompage[4]; // Offsets to the start of 8K ROM pages

// Cartridge Types
static unsigned carttype = 0; // Cartridge Type

// Super Game Module RAM
static uint8_t sgm_upper = 0; // Enable upper 24K SGM RAM
static uint8_t sgm_lower = 0; // Enable lower 8K SGM RAM - replaces BIOS mapping

// SRAM or EEPROM Save Data
static uint8_t savedata[SIZE_32K];
static size_t savesize = 0;

// Frame execution related variables
static size_t numscanlines = TMS9918_SCANLINES;
static size_t psgcycs = 0;

static cv_sys_t cvsys; // ColecoVision System Context
static sn76489_t psg; // PSG Context
static ay38910_t sgmpsg; // SGM PSG Context
static eep24cxx_t eep24cxx; // 24Cxx EEPROM Context (Activision PCBs)

void jcv_coleco_input_set_callback(uint16_t (*cb)(const void*, int), void *u) {
    jcv_coleco_input_cb = cb;
    udata_input = u;
}

// Return the size of a state
static size_t jcv_coleco_state_size(void) {
    return SIZE_STATE + savesize;
}

// Load raw state data into the running system
static void jcv_coleco_state_load_raw(const void *sstate) {
    uint8_t *st = (uint8_t*)sstate;
    jcv_serial_begin();
    jcv_serial_popblk(cvsys.ram, st, SIZE_CVRAM);
    jcv_serial_popblk(cvsys.sgmram, st, SIZE_32K);
    cvsys.cseg = jcv_serial_pop8(st);
    cvsys.ctrl[0] = jcv_serial_pop16(st);
    cvsys.ctrl[1] = jcv_serial_pop16(st);
    for (int i = 0; i < 4; ++i) rompage[i] = jcv_serial_pop32(st);
    sn76489_state_load(&psg, st);
    ay38910_state_load(&sgmpsg, st);
    tms9918_state_load(st);
    jcv_z80_state_load(st);
    sgm_upper = jcv_serial_pop8(st);
    sgm_lower = jcv_serial_pop8(st);
    if (carttype == CART_ACTIVISION) { // EEPROM
        eep24cxx.sda = jcv_serial_pop8(st);
        eep24cxx.scl = jcv_serial_pop8(st);
        eep24cxx.out = jcv_serial_pop8(st);
        eep24cxx.wp = jcv_serial_pop8(st);
        eep24cxx.mode = jcv_serial_pop8(st);
        eep24cxx.cmd = jcv_serial_pop8(st);
        eep24cxx.clk = jcv_serial_pop8(st);
        eep24cxx.shift = jcv_serial_pop8(st);
        eep24cxx.addr = jcv_serial_pop16(st);
        jcv_serial_popblk(eep24cxx.data, st, eep24cxx.datasize);
    }
    else if (carttype == CART_SRAM) {
        jcv_serial_popblk(savedata, st, SIZE_2K);
    }
}

// Snapshot the running state and return the address of the raw data
static const void* jcv_coleco_state_save_raw(void) {
    jcv_serial_begin();
    jcv_serial_pushblk(state, cvsys.ram, SIZE_CVRAM);
    jcv_serial_pushblk(state, cvsys.sgmram, SIZE_32K);
    jcv_serial_push8(state, cvsys.cseg);
    jcv_serial_push16(state, cvsys.ctrl[0]);
    jcv_serial_push16(state, cvsys.ctrl[1]);
    for (int i = 0; i < 4; ++i) jcv_serial_push32(state, rompage[i]);
    sn76489_state_save(&psg, state);
    ay38910_state_save(&sgmpsg, state);
    tms9918_state_save(state);
    jcv_z80_state_save(state);
    jcv_serial_push8(state, sgm_upper);
    jcv_serial_push8(state, sgm_lower);
    if (carttype == CART_ACTIVISION) { // EEPROM
        jcv_serial_push8(state, eep24cxx.sda);
        jcv_serial_push8(state, eep24cxx.scl);
        jcv_serial_push8(state, eep24cxx.out);
        jcv_serial_push8(state, eep24cxx.wp);
        jcv_serial_push8(state, eep24cxx.mode);
        jcv_serial_push8(state, eep24cxx.cmd);
        jcv_serial_push8(state, eep24cxx.clk);
        jcv_serial_push8(state, eep24cxx.shift);
        jcv_serial_push16(state, eep24cxx.addr);
        jcv_serial_pushblk(state, eep24cxx.data, eep24cxx.datasize);
    }
    else if (carttype == CART_SRAM) {
        jcv_serial_pushblk(state, savedata, SIZE_2K);
    }
    return (const void*)state;
}

// Load SRAM
int jcv_coleco_sram_load(const char *filename) {
    FILE *file;
    size_t filesize, result;

    // Open the file for reading
    file = fopen(filename, "rb");
    if (!file)
        return 2;

    // Find out the file's size
    fseek(file, 0, SEEK_END);
    filesize = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (filesize > savesize) {
        fclose(file);
        return 0;
    }

    // Read the file into the system's Cartridge RAM slot and then close it
    result = fread(savedata, sizeof(uint8_t), filesize, file);
    if (result != filesize) {
        fclose(file);
        return 0;
    }

    fclose(file);

    return 1; // Success!
}

// Read a byte of data from an I/O port
static uint8_t jcv_coleco_io_rd(uint16_t port) {
    /* ColecoVision I/O Read Map
       0xa0 - 0xbf: VDP Reads (Port Odd: Status, Port Even: VRAM)
       0xe0 - 0xff: Control Port Strobe (0xfc, 0xff)
    */
    switch (port & 0xe0) {
        case 0xa0: { // Video
            return port & 0x01 ? tms9918_rd_stat() : tms9918_rd_data();
        }
        case 0xe0: { // Strobe controller ports for input state
            uint8_t p = (port & 0x02) >> 1; // Port variable for convenience
            // Read frontend input state, always set bit 7 for both segments
            cvsys.ctrl[p] = jcv_coleco_input_cb(udata_input, p) | 0x8080;

            // Return the complement of the value
            return cvsys.cseg ? // Two strobes are done for two sets of buttons
                ~((uint8_t)(cvsys.ctrl[p] >> 8)) : // Joystick, FireL
                ~((uint8_t)(cvsys.ctrl[p] & 0xff)); // Numpad, FireR
        }
        default: {
            if (port == 0x52) // SGM PSG Read
                return ay38910_rd(&sgmpsg);
            return 0xff;
        }
    }
}

// Write a byte of data to an I/O port
static void jcv_coleco_io_wr(uint16_t port, uint8_t data) {
    /* ColecoVision I/O Write Map
       0x80 - 0x9f: Set Controller Strobe Segment to Numpad/FireR
       0xa0 - 0xbf: VDP Writes (Port Odd: Registers, Port Even: VRAM)
       0xc0 - 0xdf: Set Controller Strobe Segment to Joystick/FireL
       0xe0 - 0xff: PSG Writes (0xff)
    */
    switch (port & 0xe0) {
        // Data is irrelevant for cases 0x80 and 0xc0: just toggle a flip-flop
        case 0x80: { // Set Controller Strobe Segment to Numpad/FireR
            cvsys.cseg = 0;
            break;
        }
        case 0xa0: { // Write to VDP Control Registers or VRAM
            port & 0x01 ? tms9918_wr_ctrl(data) : tms9918_wr_data(data);
            break;
        }
        case 0xc0: { // Set Controller Strobe Segment to Joystick/FireL
            cvsys.cseg = 1;
            break;
        }
        case 0xe0: { // Write to the PSG Control Registers
            /* SN76489AN requires ~32 clock cycles to load data into registers
               according to the datasheet. It could be more like 54, but there
               does not seem to be any definitive data on this.
            */
            jcv_z80_delay(48); // PCM sample pitch will be high without a delay
            sn76489_wr(&psg, data);
            break;
        }
        default: {
            if (port == 0x50) // Set the SGM PSG's active register
                ay38910_set_reg(&sgmpsg, data & 0x0f);
            else if (port == 0x51) // Write to the SGM PSG's selected register
                ay38910_wr(&sgmpsg, data);
            else if (port == 0x53)
                sgm_upper = data & 0x01;
            else if (port == 0x7f)
                sgm_lower = ~data & 0x02;
            break;
        }
    }
}

/* ColecoVision Memory Map
   0x0000 - 0x1fff: BIOS ROM
   0x2000 - 0x3fff: Expansion port
   0x4000 - 0x5fff: Expansion port
   0x6000 - 0x7fff: 8K RAM mirrored every 1K
   0x8000 - 0xffff: Cartridge ROM (8K pages every 0x2000)
*/

// Read a byte of memory
static uint8_t jcv_coleco_mem_rd(uint16_t addr) {
    if (sgm_lower && (addr < 0x2000)) {
        return cvsys.sgmram[addr];
    }
    else if (addr < 0x2000) { // BIOS from 0x0000 to 0x1fff
        return cvbios[addr];
    }
    else if (sgm_upper && (addr < 0x8000)) {
        return cvsys.sgmram[addr];
    }
    else if (addr < 0x6000) { // Expansion port reads when no SGM is plugged in
        return 0xff; // Return default 0xff if nothing is plugged in
    }
    else if (addr < 0x8000) { // 1K RAM mirrored every 1K for 8K
        return cvsys.ram[addr & 0x3ff];
    }
    else { // Cartridge ROM from 0x8000 to 0xffff
        if (carttype == CART_MEGA && addr >= 0xffc0) {
            /* Select new 16K bank on Mega Carts - Divide the number of pages
               by 2 because we are dealing with 16K banks vs 8K banks. Subtract
               1 because page numbers are zero-indexed. Shift left 14 to create
               the offset into ROM data.
            */
            rompage[2] = (addr & ((rompages >> 1) - 1)) << 14;
            rompage[3] = rompage[2] + SIZE_8K; // Second half of 16K page
        }
        else if (carttype == CART_ACTIVISION && addr == 0xff80) {
            // Return the value of the EEPROM output pin for address 0xff80.
            return eep24cxx.out;
        }
        else if (carttype == CART_SRAM && addr > 0xdfff) {
            return savedata[addr & 0x7ff];
        }

        // If there are read attempts beyond the ROM's true size, return padding
        if (addr >= (romsize + SIZE_32K))
            return 0xff;

        uint8_t page = (addr >> 13) - 4; // Find the ROM page to read from
        return romdata[rompage[page] + (addr & 0x1fff)];
    }
}

// Write a byte to a memory location
static void jcv_coleco_mem_wr(uint16_t addr, uint8_t data) {
    /* If the Super Game Module is plugged in and activated, the RAM writes will
       all be mapped to the SGM RAM. This means writes that would normally go to
       base system RAM are now going into SGM RAM.
    */
    if (sgm_lower && (addr < 0x2000))
        cvsys.sgmram[addr] = data;
    else if (sgm_upper && (addr > 0x1fff) && (addr < 0x8000))
        cvsys.sgmram[addr] = data;
    else if ((addr > 0x5fff) && (addr < 0x8000)) // Base System RAM writes
        cvsys.ram[addr & 0x3ff] = data;

    if (carttype == CART_ACTIVISION) {
        /* Activision PCBs swap banks when 0xff90, 0xffa0, or 0xffb0 are
           written with any value. Writes to 0xffc0 and 0xffd0 control the
           state of SCL, while writes to 0xffe0 and 0xfff0 control the state of
           SDA.
        */
        switch (addr) {
            case 0xff90: {
                rompage[2] = 2 * SIZE_8K;
                rompage[3] = 3 * SIZE_8K;
                break;
            }
            case 0xffa0: {
                rompage[2] = 4 * SIZE_8K;
                rompage[3] = 5 * SIZE_8K;
                break;
            }
            case 0xffb0: {
                rompage[2] = 6 * SIZE_8K;
                rompage[3] = 7 * SIZE_8K;
                break;
            }
            case 0xffc0: {
                eep24cxx_wr(&eep24cxx, eep24cxx.sda, 0);
                break;
            }
            case 0xffd0: {
                eep24cxx_wr(&eep24cxx, eep24cxx.sda, 1);
                break;
            }
            case 0xffe0: {
                eep24cxx_wr(&eep24cxx, 0, eep24cxx.scl);
                break;
            }
            case 0xfff0: {
                eep24cxx_wr(&eep24cxx, 1, eep24cxx.scl);
                break;
            }
        }
    }
    else if (carttype == CART_SRAM && addr >= 0xdfff) {
        savedata[addr & 0x7ff] = data;
    }
}

// Load the ColecoVision BIOS
int jcv_coleco_bios_load_file(const char *biospath) {
    FILE *file;
    long size;

    if (!(file = fopen(biospath, "rb")))
        return 0;

    // Find out the file's size
    fseek(file, 0, SEEK_END);
    size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Make sure it is the correct size before attempting to load it
    if (size != SIZE_CVBIOS) {
        fclose(file);
        return 0;
    }

    // Allocate memory for the BIOS
    cvbios = (uint8_t*)calloc(SIZE_CVBIOS, sizeof(uint8_t));

    if (!fread(cvbios, SIZE_CVBIOS, 1, file)) {
        fclose(file);
        return 0;
    }

    fclose(file);

    bios_internal = 1;
    return 1;
}

// Load the ColecoVision BIOS from a memory buffer
int jcv_coleco_bios_load(void *data, size_t size) {
    cvbios = data;
    if (size != SIZE_CVBIOS)
        return 0;
    return 1;
}

// Load a ColecoVision ROM Image
int jcv_coleco_rom_load(void *data, size_t size) {
    romdata = (uint8_t*)data; // Assign internal ROM pointer
    romsize = size; // Record the true size of the ROM data in bytes

    /* ROM data should start with one of two possible combinations of two bytes:
       0xaa, 0x55: Show the BIOS screen with game title and copyright info
       0x55, 0xaa: Jump to the code vector (start of game code), bypassing BIOS
                   boot routines
    */
    uint16_t hword = romdata[1] | (romdata[0] << 8); // Header Word

    if (romsize > 0x8000) { // ROM is possibly a Mega Cart or Activision PCB
        uint16_t mchword = // First, check if this is a Mega Cart
            romdata[romsize - SIZE_16K] | (romdata[romsize - SIZE_16K+1] << 8);

        if (mchword == 0xaa55 || mchword == 0x55aa) {
            carttype = CART_MEGA; // Mark the Mega Cart bit true
            hword = mchword;
        }
        else {
            carttype = CART_ACTIVISION; // It is most likely an Activision PCB
        }
    }

    if (hword != 0xaa55 && hword != 0x55aa)
        return 0; // Fail if this not a valid ColecoVision ROM image

    // Find out how many 8K pages of ROM data there are
    // Use modulus to discover if there is a page that is not quite 8K
    rompages = (size / SIZE_8K) + (size % SIZE_8K ? 1 : 0);

    if (carttype == CART_MEGA) {
        // The selectable banks are 16K and mapped to 0xc000 - 0xffff
        rompage[2] = 0x0000; // Map 0xc000 to the first 8K bank
        rompage[3] = SIZE_8K; // Map 0xe000 to the second 8K bank

        // The final 16K segment of ROM is always mapped to 0x8000 - 0xbfff
        rompage[0] = romsize - SIZE_16K; // First half of final 16K bank
        rompage[1] = romsize - SIZE_8K; // Second half of final 16K bank
    }
    else if (carttype == CART_ACTIVISION) {
        /* The selectable banks are 16K and mapped to 0xc000 - 0xffff. The
           first 16K is always mapped to the first 16K of address space.
        */
        for (int i = 0; i < 4; ++i)
            rompage[i] = i * SIZE_8K;
        eep24cxx_init(&eep24cxx, savedata, savesize);
    }
    else {
        /* Assign ROM page offsets to locations in ROM data
           Schematic shows 4 lines for 8K ROM pages: EN_80, EN_A0, EN_C0, EN_E0
        */
        for (int i = 0; i < rompages; ++i)
            rompage[i] = i * SIZE_8K;
    }

    return 1;
}

void jcv_coleco_set_carttype(unsigned ctype, unsigned special) {
    carttype = ctype;
    if (carttype == CART_SRAM)
        savesize = SIZE_2K;
    else if (carttype == CART_ACTIVISION)
        savesize = special;
}

// Initialize memory and set I/O states to default
void jcv_coleco_init(void) {
    /* Fill RAM with garbage - Some software relies on non-zero data at boot,
       such as Yolk's on You, and possibly more. Every individual console may
       have its own affinities, but the values are still indeterminate.
    */
    srand(time(NULL));
    for (int i = 0; i < SIZE_CVRAM; ++i)
        cvsys.ram[i] = rand() & 0xff; // Random numbers from 0-255

    memset(cvsys.sgmram, 0xff, 0x6000);

    cvsys.cseg = 0; // Controller Strobe Segment
    cvsys.ctrl[0] = cvsys.ctrl[1] = 0; // Reset input states to empty

    // Set ColecoVision function pointers
    jcv_exec = &jcv_coleco_exec;
    jcv_state_load_raw = jcv_coleco_state_load_raw;
    jcv_state_save_raw = jcv_coleco_state_save_raw;
    jcv_state_size = jcv_coleco_state_size;

    // Set Z80 function pointers
    jcv_z80_io_rd = jcv_coleco_io_rd;
    jcv_z80_io_wr = jcv_coleco_io_wr;
    jcv_z80_mem_rd = jcv_coleco_mem_rd;
    jcv_z80_mem_wr = jcv_coleco_mem_wr;

    // Set SGM RAM to disabled state
    sgm_upper = 0;
    sgm_lower = 0;

    // Initialize sound chips
    sn76489_init(&psg);
    ay38910_init(&sgmpsg);
    jcv_mixer_set_sn76489(&psg);
    jcv_mixer_set_ay38910(&sgmpsg);
}

// Deinitialize any allocated memory
void jcv_coleco_deinit(void) {
    if (cvbios && bios_internal)
        free(cvbios);
}

void jcv_coleco_set_region(unsigned region) {
    numscanlines = region ? TMS9918_SCANLINES_PAL : TMS9918_SCANLINES;
}

// Save SRAM
int jcv_coleco_sram_save(const char *filename) {
    if (!(carttype == CART_SRAM || carttype == CART_ACTIVISION))
        return 2;

    FILE *file;
    file = fopen(filename, "wb");
    if (!file)
        return 0;

    // Write and close the file
    fwrite(savedata, savesize, sizeof(uint8_t), file);
    fclose(file);

    return 1; // Success!
}

// Run emulation for one frame
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
void jcv_coleco_exec(void) {
    // Restore the leftover cycle count
    uint32_t extcycs = jcv_z80_cyc_restore();

    // Run scanline-based iterations of emulation until a frame is complete
    for (size_t i = 0; i < numscanlines; ++i) {
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

            for (size_t s = 0; s < itercycs; ++s) { // Catch PSGs up to the CPU
                if (++psgcycs % DIV_PSG == 0) {
                    sn76489_exec(&psg);
                    ay38910_exec(&sgmpsg);
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
