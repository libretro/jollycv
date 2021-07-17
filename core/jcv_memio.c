/*
 * Copyright (c) 2020-2021 Rupert Carmichael
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "jcv.h"
#include "jcv_memio.h"
#include "jcv_psg.h"
#include "jcv_sgmpsg.h"
#include "jcv_vdp.h"
#include "jcv_z80.h"

// ColecoVision State
typedef struct _cv_state_t {
    cv_sys_t cvsys;
    cv_psg_t psg;
    cv_sgmpsg_t sgmpsg;
    cv_vdp_t vdp;
    cv_z80st_t z80st;
    uint32_t rompage[4];
} cv_state_t;

static uint16_t (*jcv_input_cb)(int); // Input poll callback

static uint8_t *cvbios = NULL; // BIOS ROM
static uint8_t *romdata = NULL; // Game ROM
static size_t romsize = 0; // Size of the ROM in bytes
static uint8_t rompages = 0; // Number of 8K ROM pages
static uint32_t rompage[4]; // Offsets to the start of 8K ROM pages

static uint8_t megacart = 0; // Mark whether the cart is a Mega Cart or not
static uint8_t sgm_upper = 0; // Enable upper 24K SGM RAM
static uint8_t sgm_lower = 0; // Enable lower 8K SGM RAM - replaces BIOS mapping

static cv_sys_t cvsys; // ColecoVision System Context
static cv_state_t cvstate; // System State containing all components

void jcv_input_set_callback(uint16_t (*cb)(int)) {
    jcv_input_cb = cb;
}

// Read a byte of data from an I/O port
uint8_t jcv_io_rd(uint8_t port) {
    /* ColecoVision I/O Read Map
       0xa0 - 0xbf: VDP Reads (Port Odd: Status, Port Even: VRAM)
       0xe0 - 0xff: Control Port Strobe (0xfc, 0xff)
    */
    switch (port & 0xe0) {
        case 0xa0: { // Video
            return port & 0x01 ? jcv_vdp_rd_stat() : jcv_vdp_rd_data();
        }
        case 0xe0: { // Strobe controller ports for input state
            uint8_t p = (port & 0x02) >> 1; // Port variable for convenience
            cvsys.ctrl[p] = jcv_input_cb(p); // Call frontend for input state
            
            // Return the complement of the value
            return cvsys.cseg ? // Two strobes are done for two sets of buttons
                ~((uint8_t)(cvsys.ctrl[p] >> 8)) : // Joystick, FireL
                ~((uint8_t)(cvsys.ctrl[p] & 0xff)); // Numpad, FireR
        }
        default: {
            if (port == 0x52) // SGM PSG Read
                return jcv_sgmpsg_rd();
            return 0xff;
        }
    }
}

