/*
 * Copyright (c) 2020 Rupert Carmichael
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef JCV_SGMPSG_H
#define JCV_SGMPSG_H

typedef struct _cv_sgmpsg_t {
    uint8_t reg[16]; // 16 Read/Write 8-bit registers
    uint8_t rlatch; // Register that is currently selected
    
    uint16_t tperiod[3]; // Periods for Tones A, B, and C
    uint16_t tcounter[3]; // Counters for Tones A, B, and C
    uint8_t amplitude[3]; // Amplitudes for Tones A, B, and C
    
    uint8_t nperiod; // Noise Period
    uint16_t ncounter; // Noise Counter
    uint32_t nshift; // Noise Random Number Generator Shift Register (17-bit)
    
    uint16_t eperiod; // Envelope Period
    uint16_t ecounter; // Envelope Counter
    uint8_t eseg; // Envelope Segment: Which half of the cycle
    uint8_t estep; // Envelope Step
    uint8_t evol; // Envelope Volume
    
    uint8_t tenable[3]; // Enable bit for Tones A, B, and C
    uint8_t nenable[3]; // Enable bit for Noise on Channels A, B, and C
    uint8_t emode[3]; // Envelope Mode Enable bit for Tones A, B, and C
    
    uint8_t sign[3]; // Signify whether the waveform is high or low
    
    size_t cfrac; // Clock the PSG every 16 CPU cycles (Clock Fractions)
} cv_sgmpsg_t;

int16_t* jcv_sgmpsg_get_buffer(void);
void jcv_sgmpsg_init(void);
uint8_t jcv_sgmpsg_rd(void);
void jcv_sgmpsg_wr(uint8_t data);
void jcv_sgmpsg_set_reg(uint8_t);
size_t jcv_sgmpsg_exec(void);

void jcv_sgmpsg_state_load(cv_sgmpsg_t*);
void jcv_sgmpsg_state_save(cv_sgmpsg_t*);

#endif
