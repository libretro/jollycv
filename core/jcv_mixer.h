/*
 * Copyright (c) 2020 Rupert Carmichael
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef JCV_MIXER_H
#define JCV_MIXER_H

void jcv_mixer_deinit(void);
void jcv_mixer_init(void);

void jcv_mixer_set_buffer(int16_t*);
void jcv_mixer_set_callback(void (*)(size_t));
void jcv_mixer_set_rate(size_t);
void jcv_mixer_set_region(uint8_t);
void jcv_mixer_set_rsqual(uint8_t);
void jcv_mixer_resamp(size_t, size_t);

#endif
