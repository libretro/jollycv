/*
MIT License

Copyright (c) 2018 Nicolas Allemand

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef M6502_M6502_H_
#define M6502_M6502_H_

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct m6502 {
    uint8_t (*read_byte)(void*, uint16_t); // user function to read from memory
    void (*write_byte)(void*, uint16_t, uint8_t); // same for writing to memory
    void* userdata; // user custom pointer
    unsigned cyc; // cycle count

    uint16_t pc; // program counter
    uint8_t a, x, y, sp; // register A, X, Y and stack pointer

    // flags: carry, zero, interrupt disable, decimal mode,
    // break command, overflow, negative
    bool cf : 1, zf : 1, idf : 1, df : 1, bf : 1, vf : 1, nf : 1;

    bool page_crossed : 1; // helper flag to keep track of page crossing
    bool enable_bcd : 1; // helper flag to enable/disable BCD
    bool m65c02_mode : 1; // helper flag to enable 65C02 emulation

    bool stop : 1, wait : 1; // flags used with STP/WAI 65C02 instructions
} m6502;

void m6502_init(m6502* const c);
void m6502_debug_output(m6502* const c);
unsigned m6502_step(m6502* const c);

// interrupts
void m6502_gen_nmi(m6502* const c);
void m6502_gen_res(m6502* const c);
void m6502_gen_irq(m6502* const c);

#endif // M6502_M6502_H_
