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

// An idiot admires complexity, a genius admires simplicity - Terry A. Davis

#ifndef JOLLYCV_H
#define JOLLYCV_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Log levels
 */
enum jcv_loglevel {
    JCV_LOG_DBG,    /**< Debug log level */
    JCV_LOG_INF,    /**< Info log level */
    JCV_LOG_WRN,    /**< Warn log level */
    JCV_LOG_ERR,    /**< Error log level */
    JCV_LOG_SCR     /**< Screen log level */
};

/**
 * Set the log callback
 * @param cb Pointer to the callback to be used for log output
 */
void jcv_log_set_callback(void (*cb)(int, const char*, ...));

/**
 * Log to the console
 * @param level Log level
 * @param fmt Format string in printf syntax
 * @param ... Variable arguments corresponding to the format string
 */
void jcv_log(int level, const char *fmt, ...);

#define JCV_REGION_NTSC             0   /**< NTSC: Japan, North America */
#define JCV_REGION_PAL              1   /**< PAL: UK, Europe, Australia */

/**
 * Get the currently active region
 * @return Region currently in use
 */
unsigned jcv_get_region(void);

/**
 * Set the region for the emulated system
 * @param r Region for the emulated system
 */
void jcv_set_region(unsigned r);

#define JCV_SYS_COLECO              0x00    /**< ColecoVision */
#define JCV_SYS_CRVISION            0x01    /**< CreatiVision */
#define JCV_SYS_MYVISION            0x02    /**< My Vision */

/**
 * Get the currently active emulated system
 * @return System currently being emulated
 */
unsigned jcv_get_system(void);

/**
 * Set the emulated system type
 * @param s System to emulate
 */
void jcv_set_system(unsigned s);

#define JCV_DB_COLECO_PAD           0x00000001  /**< ColecoVision Paddle */
#define JCV_DB_COLECO_ROLLER        0x00000002  /**< ColecoVision Roller
                                                    Controller */
#define JCV_DB_COLECO_SAC           0x00000004  /**< ColecoVision Super Action
                                                    Controller */
#define JCV_DB_COLECO_SKETCH        0x00000008  /**< Super Sketch */
#define JCV_DB_COLECO_WHEEL         0x00000010  /**< ColecoVision Steering
                                                    Wheel */

/**
 * Check the database for any special flags pertaining to a ROM
 * @param md5 MD5 checksum of the ROM to look up
 * @return Flags for this game found in the internal database
 */
uint32_t jcv_get_dbflags(const char *md5);

#define JCV_COLECO_INPUT_U          (1 << 0)    /**< ColecoVision Up */
#define JCV_COLECO_INPUT_D          (1 << 1)    /**< ColecoVision Down */
#define JCV_COLECO_INPUT_L          (1 << 2)    /**< ColecoVision Left */
#define JCV_COLECO_INPUT_R          (1 << 3)    /**< ColecoVision Right */
#define JCV_COLECO_INPUT_FL         (1 << 4)    /**< ColecoVision Fire (L) */
#define JCV_COLECO_INPUT_FR         (1 << 5)    /**< ColecoVision Fire (R) */
#define JCV_COLECO_INPUT_1          (1 << 6)    /**< ColecoVision Numpad 1 */
#define JCV_COLECO_INPUT_2          (1 << 7)    /**< ColecoVision Numpad 2 */
#define JCV_COLECO_INPUT_3          (1 << 8)    /**< ColecoVision Numpad 3 */
#define JCV_COLECO_INPUT_4          (1 << 9)    /**< ColecoVision Numpad 4 */
#define JCV_COLECO_INPUT_5          (1 << 10)   /**< ColecoVision Numpad 5 */
#define JCV_COLECO_INPUT_6          (1 << 11)   /**< ColecoVision Numpad 6 */
#define JCV_COLECO_INPUT_7          (1 << 12)   /**< ColecoVision Numpad 7 */
#define JCV_COLECO_INPUT_8          (1 << 13)   /**< ColecoVision Numpad 8 */
#define JCV_COLECO_INPUT_9          (1 << 14)   /**< ColecoVision Numpad 9 */
#define JCV_COLECO_INPUT_0          (1 << 15)   /**< ColecoVision Numpad 0 */
#define JCV_COLECO_INPUT_STAR       (1 << 16)   /**< ColecoVision Numpad * */
#define JCV_COLECO_INPUT_POUND      (1 << 17)   /**< ColecoVision Numpad # */
#define JCV_COLECO_INPUT_Y          (1 << 18)   /**< ColecoVision SAC Yellow */
#define JCV_COLECO_INPUT_O          (1 << 19)   /**< ColecoVision SAC Orange */
#define JCV_COLECO_INPUT_P          (1 << 20)   /**< ColecoVision SAC Purple */
#define JCV_COLECO_INPUT_B          (1 << 22)   /**< ColecoVision SAC Blue */
#define JCV_COLECO_INPUT_SP_PLUS    (1 << 22)   /**< ColecoVision Spinner+ */
#define JCV_COLECO_INPUT_SP_MINUS   (1 << 23)   /**< ColecoVision Spinner- */
#define JCV_COLECO_INPUT_IRQ        (1 << 24)   /**< IRQ should be fired */

