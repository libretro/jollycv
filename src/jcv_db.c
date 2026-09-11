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

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "jollycv.h"

#include "jcv_db.h"

#include "jcv_coleco.h"

typedef struct _dbentry_t {
    const char *md5; unsigned flags; unsigned cart; unsigned special;
} dbentry_t;

/* There may be more PAL-only releases, but many dumps marked as "Europe" are
   actually worldwide releases - for now, don't use this list for anything
   "3ae1ddd596faaced37274dd407d5523c" // Cosmic Crisis (Europe)
   "8f1f9e8267b51d8b9b29fb62f0c050ec" // Meteoric Shower (Europe)
   "8aabed060476fde3cc706c6463f02980" // Super Action Football (Europe)
   "a31facd8adc1134942d9f4102dd3fa9f" // Victory (Europe)
*/
static dbentry_t db_coleco[] = {
    // Super Action Controller
    // Front Line (USA, Europe)
        { "4520ee5d8d0fcf151a3332966f7ebda0", JCV_DB_COLECO_SAC, 0, 0 },
    // Front Line (USA, Europe) (Green Version)
        { "d145de191e3f694c7f0920787ccbda48", JCV_DB_COLECO_SAC, 0, 0 },
    // Rocky - Super Action Boxing (USA, Europe)
        { "d35fdb81f4a733925b0a33dfb53d9d78", JCV_DB_COLECO_SAC, 0, 0 },
    // Spy Hunter (USA)
        { "f96a21f920e889d1e21abbf00f4d381d", JCV_DB_COLECO_SAC, 0, 0 },
    // Spy Hunter (USA) (Beta)
        { "7da9f2fda17e1e34a41b180d1ceb0c37", JCV_DB_COLECO_SAC, 0, 0 },
    // Star Trek - Strategic Operations Simulator (USA)
        { "45006eaf52ee16ddcadd1dca68b265c8", JCV_DB_COLECO_SAC, 0, 0 },
    // Super Action Baseball (USA)
        { "4c4b25a93301e59b86decb0df7a0ee51", JCV_DB_COLECO_SAC, 0, 0 },
    // Super Action Football (Europe)
        { "8aabed060476fde3cc706c6463f02980", JCV_DB_COLECO_SAC, 0, 0 },
    // Super Action Football (USA)
        { "bee90a110d14b29d2e64f0ff0f303bc6", JCV_DB_COLECO_SAC, 0 , 0},

    // Roller Controller
    // Slither (USA, Europe)
        { "7cdc148dff40389fa1ad012d4734ceed", JCV_DB_COLECO_ROLLER, 0, 0 },
    // Victory (Europe)
        { "a31facd8adc1134942d9f4102dd3fa9f", JCV_DB_COLECO_ROLLER, 0, 0 },
    // Victory (USA)
        { "200aa603996bfd2734e353098ebe8dd5", JCV_DB_COLECO_ROLLER, 0, 0 },

    // Steering Wheel
    // Destructor (USA, Europe)
        { "ec72a0e3bebe07ba631a8dcb750c1591", JCV_DB_COLECO_WHEEL, 0, 0 },
    // Dukes of Hazzard, The (USA)
        { "dbd4f21702be17775e84b2fb6c534c94", JCV_DB_COLECO_WHEEL, 0, 0 },
    // Turbo (USA, Europe)
        { "6f146d9bd3f64bbc006a761f59e2a1cf", JCV_DB_COLECO_WHEEL, 0, 0 },

    // Super Sketch
    //Super Sketch - Sketch Master (USA)
        { "a46d20d65533ed979933fc1cfe6c0ad7", JCV_DB_COLECO_SKETCH, 0, 0 },

    // Games containing SRAM
    // Lord of the Dungeon (USA) (Beta)
        { "d5964ac4e7b1fd3ae91a8008ef57a3cc", JCV_DB_COLECO_PAD, CART_SRAM, 0 },

    // Games containing an EEPROM (Activision style)
    // Boxxle
        { "6acf055212043cd047202adc3316e85c", JCV_DB_COLECO_PAD,
            CART_ACTIVISION, SIZE_32K },
    // Black Onyx, The
        { "aa9c71e6b97a1ec3ff8ec4e7905a5da6", JCV_DB_COLECO_PAD,
            CART_ACTIVISION, 0x100 }, // 256B
    // Jewel Panic
        { "46fbe0b1d921e7970ede44200f5141a4", JCV_DB_COLECO_PAD,
            CART_ACTIVISION, 0x100 }, // 256B
};

