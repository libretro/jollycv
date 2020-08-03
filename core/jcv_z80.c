/*
 * Copyright (c) 2020 Rupert Carmichael
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "jcv_memio.h"
#include "jcv_z80.h"

#include "z80.h"

static z80 z80ctx;
static size_t extracycs = 0;
static size_t delaycycs = 0;

// Memory Read
static uint8_t read_byte(void *userdata, uint16_t addr) {
    if (userdata) { } // Unused
    return jcv_mem_rd(addr);
}

// Memory Write
static void write_byte(void *userdata, uint16_t addr, uint8_t data) {
    if (userdata) { } // Unused
    jcv_mem_wr(addr, data);
}

// IO Port Read
static uint8_t port_in(z80 *z, uint8_t port) {
    if (z) { } // Unused
    return jcv_io_rd(port);
}

// IO Port Write
static void port_out(z80 *z, uint8_t port, uint8_t data) {
    if (z) { } // Unused
    jcv_io_wr(port, data);
}

// Store extra cycle count
void jcv_z80_cyc_store(size_t cycs) {
    extracycs = cycs;
}

// Retrieve stored extra cycle count
size_t jcv_z80_cyc_restore(void) {
    size_t ret = extracycs;
    extracycs = 0;
    return ret;
}

// Initialize the Z80
void jcv_z80_init(void) {
    z80_init(&z80ctx);
    z80ctx.read_byte = &read_byte;
    z80ctx.write_byte = &write_byte;
    z80ctx.port_in = &port_in;
    z80ctx.port_out = &port_out;
}

// Reset the Z80
void jcv_z80_reset(void) {
    jcv_z80_init();
}

// Generate an Interrupt
void jcv_z80_irq(uint8_t data) {
    z80_gen_int(&z80ctx, data);
}

// Generate a Non-Maskable Interrupt
void jcv_z80_nmi(void) {
    z80_gen_nmi(&z80ctx);
}

// Delay the Z80's execution by a requested number of cycles
void jcv_z80_delay(size_t delay) {
    delaycycs += delay;
}

// Run a single Z80 instruction
uint32_t jcv_z80_exec(void) {
    uint32_t retcyc = 0;
    uint32_t curcyc = z80ctx.cyc;
    z80_step(&z80ctx);
    retcyc = z80ctx.cyc - curcyc;
    
    if (delaycycs) {
        retcyc += delaycycs;
        delaycycs = 0;
    }
    
    return retcyc;
}

// Run Z80 instructions until at least the requested number of cycles have run
uint32_t jcv_z80_run(uint32_t cycles) {
    uint32_t retcyc = 0;
    uint32_t curcyc = z80ctx.cyc;
    
    while (retcyc < cycles) {
        z80_step(&z80ctx);
        retcyc = z80ctx.cyc - curcyc;
    }
    
    if (delaycycs) {
        retcyc += delaycycs;
        delaycycs = 0;
    }
    
    return retcyc;
}

// Restore the Z80's state from external data
void jcv_z80_state_load(cv_z80st_t *st_z80) {
    z80ctx.cyc = st_z80->cyc;
    z80ctx.pc = st_z80->pc;
    z80ctx.sp = st_z80->sp;
    z80ctx.ix = st_z80->ix;
    z80ctx.iy = st_z80->iy;
    z80ctx.mem_ptr = st_z80->mem_ptr;
    z80ctx.a = st_z80->a;
    z80ctx.b = st_z80->b;
    z80ctx.c = st_z80->c;
    z80ctx.d = st_z80->d;
    z80ctx.e = st_z80->e;
    z80ctx.h = st_z80->h;
    z80ctx.l = st_z80->l;
    z80ctx.a_ = st_z80->a_;
    z80ctx.b_ = st_z80->b_;
    z80ctx.c_ = st_z80->c_;
    z80ctx.d_ = st_z80->d_;
    z80ctx.e_ = st_z80->e_;
    z80ctx.h_ = st_z80->h_;
    z80ctx.l_ = st_z80->l_;
    z80ctx.f_ = st_z80->f_;
    z80ctx.i = st_z80->i;
    z80ctx.r = st_z80->r;
    z80ctx.sf = st_z80->sf;
    z80ctx.zf = st_z80->zf;
    z80ctx.yf = st_z80->yf;
    z80ctx.hf = st_z80->hf;
    z80ctx.xf = st_z80->xf;
    z80ctx.pf = st_z80->pf;
    z80ctx.nf = st_z80->nf;
    z80ctx.cf = st_z80->cf;
    z80ctx.iff_delay = st_z80->iff_delay;
    z80ctx.interrupt_mode = st_z80->interrupt_mode;
    z80ctx.int_data = st_z80->int_data;
    z80ctx.iff1 = st_z80->iff1;
    z80ctx.iff2 = st_z80->iff2;
    z80ctx.halted = st_z80->halted;
    z80ctx.int_pending = st_z80->int_pending;
    z80ctx.nmi_pending = st_z80->nmi_pending;
    extracycs = st_z80->extracycs;
}

// Export the Z80's state
void jcv_z80_state_save(cv_z80st_t *st_z80) {
    st_z80->cyc = z80ctx.cyc;
    st_z80->pc = z80ctx.pc;
    st_z80->sp = z80ctx.sp;
    st_z80->ix = z80ctx.ix;
    st_z80->iy = z80ctx.iy;
    st_z80->mem_ptr = z80ctx.mem_ptr;
    st_z80->a = z80ctx.a;
    st_z80->b = z80ctx.b;
    st_z80->c = z80ctx.c;
    st_z80->d = z80ctx.d;
    st_z80->e = z80ctx.e;
    st_z80->h = z80ctx.h;
    st_z80->l = z80ctx.l;
    st_z80->a_ = z80ctx.a_;
    st_z80->b_ = z80ctx.b_;
    st_z80->c_ = z80ctx.c_;
    st_z80->d_ = z80ctx.d_;
    st_z80->e_ = z80ctx.e_;
    st_z80->h_ = z80ctx.h_;
    st_z80->l_ = z80ctx.l_;
    st_z80->f_ = z80ctx.f_;
    st_z80->i = z80ctx.i;
    st_z80->r = z80ctx.r;
    st_z80->sf = z80ctx.sf;
    st_z80->zf = z80ctx.zf;
    st_z80->yf = z80ctx.yf;
    st_z80->hf = z80ctx.hf;
    st_z80->xf = z80ctx.xf;
    st_z80->pf = z80ctx.pf;
    st_z80->nf = z80ctx.nf;
    st_z80->cf = z80ctx.cf;
    st_z80->iff_delay = z80ctx.iff_delay;
    st_z80->interrupt_mode = z80ctx.interrupt_mode;
    st_z80->int_data = z80ctx.int_data;
    st_z80->iff1 = z80ctx.iff1;
    st_z80->iff2 = z80ctx.iff2;
    st_z80->halted = z80ctx.halted;
    st_z80->int_pending = z80ctx.int_pending;
    st_z80->nmi_pending = z80ctx.nmi_pending;
    st_z80->extracycs = extracycs;
}
