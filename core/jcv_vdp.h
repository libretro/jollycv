/*
 * Copyright (c) 2020 Rupert Carmichael
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef JCV_VDP_H
#define JCV_VDP_H

#define CV_VDP_WIDTH 256
#define CV_VDP_HEIGHT 192
#define CV_VDP_SCANLINES 262
#define CV_VDP_SCANLINES_PAL 313

#define SIZE_VRAM 0x4000

typedef struct _cv_vdp_t {
    int line; // Line currently being drawn
    int dot; // Dot currently being drawn
    uint8_t vram[SIZE_VRAM]; // 16K VRAM
    uint16_t addr; // Memory Address - 14 bit address
    uint8_t dlatch; // Data Latch (general purpose 8-bit data register)
    uint8_t wlatch; // Write Latch
    uint8_t ctrl[8]; // 8 Control Registers - write only
    uint8_t stat; // Status Register - read only
    uint16_t tbl_col; // Address for Colour table
    uint16_t tbl_pgen; // Address for Pattern Generator table
    uint16_t tbl_pname; // Address for Pattern Name table
    uint16_t tbl_sattr; // Address for Sprite Attribute table
    uint16_t tbl_spgen; // Addresss for Sprite Generator table
} cv_vdp_t;

void jcv_vdp_init(void);

void jcv_vdp_set_buffer(uint32_t*);
void jcv_vdp_set_palette(uint8_t);
void jcv_vdp_set_region(uint8_t);

uint8_t jcv_vdp_rd_data(void);
uint8_t jcv_vdp_rd_stat(void);

void jcv_vdp_wr_ctrl(uint8_t);
void jcv_vdp_wr_data(uint8_t);

void jcv_vdp_exec(void);

void jcv_vdp_state_load(cv_vdp_t*);
void jcv_vdp_state_save(cv_vdp_t*);

#endif