// Write a byte of data to an I/O port
void jcv_io_wr(uint8_t port, uint8_t data) {
    /* ColecoVision I/O Write Map
       0x80 - 0x9f: Set Controller Strobe Segment to Numpad/FireR
       0xa0 - 0xbf: VDP Writes (Port Odd: Registers, Port Even: VRAM)
       0xc0 - 0xdf: Set Controller Strobe Segment to Joystick/FireL
       0xe0 - 0xff: PSG Writes (0xff)
    */
    switch(port & 0xe0) {
        // Data is irrelevant for cases 0x80 and 0xc0: just toggle a flip-flop
        case 0x80: { // Set Controller Strobe Segment to Numpad/FireR
            cvsys.cseg = 0;
            break;
        }
        case 0xa0: { // Write to VDP Control Registers or VRAM
            port & 0x01 ? jcv_vdp_wr_ctrl(data) : jcv_vdp_wr_data(data);
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
            jcv_z80_delay(32); // PCM sample pitch will be high without a delay
            jcv_psg_wr(data);
            break;
        }
        default: {
            if (port == 0x50) // Set the SGM PSG's active register
                jcv_sgmpsg_set_reg(data & 0x0f);
            else if (port == 0x51) // Write to the SGM PSG's selected register
                jcv_sgmpsg_wr(data);
            else if (port == 0x53)
                sgm_upper = 1;
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
uint8_t jcv_mem_rd(uint16_t addr) {
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
        if (megacart && addr >= 0xffc0) { // Select new 16K bank on Mega Carts
            /* Divide the number of pages by 2 because we are dealing with 16K
               banks vs 8K banks. Subtract 1 because page numbers are
               zero-indexed. Shift left 14 to create the offset into ROM data.
            */
            rompage[2] = (addr & ((rompages >> 1) - 1)) << 14;
            rompage[3] = rompage[2] + SIZE_8K; // Second half of 16K page
        }
        
        // If there are read attempts beyond the ROM's true size, return padding
        if (addr >= (romsize + SIZE_32K))
            return 0xff;
        
        uint8_t page = (addr >> 13) - 4; // Find the ROM page to read from
        return romdata[rompage[page] + (addr & 0x1fff)];
    }
}

// Write a byte to a memory location
void jcv_mem_wr(uint16_t addr, uint8_t data) {
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
}

// Load the ColecoVision BIOS
int jcv_bios_load(const char *biospath) {
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
    cvbios = (uint8_t*)malloc(SIZE_CVBIOS * sizeof(uint8_t));
    
    if (!fread(cvbios, SIZE_CVBIOS, 1, file)) {
        fclose(file);
        return 0;
    }
    
    fclose(file);
    
    return 1;
}

// Load a ColecoVision ROM Image
int jcv_rom_load(void *data, size_t size) {
    romdata = (uint8_t*)data; // Assign internal ROM pointer
    romsize = size; // Record the true size of the ROM data in bytes
    
    if (romsize > 0x8000) { // ROM image is possibly a Mega Cart
        uint16_t hword = // First, check if this is a valid ROM image
            romdata[romsize - SIZE_16K] | (romdata[romsize - SIZE_16K+1] << 8);
        if (hword != 0xaa55 && hword != 0x55aa)
            return 0; // Fail if this not a valid ColecoVision ROM image
        
        megacart = 1; // Mark the Mega Cart bit true
        rompages = (size / SIZE_8K) + (size % SIZE_8K ? 1 : 0); // Count pages
        
        // The selectable banks are 16K and mapped to 0xc000 - 0xffff
        rompage[2] = 0x0000; // Map 0xc000 to the first 8K bank
        rompage[3] = SIZE_8K; // Map 0xe000 to the second 8K bank
        
        // The final 16K segment of ROM is always mapped to 0x8000 - 0xbfff
        rompage[0] = romsize - SIZE_16K; // First half of final 16K bank
        rompage[1] = romsize - SIZE_8K; // Second half of final 16K bank
        
        return 1;
    }
    
    /* ROM data should start with one of two possible combinations of two bytes:
       0xaa, 0x55: Show the BIOS screen with game title and copyright info
       0x55, 0xaa: Jump to the code vector (start of game code), bypassing BIOS
                   boot routines
    */
    uint16_t hword = romdata[1] | (romdata[0] << 8); // Header Word
    if (hword != 0xaa55 && hword != 0x55aa)
        return 0; // Fail if this not a valid ColecoVision ROM image
    
    // Find out how many 8K pages of ROM data there are
    // Use modulus to discover if there is a page that is not quite 8K
    rompages = (size / SIZE_8K) + (size % SIZE_8K ? 1 : 0);
    
    // Assign ROM page offsets to locations in ROM data
    // Schematic shows 4 lines for 8K ROM pages (EN_80, EN_A0, EN_C0, EN_E0)
    for (int i = 0; i < rompages; i++)
        rompage[i] = i * SIZE_8K;
    
    return 1;
}

// Initialize memory and set I/O states to default
void jcv_memio_init(void) {
    /* Fill RAM with garbage - Some software relies on non-zero data at boot,
       such as Yolk's on You, and possibly more. Every individual console may
       have its own affinities, but the values are still indeterminate.
    */
    srand(time(NULL));
    for (int i = 0; i < SIZE_CVRAM; ++i)
        cvsys.ram[i] = rand() % 256; // Random numbers from 0-255
    
    memset(cvsys.sgmram, 0xff, 0x6000);
    
    cvsys.cseg = 0; // Controller Strobe Segment
    cvsys.ctrl[0] = cvsys.ctrl[1] = 0; // Reset input states to empty
    
    // Set SGM RAM to disabled state
    sgm_upper = 0;
    sgm_lower = 0;
}

// Deinitialize any allocated memory
void jcv_memio_deinit(void) {
    free(cvbios);
}

// Return the size of a state
size_t jcv_state_size(void) {
    return sizeof(cv_state_t);
}

// Load raw state data into the running system
void jcv_state_load_raw(const void *sstate) {
    cv_state_t *st = (cv_state_t*)sstate; // Cast the raw data
    cvsys = st->cvsys;
    jcv_psg_state_load(&(st->psg));
    jcv_sgmpsg_state_load(&(st->sgmpsg));
    jcv_vdp_state_load(&(st->vdp));
    jcv_z80_state_load(&(st->z80st));
    for (int i = 0; i < 4; i++)
        rompage[i] = st->rompage[i];
}

// Load a state from a file
int jcv_state_load(const char *filename) {
    FILE *file;
    size_t filesize, result;
    void *sstatefile;
    
    // Open the file for reading
    file = fopen(filename, "rb");
    if (!file)
        return 0;
    
    // Find out the file's size
    fseek(file, 0, SEEK_END);
    filesize = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Allocate memory to read the file into
    sstatefile = (void*)malloc(filesize * sizeof(uint8_t));
    if (sstatefile == NULL)
        return 0;
    
    // Read the file into memory and then close it
    result = fread(sstatefile, sizeof(uint8_t), filesize, file);
    if (result != filesize)
        return 0;
    fclose(file);
    
    // File has been read, now copy it into the emulator
    jcv_state_load_raw((const void*)sstatefile);
    
    // Free the allocated memory
    free(sstatefile);
    
    return 1; // Success!
}

// Snapshot the running state and return the address of the raw data
const void* jcv_state_save_raw(void) {
    cvstate.cvsys = cvsys;
    jcv_psg_state_save(&cvstate.psg);
    jcv_sgmpsg_state_save(&cvstate.sgmpsg);
    jcv_vdp_state_save(&cvstate.vdp);
    jcv_z80_state_save(&cvstate.z80st);
    for (int i = 0; i < 4; ++i)
        cvstate.rompage[i] = rompage[i];
    return (const void*)&cvstate;
}

// Save a state to a file
int jcv_state_save(const char *filename) {
    // Open the file for writing
    FILE *file;
    file = fopen(filename, "wb");
    if (!file)
        return 0;
    
    // Snapshot the running state and get the memory address
    cv_state_t *sstate = (cv_state_t*)jcv_state_save_raw();
    
    // Write and close the file
    fwrite(sstate, jcv_state_size(), sizeof(uint8_t), file);
    fclose(file);
    
    return 1; // Success!
}