#define CRV_INPUT_UP                (1 << 0)    /**< CreatiVision Up */
#define CRV_INPUT_DOWN              (1 << 1)    /**< CreatiVision Down */
#define CRV_INPUT_LEFT              (1 << 2)    /**< CreatiVision Left */
#define CRV_INPUT_RIGHT             (1 << 3)    /**< CreatiVision Right */
#define CRV_INPUT_FIRE1             (1 << 4)    /**< CreatiVision Fire 1 */
#define CRV_INPUT_FIRE2             (1 << 5)    /**< CreatiVision Fire 2 */
#define CRV_INPUT_1                 (1 << 6)    /**< CreatiVision 1 */
#define CRV_INPUT_2                 (1 << 7)    /**< CreatiVision 2 */
#define CRV_INPUT_3                 (1 << 8)    /**< CreatiVision 3 */
#define CRV_INPUT_4                 (1 << 9)    /**< CreatiVision 4 */
#define CRV_INPUT_5                 (1 << 10)   /**< CreatiVision 5 */
#define CRV_INPUT_6                 (1 << 11)   /**< CreatiVision 6 */
#define CRV_INPUT_CNTL              (1 << 12)   /**< CreatiVision Cntl */
#define CRV_INPUT_Q                 (1 << 13)   /**< CreatiVision Q */
#define CRV_INPUT_W                 (1 << 14)   /**< CreatiVision W */
#define CRV_INPUT_E                 (1 << 15)   /**< CreatiVision E */
#define CRV_INPUT_R                 (1 << 16)   /**< CreatiVision R */
#define CRV_INPUT_T                 (1 << 17)   /**< CreatiVision T */
#define CRV_INPUT_BACKSPACE         (1 << 18)   /**< CreatiVision Backspace */
#define CRV_INPUT_A                 (1 << 19)   /**< CreatiVision A */
#define CRV_INPUT_S                 (1 << 20)   /**< CreatiVision S */
#define CRV_INPUT_D                 (1 << 21)   /**< CreatiVision D */
#define CRV_INPUT_F                 (1 << 22)   /**< CreatiVision F */
#define CRV_INPUT_G                 (1 << 23)   /**< CreatiVision G */
#define CRV_INPUT_SHIFT             (1 << 24)   /**< CreatiVision Shift */
#define CRV_INPUT_Z                 (1 << 25)   /**< CreatiVision Z */
#define CRV_INPUT_X                 (1 << 26)   /**< CreatiVision X */
#define CRV_INPUT_C                 (1 << 27)   /**< CreatiVision C */
#define CRV_INPUT_V                 (1 << 28)   /**< CreatiVision V */
#define CRV_INPUT_B                 (1 << 29)   /**< CreatiVision B */
#define CRV_INPUT_7                 (1 << 6)    /**< CreatiVision 7 */
#define CRV_INPUT_8                 (1 << 7)    /**< CreatiVision 8 */
#define CRV_INPUT_9                 (1 << 8)    /**< CreatiVision 9 */
#define CRV_INPUT_0                 (1 << 9)    /**< CreatiVision 0 */
#define CRV_INPUT_COLON             (1 << 10)   /**< CreatiVision : */
#define CRV_INPUT_MINUS             (1 << 11)   /**< CreatiVision - */
#define CRV_INPUT_Y                 (1 << 12)   /**< CreatiVision Y */
#define CRV_INPUT_U                 (1 << 13)   /**< CreatiVision U */
#define CRV_INPUT_I                 (1 << 14)   /**< CreatiVision I */
#define CRV_INPUT_O                 (1 << 15)   /**< CreatiVision O */
#define CRV_INPUT_P                 (1 << 16)   /**< CreatiVision P */
#define CRV_INPUT_RETN              (1 << 17)   /**< CreatiVision Retn */
#define CRV_INPUT_H                 (1 << 18)   /**< CreatiVision H */
#define CRV_INPUT_J                 (1 << 19)   /**< CreatiVision J */
#define CRV_INPUT_K                 (1 << 20)   /**< CreatiVision K */
#define CRV_INPUT_L                 (1 << 21)   /**< CreatiVision L */
#define CRV_INPUT_SEMICOLON         (1 << 22)   /**< CreatiVision ; */
#define CRV_INPUT_TAB               (1 << 23)   /**< CreatiVision Tab */
#define CRV_INPUT_N                 (1 << 24)   /**< CreatiVision N */
#define CRV_INPUT_M                 (1 << 25)   /**< CreatiVision M */
#define CRV_INPUT_COMMA             (1 << 26)   /**< CreatiVision , */
#define CRV_INPUT_PERIOD            (1 << 27)   /**< CreatiVision . */
#define CRV_INPUT_SLASH             (1 << 28)   /**< CreatiVision / */
#define CRV_INPUT_SPACE             (1 << 29)   /**< CreatiVision ' ' */

