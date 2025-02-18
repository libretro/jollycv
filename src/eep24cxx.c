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

// 24CXX Two Wire Serial EEPROM

#include <stddef.h>
#include <stdint.h>

#include "eep24cxx.h"

enum {
    STANDBY, STOP, START, ADDR_DEVICE, ADDR_PAGE, DATA_WR, DATA_RD
};

#define EEP24CXX_RESETCLK 9 // Clock up to 9 cycles

static inline void eep24cxx_addr_inc(eep24cxx_t *eep) {
    // Increment the address within a page
    eep->addr = ((eep->addr + 1) & (eep->pagesize - 1)) |
        (eep->addr & ~(eep->pagesize - 1));
}

void eep24cxx_init(eep24cxx_t *eep, uint8_t *savedata, unsigned sz) {
    // Initialize the EEPROM
    eep->data = savedata;
    eep->datasize = sz;

    // Set page size
    if (sz >= 0x4000) // 24C128, 24C256
        eep->pagesize = 0x40;
    else if (sz >= 0x1000) // 24C32, 24C64
        eep->pagesize = 0x20;
    else if (sz >= 0x200) // 24C16, 24C08, 24C04
        eep->pagesize = 0x10;
    else // 24C01, 24C02
        eep->pagesize = 0x04;

    // Set the default value to all 1s
    for (size_t i = 0; i < sz; ++i)
        eep->data[i] = 0xff;

    eep->sda = 1;
    eep->scl = 1;
    eep->wp = 0;
    eep->out = 1;
    eep->mode = STANDBY;
    eep->cmd = 0x00;
    eep->addr = 0x0000;
    eep->shift = 0x00;
    eep->clk = 0;
}

void eep24cxx_wr(eep24cxx_t *eep, uint8_t sda, uint8_t scl) {
    /* The SDA pin is normally pulled high with an external device. Data on the
       SDA pin may change only during SCL low time periods. Data changes during
       SCL high periods will indicate a start or stop condition.
    */
    if ((eep->sda != sda) && scl) { // SDA changed with SCL high
        /* Start Condition: A high-to-low transition of SDA with SCL high is a
           start condition that must precede any other command.

           Stop Condition: A low-to-high transition of SDA with SCL high is a
           stop condition. After a read sequence, the stop command will place
           the EEPROM in a standby power mode.
        */
        if (sda)
            eep->mode = STOP;
        else
            eep->mode = START;

        eep->shift = 0x00;
        eep->clk = 0;
    }
    else if ((eep->scl != scl) && scl) { // Positive edge
        /* The SCL input is used to positive-edge clock data into each EEPROM
           device and negative-edge clock data out of each device.
        */
        if (eep->mode == DATA_RD) { // Read mode, shift data onto output pin
            if (eep->clk == 0) { // Start fresh
                eep->shift = eep->data[eep->addr];
                eep24cxx_addr_inc(eep);
            }

            eep->out = (eep->shift >> 7) & 0x01;
            eep->shift <<= 1;

            if (++eep->clk == EEP24CXX_RESETCLK) { // Perform the ACK
                eep->clk = 0;
                eep->out = 0;
            }
        }
        else if (++eep->clk < EEP24CXX_RESETCLK) {
            // Shift input bits into the shift register if not in read mode
            eep->shift = (eep->shift << 1) | sda;
        }

        if (eep->clk == EEP24CXX_RESETCLK) {
            // 8 bits have been populated, perform the ACK and handle the byte
            eep->clk = 0;
            eep->out = 0;

            switch (eep->mode) {
                case STANDBY: {
                    break;
                }
                case STOP: {
                    eep->mode = STANDBY;
                    break;
                }
                case START: {
                    /* The device address word consists of a mandatory 1, 0
                       sequence for the four most significant bits. The eighth
                       bit of the device address is the read/write operation
                       select bit. A read operation is initiated if this bit is
                       high and a write operation is initiated if this bit is
                       low. If the device address has the wrong sequence in the
                       four high bits, wait for the next command.
                    */
                    eep->cmd = eep->shift;
                    if ((eep->cmd & 0xf0) == 0xa0)
                        eep->mode = eep->cmd & 0x01 ? DATA_RD : ADDR_DEVICE;
                    else
                        eep->mode = START;
                    break;
                }
                case ADDR_DEVICE: {
                    eep->addr = eep->shift;
                    if (eep->datasize >= 0x1000) {
                        eep->mode = ADDR_PAGE;
                    }
                    else {
                        /* Use the A0, A1, A2 bits with the shift register to
                           determine the address.
                        */
                        eep->addr |= ((unsigned)(eep->cmd & 0x0e) << 7);
                        eep->mode = DATA_WR;
                    }
                    break;
                }
                case ADDR_PAGE: {
                    eep->addr = (eep->addr << 8) | eep->shift;
                    eep->mode = DATA_WR;
                    break;
                }
                case DATA_WR: {
                    eep->data[eep->addr] = eep->shift;
                    eep24cxx_addr_inc(eep);
                    break;
                }
                case DATA_RD: { // Should not be reachable
                    break;
                }
                default: { // Failsafe
                    eep->mode = STOP;
                    break;
                }
            }
        }
    }

    // Store latest pin values
    eep->sda = sda;
    eep->scl = scl;
}
