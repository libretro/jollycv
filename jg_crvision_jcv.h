/*
zlib License

Copyright (c) 2020-2025 Rupert Carmichael

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#ifndef JG_CRVISION_H
#define JG_CRVISION_H

enum jg_crvision_input_type {
    JG_CRVISION_UNCONNECTED,
    JG_CRVISION_LPAD,
    JG_CRVISION_RPAD
};

static const char *jg_crvision_input_name[] = {
    "Unconnected",
    "CreatiVision Left Paddle",
    "CreatiVision Right Paddle"
};

#define NDEFS_CRVISIONPAD 30
static const char *defs_crvision_lpad[NDEFS_CRVISIONPAD] = {
    "Up", "Down", "Left", "Right", "Fire1", "Fire2",
    "1", "2", "3", "4", "5", "6",
    "Cntl", "Q", "W", "E", "R", "T",
    "Backspace", "A", "S", "D", "F", "G",
    "Shift", "Z", "X", "C", "V", "B"
};

static const char *defs_crvision_rpad[NDEFS_CRVISIONPAD] = {
    "Up", "Down", "Left", "Right", "Fire1", "Fire2",
    "7", "8", "9", "0", "Colon", "Minus",
    "Y", "U", "I", "O", "P", "Retn",
    "H", "J", "K", "L", "Semicolon", "Tab",
    "N", "M", "Comma", "Period", "Slash", "Space"
};

static jg_inputinfo_t jg_crvision_inputinfo(int index, int type) {
    jg_inputinfo_t ret;
    switch (type) {
        case JG_CRVISION_LPAD: {
            ret.type = JG_INPUT_CONTROLLER;
            ret.index = index;
            ret.name = "crvisionlpad";
            ret.fname = jg_crvision_input_name[type];
            ret.defs = defs_crvision_lpad;
            ret.numaxes = 0;
            ret.numbuttons = NDEFS_CRVISIONPAD;
            return ret;
        }
        case JG_CRVISION_RPAD: {
            ret.type = JG_INPUT_CONTROLLER;
            ret.index = index;
            ret.name = "crvisionrpad";
            ret.fname = jg_crvision_input_name[type];
            ret.defs = defs_crvision_rpad;
            ret.numaxes = 0;
            ret.numbuttons = NDEFS_CRVISIONPAD;
            return ret;
        }
        default: {
            ret.type = JG_INPUT_EXTERNAL;
            ret.index = index;
            ret.name = "unconnected";
            ret.fname = jg_crvision_input_name[0];
            ret.defs = NULL;
            ret.numaxes = ret.numbuttons = 0;
            return ret;
        }
    }
}

#endif