#define MYV_INPUT_1                 (1 << 0)    /**< My Vision 1 */
#define MYV_INPUT_2                 (1 << 1)    /**< My Vision 2 */
#define MYV_INPUT_3                 (1 << 2)    /**< My Vision 3 */
#define MYV_INPUT_4                 (1 << 3)    /**< My Vision 4 */
#define MYV_INPUT_5                 (1 << 4)    /**< My Vision 5 */
#define MYV_INPUT_6                 (1 << 5)    /**< My Vision 6 */
#define MYV_INPUT_7                 (1 << 6)    /**< My Vision 7 */
#define MYV_INPUT_8                 (1 << 7)    /**< My Vision 8 */
#define MYV_INPUT_9                 (1 << 8)    /**< My Vision 9 */
#define MYV_INPUT_10                (1 << 9)    /**< My Vision 10 */
#define MYV_INPUT_11                (1 << 10)   /**< My Vision 11 */
#define MYV_INPUT_12                (1 << 11)   /**< My Vision 12 */
#define MYV_INPUT_13                (1 << 12)   /**< My Vision 13 */
#define MYV_INPUT_14                (1 << 13)   /**< My Vision 14 */
#define MYV_INPUT_A                 (1 << 14)   /**< My Vision A */
#define MYV_INPUT_B                 (1 << 15)   /**< My Vision B */
#define MYV_INPUT_C                 (1 << 16)   /**< My Vision C */
#define MYV_INPUT_D                 (1 << 17)   /**< My Vision D */
#define MYV_INPUT_E                 (1 << 18)   /**< My Vision E */

/**
 * Set the ColecoVision input callback
 * @param cb Callback returning the input state to the emulator
 * @param u User data passed to callback
 */
void jcv_input_set_callback_coleco(unsigned (*cb)(const void*, int), void *u);

/**
 * Set the CreatiVision input callback
 * @param cb Callback returning the input state to the emulator
 * @param u User data passed to callback
 */
void jcv_input_set_callback_crvision(unsigned (*cb)(const void*, int), void *u);

/**
 * Set the My Vision input callback
 * @param cb Callback returning the input state to the emulator
 * @param u User data passed to callback
 */
void jcv_input_set_callback_myvision(unsigned (*cb)(const void*), void *u);

