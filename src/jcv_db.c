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
        { "4520ee5d8d0fcf151a3332966f7ebda0", DB_COLECO_SAC, 0, 0 },
    // Front Line (USA, Europe) (Green Version)
        { "d145de191e3f694c7f0920787ccbda48", DB_COLECO_SAC, 0, 0 },
    // Rocky - Super Action Boxing (USA, Europe)
        { "d35fdb81f4a733925b0a33dfb53d9d78", DB_COLECO_SAC, 0, 0 },
    // Spy Hunter (USA)
        { "f96a21f920e889d1e21abbf00f4d381d", DB_COLECO_SAC, 0, 0 },
    // Spy Hunter (USA) (Beta)
        { "7da9f2fda17e1e34a41b180d1ceb0c37", DB_COLECO_SAC, 0, 0 },
    // Star Trek - Strategic Operations Simulator (USA)
        { "45006eaf52ee16ddcadd1dca68b265c8", DB_COLECO_SAC, 0, 0 },
    // Super Action Baseball (USA)
        { "4c4b25a93301e59b86decb0df7a0ee51", DB_COLECO_SAC, 0, 0 },
    // Super Action Football (Europe)
        { "8aabed060476fde3cc706c6463f02980", DB_COLECO_SAC, 0, 0 },
    // Super Action Football (USA)
        { "bee90a110d14b29d2e64f0ff0f303bc6", DB_COLECO_SAC, 0 , 0},

    // Roller Controller
    // Slither (USA, Europe)
        { "7cdc148dff40389fa1ad012d4734ceed", DB_COLECO_ROLLER, 0, 0 },
    // Victory (Europe)
        { "a31facd8adc1134942d9f4102dd3fa9f", DB_COLECO_ROLLER, 0, 0 },
    // Victory (USA)
        { "200aa603996bfd2734e353098ebe8dd5", DB_COLECO_ROLLER, 0, 0 },

    // Steering Wheel
    // Destructor (USA, Europe)
        { "ec72a0e3bebe07ba631a8dcb750c1591", DB_COLECO_WHEEL, 0, 0 },
    // Dukes of Hazzard, The (USA)
        { "dbd4f21702be17775e84b2fb6c534c94", DB_COLECO_WHEEL, 0, 0 },
    // Turbo (USA, Europe)
        { "6f146d9bd3f64bbc006a761f59e2a1cf", DB_COLECO_WHEEL, 0, 0 },

    // Super Sketch
    //Super Sketch - Sketch Master (USA)
        { "a46d20d65533ed979933fc1cfe6c0ad7", DB_COLECO_SKETCH, 0, 0 },

    // Games containing SRAM
    // Lord of the Dungeon (USA) (Beta)
        { "d5964ac4e7b1fd3ae91a8008ef57a3cc", DB_COLECO_PAD, CART_SRAM, 0 },

    // Games containing an EEPROM (Activision style)
    // Boxxle
        { "6acf055212043cd047202adc3316e85c", DB_COLECO_PAD, CART_ACTIVISION,
            SIZE_32K },
    // Black Onyx, The
        { "aa9c71e6b97a1ec3ff8ec4e7905a5da6", DB_COLECO_PAD, CART_ACTIVISION,
            0x100 }, // 256B
};

uint32_t jcv_db_process_coleco(const char *md5) {
    // Loop through the database and compare the MD5 checksum
    for (size_t i = 0; i < (sizeof(db_coleco) / sizeof(dbentry_t)); ++i) {
        if (!strcmp(md5, db_coleco[i].md5)) {
            // Match found - return the flags
            jcv_coleco_set_carttype(db_coleco[i].cart, db_coleco[i].special);
            return db_coleco[i].flags;
        }
    }
    return 0;
}
