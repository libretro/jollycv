/*
 * Copyright (c) 2020-2021 Rupert Carmichael
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef JCV_PSG_H
#define JCV_PSG_H

typedef struct _cv_psg_t {
    uint8_t clatch; // Channel latch tells which channel's registers to write
    uint8_t attenuator[4]; // Four attenuators control volume on four channels
    uint16_t frequency[3]; // Three frequency registers for Tone Generators
    uint8_t noise; // One register for the Noise Generator
    uint16_t lfsr; // Linear Feedback Shift Register (15 bits on ColecoVision)
    uint16_t counter[4]; // Period Counter
    int16_t output[4]; // Per-channel output volumes for mixing
    uint8_t sign; // Four bits for four channels, 0 = Positive, 1 = Negative
} cv_psg_t;

void jcv_psg_set_buffer(int16_t*);
void jcv_psg_reset_buffer(void);

void jcv_psg_init(void);
void jcv_psg_wr(uint8_t);
size_t jcv_psg_exec(void);

void jcv_psg_state_load(cv_psg_t*);
void jcv_psg_state_save(cv_psg_t*);

#endif
