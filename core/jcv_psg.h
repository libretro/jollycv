/*
 * Copyright (c) 2020 Rupert Carmichael
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef JCV_PSG_H
#define JCV_PSG_H

#define CFRAC 16 // Clock Fraction - PSG runs at 1/16th the input clock rate
#define LFSRSHIFT 14 // Linear Feedback Shift Register is 15 bits, so shift 14
#define NOISETAP 0x0003 // Tapped bits for ColecoVision are 0 and 1
#define SAMPLERATE_PSG 224000 // Approximate sample rate the PSG outputs at (Hz)
#define SIZE_PSGBUF 4600 // 4461 maximum in PAL mode, but give some overhead

typedef struct _cv_psg_t {
    uint8_t clatch; // Channel latch tells which channel's registers to write
    uint8_t attenuator[4]; // Four attenuators control volume on four channels
    uint16_t frequency[3]; // Three frequency registers for Tone Generators
    uint8_t noise; // One register for the Noise Generator
    uint16_t lfsr; // Linear Feedback Shift Register (15 bits on ColecoVision)
    uint16_t counter[4]; // Period Counter
    int16_t output[4]; // Per-channel output volumes for mixing
    uint8_t sign; // Four bits for four channels, 0 = Positive, 1 = Negative
    size_t cfrac; // Clock the PSG every 16 CPU cycles (Clock Fractions)
} cv_psg_t;

void jcv_psg_set_buffer(int16_t*);
void jcv_psg_set_callback(void (*)(size_t));
void jcv_psg_set_rate(size_t);
void jcv_psg_set_region(uint8_t);
void jcv_psg_set_rsqual(uint8_t);

void jcv_psg_reset(void);
void jcv_psg_init(void);
void jcv_psg_deinit(void);

void jcv_psg_wr(uint8_t);

void jcv_psg_resamp(size_t);
size_t jcv_psg_exec(void);

void jcv_psg_state_load(cv_psg_t*);
void jcv_psg_state_save(cv_psg_t*);

#endif
