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

#ifndef JG_MYVISION_H
#define JG_MYVISION_H

enum jg_myvision_input_type {
    JG_MYVISION_SYSTEM
};

static const char *jg_myvision_input_name[] = {
    "MyVision System"
};

#define NDEFS_MYVISION 19
static const char *defs_myvision[NDEFS_MYVISION] = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14",
    "A", "B", "C", "D", "E"
};


static jg_inputinfo_t jg_myvision_inputinfo(int index, int type) {
    jg_inputinfo_t ret;
    ret.type = JG_INPUT_EXTERNAL;
    ret.index = index;
    ret.name = "myvision";
    ret.fname = jg_myvision_input_name[type];
    ret.defs = defs_myvision;
    ret.numaxes = 0;
    ret.numbuttons = NDEFS_MYVISION;
    return ret;
}

#endif