/**
 * Set the audio sample buffer
 * @param buf Pointer to the buffer used for audio samples
 */
void jcv_audio_set_buffer(void *buf);

/**
 * Set the audio callback
 * @param cb Callback which handles emulated sample data
 * @param u User data passed to callback
 */
void jcv_audio_set_callback(void (*cb)(const void*, size_t), void *u);

/**
 * Set the audio sample rate
 * @param rate Sample rate for resampled audio
 */
void jcv_audio_set_rate(size_t rate);

/**
 * Set the internal resampler quality level
 * @param rsqual Internal resampler quality level (0-10)
 */
void jcv_audio_set_rsqual(unsigned rsqual);

#define JCV_VIDEO_WIDTH_MAX        272  /**< Maximum video width */
#define JCV_VIDEO_HEIGHT_MAX       208  /**< Maximum video height */

/**
 * Set the video buffer
 * @param buf Pointer to the buffer used for video output
 */
void jcv_video_set_buffer(uint32_t *buf);

/**
 * Set the TMS9918 palette
 * @param p Palette to use for TMS9918 video
 */
void jcv_video_set_palette_tms9918(unsigned p);

/**
 * Initialize the emulator
 */
void jcv_init(void);

/**
 * Deinitialize the emulator
 */
void jcv_deinit(void);

/**
 * Reset the emulated system
 * @param hard Reset type (0 = soft, 1 = hard, 2 = first boot)
 */
void jcv_reset(int hard);

/**
 * Execute a single video frame worth of emulation
 */
void jcv_exec(void);

/**
 * Load a saved state
 * @param filename Filename of the state to load
 */
int jcv_state_load(const char *filename);

/**
 * Save the emulation state
 * @param filename Filename of the state to save
 */
int jcv_state_save(const char *filename);

/**
 * Get the size of the state for the current system/software being run
 * @return Size of the state
 */
size_t jcv_state_size(void);

/**
 * Load a saved state (raw)
 * @param s Block of memory to load the state from
 */
void jcv_state_load_raw(const void *s);

/**
 * Save the emulation state (raw)
 * @return Block of memory containing the emulation state
 */
const void* jcv_state_save_raw(void);

enum jcv_saveresult {
    JCV_SAVE_FAIL,      /**< Failed to save */
    JCV_SAVE_SUCCESS,   /**< Save successful */
    JCV_SAVE_NONE       /**< No save data */
};

/**
 * Load non-volatile save data (SRAM/EEPROM)
 * @param filename Filename of the non-volatile save data
 * @return Result of the operation
 */
int jcv_savedata_load(const char *filename);

/**
 * Save non-volatile save data (SRAM/EEPROM)
 * @param filename Filename of the non-volatile save data
 * @return Result of the operation
 */
int jcv_savedata_save(const char *filename);

/**
 * Load a BIOS image from a memory block
 * @param data Pointer to the memory block containing the BIOS image
 * @param size Size of the memory block containing the BIOS image
 * @return Result of the operation
 */
int jcv_bios_load(void *data, size_t size);

/**
 * Load a BIOS image from a memory block
 * @param biospath Filename of the BIOS image
 * @return Result of the operation
 */
int jcv_bios_load_file(const char *biospath);

/**
 * Load media (ROM, or otherwise) from a memory block
 * @param data Pointer to the memory block containing the data to load
 * @param size Size of the memory block containing the data to load
 * @return Result of the operation
 */
int jcv_media_load(void *data, size_t size);

enum jcv_memtype {
    JCV_MEM_SYSTEM,     /**< Main system RAM */
    JCV_MEM_VRAM,       /**< Video RAM */
    JCV_MEM_SAVEDATA    /**< Non-volatile save data */
};

/**
 * Get the pointer to a block of raw memory in the emulator
 * @param type Type of raw memory to retrieve
 * @return Pointer to the desired memory block
 */
void* jcv_memory_get_data(unsigned type);

/**
 * Get the size of a blck of memory in the emulator
 * @param type Type of raw memory to retrieve the size of
 * @return Size of the desired memory block
 */
size_t jcv_memory_get_size(unsigned type);

#ifdef __cplusplus
}
#endif
#endif
