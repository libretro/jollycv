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
#include <stdarg.h>

#include "jollycv.h"

#include "jcv_coleco.h"
#include "jcv_crvision.h"
#include "jcv_myvision.h"

#include "jcv_db.h"
#include "jcv_mixer.h"
#include "jcv_z80.h"
#include "jcv_m6502.h"

#include "ay38910.h"
#include "sn76489.h"
#include "tms9918.h"

// Function pointer for execution of one frame of emulation
static void (*jcv_exec_fn)(void) = NULL;

// Log callback
static void (*jcv_log_cb)(int, const char *, ...) = NULL;

// Function pointers for state functions
size_t (*jcv_state_size_fn)(void);
void (*jcv_state_load_raw_fn)(const void*);
const void* (*jcv_state_save_raw_fn)(void);

// System being emulated
static unsigned sys = JCV_SYS_COLECO;

// System region
static unsigned region = JCV_REGION_NTSC;

static void jcv_log_default(int level, const char *fmt, ...) {
    char buffer[512];
    va_list va;
    va_start(va, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, va);
    va_end(va);

    FILE *fout = level == 1 ? stdout : stderr;
    fprintf(fout, "%s", buffer);
    fflush(fout);
}

// Set the log callback
void jcv_log_set_callback(void (*cb)(int, const char *, ...)) {
    jcv_log_cb = cb;
}

