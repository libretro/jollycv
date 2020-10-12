/*
 * Copyright (c) 2020 Rupert Carmichael
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// ColecoVision Super Game Module PSG - General Instrument AY-3-8910

#include <stdint.h>
#include <string.h>

#include "jcv_sgmpsg.h"

#define SIZE_PSGBUF 4600
#define CFRAC 16 // Clock Fraction - PSG runs at 1/16th the input clock rate

static const int16_t vtable[16] = { // Volume Table
    0,       40,      60,     86,    124,    186,    264,    440,
    518,    840,    1196,   1526,   2016,   2602,   3300,   4096,
};

static cv_sgmpsg_t psg; // SGM PSG Context

static int16_t psgbuf[SIZE_PSGBUF]; // Buffer for raw PSG output samples
static size_t bufpos = 0; // Keep track of the position in the PSG output buffer

// Reset the Envelope step and volume depending on the currently selected shape
static inline void jcv_sgmpsg_env_reset(void) {
    psg.estep = 0; // Reset the step counter
    
    if (psg.eseg) { // Segment 1
        switch (psg.reg[13]) {
            case 8: case 11: case 13: case 14: // Start from the top
                psg.evol = 15;
                break;
            default: // Start from the bottom
                psg.evol = 0;
                break;
        }
    }
    else { // Segment 0
        // If the Attack bit is set, start from the bottom. Otherwise, the top.
        psg.evol = psg.reg[13] & 0x04 ? 0 : 15;
    }
}

// Grab the pointer to the PSG's buffer
int16_t* jcv_sgmpsg_get_buffer(void) {
    bufpos = 0;
    return &psgbuf[0];
}

// Set initial values
void jcv_sgmpsg_init(void) {
    for (int i = 0; i < 16; i++)
        psg.reg[i] = 0x00;
    
    for (int i = 0; i < 3; i++)
        psg.tcounter[i] = 0;
    
    // Seed the Noise RNG Shift Register
    psg.nshift = 1;
    
    psg.estep = 0;
}

// Read from the currently latched Control Register
uint8_t jcv_sgmpsg_rd(void) {
    return psg.reg[psg.rlatch];
}

// Write to the currently latched Control Register
void jcv_sgmpsg_wr(uint8_t data) {
    /* Registers
    |---|-----------------------------------------------|
    | R |  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
    |---|-----------------------------------------------|
    | 0 |                8-bit fine tune                | Channel A Tone Period
    | 1 |  -     -     -     -  |   4-bit coarse tune   |
    |---|-----------------------------------------------|
    | 2 |                8-bit fine tune                | Channel B Tone Period
    | 3 |  -     -     -     -  |   4-bit coarse tune   |
    |---|-----------------------------------------------|
    | 4 |                8-bit fine tune                | Channel C Tone Period
    | 5 |  -     -     -     -  |   4-bit coarse tune   |
    |---|-----------------------------------------------|
    | 6 |  -     -     -  |    5-bit period control     | Noise Period
    |---|-----------------------------------------------|
    | 7 | IOB | IOA | NC  | NB  | NA  | TC  | TB  | TA  | Enable IO/Noise/Tone
    |---|-----------------------------------------------|
    | 8 |  -     -     -  |  M  | L3  | L2  | L1  | L0  | Channel A Amplitude
    |---|-----------------------------------------------|
    | 9 |  -     -     -  |  M  | L3  | L2  | L1  | L0  | Channel B Amplitude
    |---|-----------------------------------------------|
    |10 |  -     -     -  |  M  | L3  | L2  | L1  | L0  | Channel C Amplitude
    |---|-----------------------------------------------|
    |11 |                8-bit fine tune                | Envelope Period
    |12 |               8-bit coarse tune               |
    |---|-----------------------------------------------|
    |13 |  -     -     -     -  |CONT | ATT | ALT |HOLD | Envelope Shape/Cycle
    |---|-----------------------------------------------|
    |14 |          8-bit Parallel IO on Port A          | IO Port A Data Store
    |---|-----------------------------------------------|
    |15 |          8-bit Parallel IO on Port B          | IO Port B Data Store
    |---|-----------------------------------------------|
    */
    
    // Masks to avoid writing "Don't Care" bits
    uint8_t dcmask[16] = {
        0xff, 0x0f, 0xff, 0x0f, 0xff, 0x0f, 0x1f, 0xff,
        0x1f, 0x1f, 0x1f, 0xff, 0xff, 0x0f, 0xff, 0xff,
    };
    
    // Write data to the latched register
    psg.reg[psg.rlatch] = data & dcmask[psg.rlatch];
    
    switch (psg.rlatch) {
        /* Tone Periods are 12-bit values comprising 8 bits from the first
           register, and 4 bits from the second regiser.
           The lowest period for tones is 1, so if 0 is set, change it to 1.
        */
        case 0: case 1: { // Channel A Tone Period
            psg.tperiod[0] = psg.reg[0] | (psg.reg[1] << 8);
            if (psg.tperiod[0] == 0)
                psg.tperiod[0] = 1;
            break;
        }
        case 2: case 3: { // Channel B Tone Period
            psg.tperiod[1] = psg.reg[2] | (psg.reg[3] << 8);
            if (psg.tperiod[1] == 0)
                psg.tperiod[1] = 1;
            break;
        }
        case 4: case 5: { // Channel C Tone Period
            psg.tperiod[2] = psg.reg[4] | (psg.reg[5] << 8);
            if (psg.tperiod[2] == 0)
                psg.tperiod[2] = 1;
            break;
        }
        case 6: { // Noise Period
            psg.nperiod = psg.reg[6];
            if (psg.nperiod == 0) // As with Tones, lowest period value is 1.
                psg.nperiod = 1;
            break;
        }
        case 7: { // Enable IO/Noise/Tone
            // Register 7's Enable bits are actually Disable bits.
            psg.tenable[0] = !(psg.reg[7] & 0x01);
            psg.tenable[1] = !(psg.reg[7] & 0x02);
            psg.tenable[2] = !(psg.reg[7] & 0x04);
            psg.nenable[0] = !(psg.reg[7] & 0x08);
            psg.nenable[1] = !(psg.reg[7] & 0x10);
            psg.nenable[2] = !(psg.reg[7] & 0x20);
            break;
        }
        case 8: { // Channel A Amplitude
            psg.amplitude[0] = data & 0x0f;
            psg.emode[0] = (data >> 4) & 0x01;
            break;
        }
        case 9: { // Channel B Amplitude
            psg.amplitude[1] = data & 0x0f;
            psg.emode[1] = (data >> 4) & 0x01;
            break;
        }
        case 10: { // Channel C Amplitude
            psg.amplitude[2] = data & 0x0f;
            psg.emode[2] = (data >> 4) & 0x01;
            break;
        }
        case 11: case 12: { // Envelope Period
            psg.eperiod = psg.reg[11] | (psg.reg[12] << 8);
            break;
        }
        case 13: { // Envelope Shape
            // Reset all Envelope related variables when Register 13 is written
            psg.ecounter = 0;
            psg.eseg = 0;
            jcv_sgmpsg_env_reset();
            break;
        }
        /* Nothing really needs to be done for the IO Port Data Store Registers,
           so skip cases 14 and 15.
        */
        default:
            break;
    }
}

