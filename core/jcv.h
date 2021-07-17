/*
 * Copyright (c) 2020-2021 Rupert Carmichael
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// An idiot admires complexity, a genius admires simplicity - Terry A. Davis

#ifndef JCV_H
#define JCV_H

#define REGION_NTSC 0
#define REGION_PAL 1

#define VERSION "0.2.0-pre1"

void jcv_set_region(uint8_t);
void jcv_init(void);
void jcv_deinit(void);
void jcv_reset(int);
void jcv_exec(void);

#endif
