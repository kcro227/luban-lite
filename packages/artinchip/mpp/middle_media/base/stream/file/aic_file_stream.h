/*
 * Copyright (C) 2020-2024 ArtInChip Technology Co. Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: <jun.ma@artinchip.com>
 * Desc: aic file stream
 */

#ifndef __AIC_FILE_STREAM_H__
#define __AIC_FILE_STREAM_H__

#include "aic_stream.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

s32 file_stream_open(const char* uri, struct aic_stream **file_stream, int flags);

#ifdef __cplusplus
}
#endif /* End of #ifdef __cplusplus */
#endif

