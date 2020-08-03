/*
 * Copyright (c) 2020 Rupert Carmichael
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef JCV_Z80_H
#define JCV_Z80_H

// The Z80 struct contains function pointers, so this struct is used for states
typedef struct _cv_z80st_t {
    uint64_t cyc;
    uint16_t pc, sp, ix, iy;
    uint16_t mem_ptr;
    uint8_t a, b, c, d, e, h, l;
    uint8_t a_, b_, c_, d_, e_, h_, l_, f_;
    uint8_t i, r;
    uint8_t sf, zf, yf, hf, xf, pf, nf, cf;
    uint8_t iff_delay;
    uint8_t interrupt_mode;
    uint8_t int_data;
    uint8_t iff1, iff2;
    uint8_t halted;
    uint8_t int_pending, nmi_pending;
    size_t extracycs;
} cv_z80st_t;

void jcv_z80_cyc_store(size_t);
size_t jcv_z80_cyc_restore(void);
void jcv_z80_init(void);
void jcv_z80_irq(uint8_t data);
void jcv_z80_nmi(void);
void jcv_z80_reset(void);
void jcv_z80_delay(size_t);
uint32_t jcv_z80_exec(void);
uint32_t jcv_z80_run(uint32_t);

void jcv_z80_state_load(cv_z80st_t*);
void jcv_z80_state_save(cv_z80st_t*);

#endif