void jcv_log(int level, const char *fmt, ...) {
    char buffer[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    jcv_log_cb(level, buffer);
}

// Get the region
unsigned jcv_get_region(void) {
    return region;
}

// Set the region
void jcv_set_region(unsigned r) {
    region = r;
    jcv_coleco_set_region(region);
    jcv_mixer_set_region(region);
    tms9918_set_region(region);
}

// Get the emulated system
unsigned jcv_get_system(void) {
    return sys;
}

// Set the emulated system
void jcv_set_system(unsigned s) {
    sys = s;
}

// Get database flags
uint32_t jcv_get_dbflags(void) {
    switch (sys) {
        default: case JCV_SYS_COLECO: return jcv_db_get_flags();
    }
}

// Get database flags
void jcv_process_hash(const char *md5) {
    switch (sys) {
        default: case JCV_SYS_COLECO: jcv_db_process_coleco(md5); break;
    }
}

void jcv_input_set_callback_coleco(unsigned (*cb)(const void*, int), void *u) {
    jcv_coleco_input_set_callback(cb, u);
}

void jcv_input_set_callback_crvision(unsigned (*cb)(const void*,int), void *u) {
    jcv_crvision_input_set_callback(cb, u);
}

void jcv_input_set_callback_myvision(unsigned (*cb)(const void*), void *u) {
    jcv_myvision_input_set_callback(cb, u);
}

void jcv_exec(void) {
    jcv_exec_fn();
}

// Initialize
void jcv_init(void) {
    // Ensure there is a log callback
    if (jcv_log_cb == NULL)
        jcv_log_cb = jcv_log_default;

    // Set up the desired system to emulate
    switch (sys) {
        default: case JCV_SYS_COLECO: {
            jcv_exec_fn = &jcv_coleco_exec;
            jcv_state_load_raw_fn = &jcv_coleco_state_load_raw;
            jcv_state_save_raw_fn = &jcv_coleco_state_save_raw;
            jcv_state_size_fn = &jcv_coleco_state_size;
            jcv_coleco_init();
            jcv_mixer_init(sys);
            jcv_z80_init();
            tms9918_set_vblint(&jcv_z80_nmi);
            break;
        }
        case JCV_SYS_CRVISION: {
            jcv_exec_fn = &jcv_crvision_exec;
            jcv_state_load_raw_fn = &jcv_crvision_state_load_raw;
            jcv_state_save_raw_fn = &jcv_crvision_state_save_raw;
            jcv_state_size_fn = &jcv_crvision_state_size;
            jcv_crvision_init();
            jcv_mixer_init(sys);
            jcv_m6502_init();
            tms9918_set_vblint(&jcv_m6502_irq);
            break;
        }
        case JCV_SYS_MYVISION: {
            jcv_exec_fn = &jcv_myvision_exec;
            jcv_state_load_raw_fn = &jcv_myvision_state_load_raw;
            jcv_state_save_raw_fn = &jcv_myvision_state_save_raw;
            jcv_state_size_fn = &jcv_myvision_state_size;
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

// Wrappers for audio/video functions
void jcv_audio_set_buffer(void *buf) {
    jcv_mixer_set_buffer((int16_t*)buf);
}

void jcv_audio_set_callback(void (*cb)(const void*, size_t), void *u) {
    jcv_mixer_set_callback(cb, u);
}

void jcv_audio_set_rate(size_t rate) {
    jcv_mixer_set_rate(rate);
}

void jcv_audio_set_raw(int raw) {
    jcv_mixer_set_raw(raw);
}

void jcv_audio_set_rsqual(unsigned rsqual) {
    jcv_mixer_set_rsqual(rsqual);
}

void jcv_video_set_buffer(uint32_t *buf) {
    tms9918_set_buffer(buf);
}

void jcv_video_set_palette_tms9918(unsigned p) {
    tms9918_set_palette(p);
}

void jcv_video_set_nosprlimit_tms9918(unsigned l) {
    tms9918_set_nosprlimit(l);
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

size_t jcv_state_size(void) {
    return jcv_state_size_fn();
}

void jcv_state_load_raw(const void *s) {
    jcv_state_load_raw_fn(s);
}

const void* jcv_state_save_raw(void) {
    return jcv_state_save_raw_fn();
}

int jcv_savedata_load(const char *filename) {
    FILE *file;
    size_t filesize, result;

    // Open the file for reading
    file = fopen(filename, "rb");
    if (!file)
        return JCV_SAVE_NONE;

    // Find out the file's size
    fseek(file, 0, SEEK_END);
    filesize = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (filesize > jcv_memory_get_size(JCV_MEM_SAVEDATA)) {
        fclose(file);
        return 0;
    }

    // Read the file into the memory block and then close it
    result = fread(jcv_memory_get_data(JCV_MEM_SAVEDATA),
        sizeof(uint8_t), filesize, file);
    if (result != filesize) {
        fclose(file);
        return JCV_SAVE_FAIL;
    }

    fclose(file);

    return JCV_SAVE_SUCCESS;
}

int jcv_savedata_save(const char *filename) {
    if (!jcv_memory_get_size(JCV_MEM_SAVEDATA))
        return JCV_SAVE_NONE;

    FILE *file;
    file = fopen(filename, "wb");
    if (!file)
        return JCV_SAVE_FAIL;

    // Write and close the file
    fwrite(jcv_coleco_get_save_data(), jcv_coleco_get_save_size(),
        sizeof(uint8_t), file);
    fclose(file);

    return JCV_SAVE_SUCCESS;
}

int jcv_bios_load(void *data, size_t size) {
    switch (sys) {
        case JCV_SYS_COLECO: return jcv_coleco_bios_load(data, size);
        case JCV_SYS_CRVISION: return jcv_crvision_bios_load(data, size);
    }
    return 0;
}

int jcv_bios_load_file(const char *biospath) {
    FILE *file;
    size_t size;

    if (!(file = fopen(biospath, "rb")))
        return 0;

    // Find out the file's size
    fseek(file, 0, SEEK_END);
    size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allocate memory for the BIOS
    uint8_t *bios = (uint8_t*)calloc(size, sizeof(uint8_t));

    if (!fread(bios, size, 1, file)) {
        free(bios);
        fclose(file);
        return 0;
    }

    jcv_bios_load(bios, size);

    free(bios);
    fclose(file);
    return 1;
}

int jcv_media_load(void *data, size_t size) {
    switch (sys) {
        case JCV_SYS_COLECO: return jcv_coleco_rom_load(data, size);
        case JCV_SYS_CRVISION: return jcv_crvision_rom_load(data, size);
        case JCV_SYS_MYVISION: return jcv_myvision_rom_load(data, size);
    }
    return 0;
}

void* jcv_memory_get_data(unsigned type) {
    switch (type) {
        case JCV_MEM_SYSTEM: {
            switch (sys) {
                case JCV_SYS_COLECO: return jcv_coleco_get_ram_data();
                case JCV_SYS_CRVISION: return jcv_crvision_get_ram_data();
                case JCV_SYS_MYVISION: return jcv_myvision_get_ram_data();
            }
            break;
        }
        case JCV_MEM_VRAM: return tms9918_get_vram_data();
        case JCV_MEM_SAVEDATA: {
            switch (sys) {
                case JCV_SYS_COLECO: return jcv_coleco_get_save_data();
            }
            break;
        }
    }
    return NULL;
}

size_t jcv_memory_get_size(unsigned type) {
    switch (type) {
        case JCV_MEM_SYSTEM: {
            switch (sys) {
                case JCV_SYS_COLECO: return SIZE_CVRAM;
                case JCV_SYS_CRVISION: return SIZE_CRVRAM;
                case JCV_SYS_MYVISION: return SIZE_MYVRAM;
            }
            break;
        }
        case JCV_MEM_VRAM: return SIZE_TMS9918_VRAM;
        case JCV_MEM_SAVEDATA: {
            switch (sys) {
                case JCV_SYS_COLECO: return jcv_coleco_get_save_size();
            }
            break;
        }
    }
    return 0;
}
