/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _DISP_GAMMA_H_
#define _DISP_GAMMA_H_

static unsigned int gam_lut[3][16] = {
    // R
    [0] = {
        0x0C080400, 0x1C181410, 0x2C282420, 0x3C383430,
        0x4C484440, 0x5D595550, 0x6D696561, 0x7D797571,
        0x8D898581, 0x9D999591, 0xAEAAA5A1, 0xBEBAB6B2,
        0xCECAC6C2, 0xDEDAD6D2, 0xEEEAE6E2, 0xFFFAF6F2,
    },
    // G
    [1] = {
        0x0C080400, 0x1C181410, 0x2C282420, 0x3C383430,
        0x4C484440, 0x5D595550, 0x6D696561, 0x7D797571,
        0x8D898581, 0x9D999591, 0xAEAAA5A1, 0xBEBAB6B2,
        0xCECAC6C2, 0xDEDAD6D2, 0xEEEAE6E2, 0xFFFAF6F2,
    },
    // B
    [2] = {
        0x0C080400, 0x1C181410, 0x2C282420, 0x3C383430,
        0x4C484440, 0x5D595550, 0x6D696561, 0x7D797571,
        0x8D898581, 0x9D999591, 0xAEAAA5A1, 0xBEBAB6B2,
        0xCECAC6C2, 0xDEDAD6D2, 0xEEEAE6E2, 0xFFFAF6F2,
    }
};

#endif /* _DISP_GAMMA_H_ */