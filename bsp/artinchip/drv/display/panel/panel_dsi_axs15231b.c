/*
 * Copyright (c) 2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "panel_com.h"
#include "panel_dsi.h"

static int panel_enable(struct aic_panel *panel)
{
    int ret;

    panel_di_enable(panel, 0);
    panel_dsi_send_perpare(panel);

    panel_dsi_generic_send_seq(panel, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                               0x5A, 0xA5);
    panel_dsi_generic_send_seq(panel, 0xA0, 0x00, 0x30, 0x00, 0x02, 0x00, 0x00,
                               0x04, 0x3F, 0x20, 0x05, 0x3F, 0x3F, 0x00, 0x00,
                               0x00, 0x00, 0x00);
    panel_dsi_generic_send_seq(panel, 0xA2, 0x20, 0x19, 0x60, 0x64, 0x9b, 0x22,
                               0x50, 0x80, 0xAC, 0x28, 0x7F, 0x7F, 0x7F, 0x20,
                               0xF8, 0x10, 0x02, 0xFF, 0xFF, 0xF0, 0x90, 0x01,
                               0x32, 0xA0, 0x91, 0xC0, 0x10, 0x7F, 0xFF, 0x00,
                               0x04);
    panel_dsi_generic_send_seq(panel, 0xD0, 0x80, 0xAC, 0x21, 0x24, 0x08, 0x09,
                               0x10, 0x01, 0x80, 0x12, 0xC2, 0x00, 0x22, 0x22,
                               0xAA, 0x03, 0x10, 0x12, 0x40, 0x14, 0x1E, 0x51,
                               0x15, 0x00, 0x40, 0x10, 0x00, 0x03, 0x3D, 0x12);
    panel_dsi_generic_send_seq(panel, 0xA3, 0xA0, 0x06, 0xAA, 0x08, 0x08, 0x02,
                               0x0A, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
                               0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x55, 0x55);
    panel_dsi_generic_send_seq(panel, 0xC1, 0x31, 0x04, 0x02, 0x02, 0x71, 0x05,
                               0x24, 0x55, 0x02, 0x00, 0x41, 0x01, 0x53, 0xFF,
                               0xFF, 0xFF, 0x4F, 0x52, 0x00, 0x4F, 0x52, 0x00,
                               0x45, 0x3B, 0x0B, 0x02, 0x0D, 0x00, 0xFF, 0x40);
    panel_dsi_generic_send_seq(panel, 0xC3, 0x00, 0x00, 0x00, 0x50, 0x03, 0x00,
                               0x00, 0x00, 0x01, 0x80, 0x01);
    panel_dsi_generic_send_seq(panel, 0xC4, 0x00, 0x24, 0x33, 0x90, 0x0b, 0xea,
                               0x64, 0x32, 0xC8, 0x64, 0xC8, 0x32, 0x90, 0x90,
                               0x11, 0x06, 0xDC, 0xFA, 0x04, 0x03, 0x80, 0xFE,
                               0x10, 0x10, 0x00, 0x0A, 0x0A, 0x44, 0x50);
    panel_dsi_generic_send_seq(panel, 0xC5, 0x18, 0x00, 0x00, 0x03, 0xFE, 0x08,
                               0x68, 0x30, 0x30, 0x10, 0x88, 0xDE, 0x0D, 0x08,
                               0x0F, 0x0F, 0x01, 0x08, 0x68, 0x30, 0x10, 0x10,
                               0x00);
    panel_dsi_generic_send_seq(panel, 0xC6, 0x05, 0x0A, 0x05, 0x0A, 0x00, 0xE0,
                               0x2E, 0x0B, 0x12, 0x22, 0x12, 0x22, 0x01, 0x00,
                               0x00, 0x02, 0x6A, 0x18, 0xC8, 0x22);
    panel_dsi_generic_send_seq(panel, 0xC7, 0x50, 0x32, 0x28, 0x00, 0xa2, 0x80,
                               0x8f, 0x00, 0x80, 0xff, 0x07, 0x11, 0x9c, 0x6f,
                               0xff, 0x24, 0x0c, 0x0d, 0x0e, 0x0f);
    panel_dsi_generic_send_seq(panel, 0xC9, 0x33, 0x44, 0x44, 0x01);
    panel_dsi_generic_send_seq(panel, 0xCF, 0x34, 0x1E, 0x88, 0x58, 0x13, 0x18,
                               0x56, 0x18, 0x1E, 0x68, 0xF8, 0x00, 0x66, 0x0d,
                               0x22, 0xC4, 0x0C, 0x77, 0x22, 0x44, 0xAA, 0x55,
                               0x04, 0x04, 0x12, 0xA0, 0x08);
    panel_dsi_generic_send_seq(panel, 0xD5, 0x10, 0x61, 0x89, 0x00, 0x35, 0x04,
                               0x73, 0x03, 0x03, 0x73, 0x03, 0x03, 0x04, 0x03,
                               0x13, 0x46, 0x13, 0x60, 0x13, 0x60, 0x86, 0x00,
                               0x00, 0x00, 0x80, 0x52, 0x7D, 0x10, 0x10, 0x00);
    panel_dsi_generic_send_seq(panel, 0xD6, 0x10, 0x32, 0x54, 0x76, 0x98, 0xBA,
                               0xDC, 0xFE, 0x00, 0x00, 0x01, 0x83, 0x04, 0x04,
                               0x33, 0x04, 0x04, 0x33, 0x60, 0x60, 0x60, 0x60,
                               0x61, 0x60, 0x00, 0xA4, 0x51, 0x23, 0x01, 0x00);
    panel_dsi_generic_send_seq(panel, 0xD7, 0x18, 0x1A, 0x1B, 0x1F, 0x0A, 0x08,
                               0x0E, 0x0C, 0x00, 0x1F, 0x1d, 0x1F, 0x10, 0x61,
                               0x04, 0x00, 0x1F, 0x00, 0x1F);
    panel_dsi_generic_send_seq(panel, 0xD8, 0x18, 0x1A, 0x1B, 0x1f, 0x0B, 0x09,
                               0x0F, 0x0D, 0x01, 0x1F, 0x1D, 0x1F);
    panel_dsi_generic_send_seq(panel, 0xD9, 0x0F, 0x09, 0x0B, 0x1F, 0x18, 0x19,
                               0x1F, 0x01, 0x1E, 0x1D, 0x1F);
    panel_dsi_generic_send_seq(panel, 0xDD, 0x0E, 0x08, 0x0A, 0x1F, 0x18, 0x19,
                               0x1F, 0x00, 0x1E, 0x1A, 0x1F);
    panel_dsi_generic_send_seq(panel, 0xDF, 0x44, 0x73, 0x4B, 0x69, 0x00, 0x0A,
                               0x02, 0x90);
    panel_dsi_generic_send_seq(panel, 0xE0, 0x35, 0x08, 0x1B, 0x1F, 0x0F, 0x0C,
                               0x14, 0x2C, 0x54, 0x22, 0x0C, 0x16, 0x14, 0x25,
                               0x27, 0x08, 0x00);
    panel_dsi_generic_send_seq(panel, 0xE1, 0x3e, 0x08, 0x1B, 0x1F, 0x0F, 0x0C,
                               0x14, 0x2C, 0x54, 0x22, 0x0C, 0x16, 0x14, 0x25,
                               0x27, 0x08, 0x0f);
    panel_dsi_generic_send_seq(panel, 0xE2, 0x19, 0x20, 0x0A, 0x11, 0x09, 0x06,
                               0x11, 0x25, 0xD4, 0x22, 0x0B, 0x13, 0x12, 0x2D,
                               0x32, 0x2f, 0x03);
    panel_dsi_generic_send_seq(panel, 0xE3, 0x38, 0x20, 0x0A, 0x11, 0x09, 0x06,
                               0x11, 0x25, 0xC4, 0x21, 0x0A, 0x12, 0x11, 0x2C,
                               0x32, 0x2f, 0x27);
    panel_dsi_generic_send_seq(panel, 0xE4, 0x19, 0x20, 0x0D, 0x14, 0x0D, 0x08,
                               0x12, 0x2A, 0xD4, 0x26, 0x0E, 0x15, 0x13, 0x34,
                               0x39, 0x2f, 0x03);
    panel_dsi_generic_send_seq(panel, 0xE5, 0x38, 0x20, 0x0D, 0x13, 0x0D, 0x07,
                               0x12, 0x29, 0xC4, 0x25, 0x0D, 0x15, 0x12, 0x33,
                               0x39, 0x2f, 0x27);
    panel_dsi_generic_send_seq(panel, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                               0x00, 0x00);

    ret = panel_dsi_dcs_exit_sleep_mode(panel);
    if (ret < 0) {
        pr_err("Failed to exit sleep mode: %d\n", ret);
        return ret;
    }
    aic_delay_ms(200);

    ret = panel_dsi_dcs_set_display_on(panel);
    if (ret < 0) {
        pr_err("Failed to set display on: %d\n", ret);
        return ret;
    }
    aic_delay_ms(120);

    panel_dsi_setup_realmode(panel);

    panel_de_timing_enable(panel, 0);
    panel_backlight_enable(panel, 0);
    return 0;
}

static struct aic_panel_funcs panel_funcs = {
    .disable = panel_default_disable,
    .unprepare = panel_default_unprepare,
    .prepare = panel_default_prepare,
    .enable = panel_enable,
    .register_callback = panel_register_callback,
};

static struct display_timing axs15231b_timing = {
    .pixelclock   = 26000000,

    .hactive      = 172,
    .hfront_porch = 155,
    .hback_porch  = 155,
    .hsync_len    = 60,

    .vactive      = 640,
    .vfront_porch = 22,
    .vback_porch  = 100,
    .vsync_len    = 25,
};

static struct panel_dsi dsi = {
    .mode     = DSI_MOD_VID_PULSE,
    .format   = DSI_FMT_RGB888,
    .lane_num = 1,
};

struct aic_panel dsi_axs15231b = {
    .name           = "panel-axs15231b",
    .timings        = &axs15231b_timing,
    .funcs          = &panel_funcs,
    .dsi            = &dsi,
    .connector_type = AIC_MIPI_COM,
};