/* CreatiVision ROMs are distributed with file extensions which overlap with
   those used for other systems, so the only reliable way to tell them apart
   is by checksum. This list is used to detect the system a ROM was made for.
*/
static const char *db_crvision[] = {
    // Air Sea Attack (Asia, Europe)
        "2a3000132c6ae8f6712eaee387ad851c",
    // Astro Pinball (Asia, Europe)
        "da1efcd608ee688ec76fb806b28a8f04",
    // Auto Chase (Asia, Europe)
        "b6bfea8987d76872661e1dd25ba5d542",
    // Chopper Rescue (Asia, Europe) (Alt 1)
        "91a60dbb91668023629ae5165cbab9d5",
    // Chopper Rescue (Asia, Europe) (Alt 2)
        "33c422e85fcd342e722503e776726dd7",
    // Chopper Rescue (Asia, Europe)
        "6c3abd51286ee689d3576cd6f50269ff",
    // Crazy Chicky (Asia, Europe)
        "46831e263fdd536a7740861d28f24cc2",
    // Crazy Pucker (Asia, Europe)
        "93a92da8d9c40e0f53c8e0f681021f18",
    // CreatiVision Basic (Asia, Europe)
        "0e43eb5caf9c0128998d04e033016556",
    // Deep Sea Adventure (Asia, Europe)
        "11ad7d0a0caead0e6d22f055dc0aba6e",
    // Hapmon (Asia, Europe)
        "3d3dc6cb1374cca0b2a7fb4e898c98c8",
    // Locomotive (Asia, Europe)
        "f86dc1a3df3e1f25ebd60c0f1de4fdb5",
    // Mouse Puzzle (Asia, Europe)
        "136caa24d077372d94594cc722a00cf8",
    // Music Maker (Asia, Europe)
        "c19cbb873193f6441ff793b82a5ec19c",
    // Planet Defender (Asia, Europe) (Alt 1)
        "02cd5364ee077e02b3d0e1c0a6d2adef",
    // Planet Defender (Asia, Europe)
        "8aecbd20173bf4af56a1eb641cba492e",
    // Police Jump (Asia, Europe)
        "9cb73c1ca4853ba299a97ed55b17727e",
    // Soccer (Asia, Europe)
        "facb2b5308502f8acec4d77eaed6ea3a",
    // Sonic Invader (Asia, Europe)
        "fa49471a2d8d3482fddd77a35655aa26",
    // Stone Age (Asia, Europe)
        "3b4d8c419fb1bbbd2324ed29dae77c7b",
    // Tank Attack (Asia, Europe)
        "68fc7cf9039568e8be17d2175d1e342c",
    // Tennis (Asia, Europe) (DSE)
        "1bda4ee02466b98c6c4cf8c962838d3f",
    // Tennis (Asia, Europe) (VTL)
        "1a60104942ef675d6d34c0c3361025de",
};

static uint32_t flags = 0;

uint32_t jcv_db_get_flags(void) {
    return flags;
}

int jcv_db_detect_system(const char *md5) {
    // Loop through the system specific databases and compare the MD5 checksum
    for (size_t i = 0; i < (sizeof(db_crvision) / sizeof(const char*)); ++i) {
        if (!strcmp(md5, db_crvision[i]))
            return JCV_SYS_CRVISION;
    }

    return -1; // No match - the caller decides what to do with an unknown ROM
}

void jcv_db_process_coleco(const char *md5) {
    // Loop through the database and compare the MD5 checksum
    flags = 0;
    for (size_t i = 0; i < (sizeof(db_coleco) / sizeof(dbentry_t)); ++i) {
        if (!strcmp(md5, db_coleco[i].md5)) {
            // Match found - set the cart type and flags
            jcv_coleco_set_carttype(db_coleco[i].cart, db_coleco[i].special);
            flags = db_coleco[i].flags;
            return;
        }
    }
}
