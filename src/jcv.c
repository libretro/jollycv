/*
Copyright (c) 2020-2022 Rupert Carmichael
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
#include <stdint.h>
#include <stdlib.h>

#include "jcv.h"

#include "jcv_coleco.h"
#include "jcv_crvision.h"
#include "jcv_myvision.h"

#include "jcv_mixer.h"
#include "jcv_z80.h"
#include "jcv_m6502.h"

#include "ay38910.h"
#include "sn76489.h"
#include "tms9918.h"

// Function pointer for execution of one frame of emulation
void (*jcv_exec)(void) = NULL;

// Log callback
void (*jcv_log)(int, const char *, ...) = NULL;

// Function pointers for state functions
size_t (*jcv_state_size)(void);
void (*jcv_state_load_raw)(const void*);
const void* (*jcv_state_save_raw)(void);

// System being emulated
static unsigned sys = 0;

// Set the log callback
void jcv_log_set_callback(void (*cb)(int, const char *, ...)) {
    jcv_log = cb;
}

// Set the region
void jcv_set_region(unsigned region) {
    // 313 scanlines for PAL, 262 scanlines for NTSC (192 visible for both)
    jcv_coleco_set_region(region);
    jcv_mixer_set_region(region);
    tms9918_set_region(region);
}

// Set the emulated system
void jcv_set_system(unsigned s) {
    sys = s;
}

// Initialize
void jcv_init(void) {
    switch (sys) {
        default: case JCV_SYS_COLECO: {
            jcv_coleco_init();
            jcv_mixer_init(sys);
            jcv_z80_init();
            tms9918_set_vblint(&jcv_z80_nmi);
            break;
        }
        case JCV_SYS_CRVISION: {
            jcv_crvision_init();
            jcv_mixer_init(sys);
            jcv_m6502_init();
            tms9918_set_vblint(&jcv_m6502_irq);
            break;
        }
        case JCV_SYS_MYVISION: {
            jcv_myvision_init();
            jcv_mixer_init(sys);
            jcv_z80_init();
            tms9918_set_vblint(&jcv_z80_irq_ff);
            break;
        }
    }

    tms9918_init();
}

// Deinitialize
void jcv_deinit(void) {
    switch (sys) {
        default: case JCV_SYS_COLECO: {
            jcv_coleco_deinit();
            break;
        }
        case JCV_SYS_CRVISION: {
            jcv_crvision_deinit();
            break;
        }
        case JCV_SYS_MYVISION: {
            jcv_myvision_deinit();
            break;
        }
    }

    jcv_mixer_deinit();
}

// Reset the system
void jcv_reset(int hard) {
    switch (sys) {
        default: case JCV_SYS_COLECO: {
            jcv_coleco_init(); // Init does the same thing reset needs to do
            jcv_z80_reset();
            tms9918_init();
            break;
        }
        case JCV_SYS_CRVISION: {
            if (hard)
                jcv_m6502_reset();
            else
                jcv_m6502_nmi();
            break;
        }
        case JCV_SYS_MYVISION: {
            jcv_myvision_init();
            jcv_z80_reset();
            tms9918_init();
            break;
        }
    }
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
    sstatefile = (void*)calloc(filesize, sizeof(uint8_t));
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

// Save a state to a file
int jcv_state_save(const char *filename) {
    // Open the file for writing
    FILE *file;
    file = fopen(filename, "wb");
    if (!file)
        return 0;

    // Snapshot the running state and get the memory address
    uint8_t *sstate = (uint8_t*)jcv_state_save_raw();

    // Write and close the file
    fwrite(sstate, jcv_state_size(), sizeof(uint8_t), file);
    fclose(file);

    return 1; // Success!
}