// Set the latched Control Register
void jcv_sgmpsg_set_reg(uint8_t r) {
    psg.rlatch = r;
}

// Execute a PSG cycle
size_t jcv_sgmpsg_exec(void) {
    if (++psg.cfrac != CFRAC) // Only clock the PSG every 16th CPU clock cycle
        return 0; // Zero samples generated
    
    // Every 16th CPU clock cycle, allow the code below to run
    psg.cfrac = 0; // Reset the PSG clock fraction counter
    
    for (int i = 0; i < 3; i++) {
        if (++psg.tcounter[i] >= psg.tperiod[i]) {
            psg.tcounter[i] = 0;
            psg.sign[i] ^= 1;
        }
    }
    
    if (++psg.ncounter >= (psg.nperiod << 1)) {
        psg.ncounter = 0;
        /* The Noise Random Number Generator is a 17-bit shift register, whose
           input is bit 0 XOR bit 3. The result of this operation is output at
           bit 16 as bit 1 becomes the new bit 0, which will determine whether
           to output noise when a sample is generated.
        */
        psg.nshift = (psg.nshift >> 1) |
            (((psg.nshift ^ (psg.nshift >> 3)) & 0x1) << 16);
    }
    
    if (++psg.ecounter >= (psg.eperiod << 1)) {
        psg.ecounter = 0;
        /* Envelope Shape
           The bits from 3 to 0 represent Continue, Attack, Alternate, and Hold.
           For Continue values of 0, the bottom two bits are irrelevant, meaning
           there are only 2 possible shapes for the first 8 numerical values.
           00xx: \____
           01xx: /|____
           1000: \|\|\|     1001: \_____     1010: \/\/\/     1011: \|----
           1100: /|/|/|     1101: /-----     1110: /\/\/\     1111: /|____
        */
        if (psg.eseg) { // Second half of the envelope shape
            switch (psg.reg[13]) {
                case 8: case 14: { // Count Downwards
                    psg.evol--;
                    break;
                }
                case 10: case 12: { // Count Upwards
                    psg.evol++;
                    break;
                }
                case 11: case 13: { // Hold High
                    psg.evol = 15;
                    break;
                }
                default: { // Hold Low
                    psg.evol = 0;
                    break;
                }
            }
        }
        else { // First half of the envelope shape
            if (psg.reg[13] & 0x04) // Attack is set - Count upwards
                psg.evol++;
            else // Otherwise count downwards
                psg.evol--;
        }
        if (++psg.estep >= 16) {
            psg.eseg ^= 1;
            jcv_sgmpsg_env_reset();
        }
    }
    
    int16_t vol = 0;
    for (int i = 0; i < 3; i++) {
        /* If the tone channel is enabled and the sign bit is set, or noise
           output for this channel is enabled and the bit 0 of the noise shift
           register is set, output a value. Otherwise, leave it at 0.
        */
        uint8_t out = (psg.tenable[i] && psg.sign[i]) ||
            (psg.nenable[i] && psg.nshift & 0x01);
        
        /* If the envelope mode bit is set for this channel, output variable
           level amplitude (envelope step), otherwise output the fixed level
           amplitude value.
        */
        if (out)
            vol += psg.emode[i] ? vtable[psg.evol] : vtable[psg.amplitude[i]];
    }
    
    psgbuf[bufpos++] = vol;
    return 1;
}

void jcv_sgmpsg_state_load(cv_sgmpsg_t *st_sgmpsg) {
    psg = *st_sgmpsg;
}

void jcv_sgmpsg_state_save(cv_sgmpsg_t *st_sgmpsg) {
    *st_sgmpsg = psg;
}
