/*
 * Copyright (c) 2023-2024, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Wu Dehuang <dehuang.wu@artinchip.com>
 */

#include <string.h>
#include <malloc.h>
#include <aicupg.h>
#include <aic_core.h>
#include <partition_table.h>
#include "upg_internal.h"

#ifdef IMAGE_CFG_JSON_PARTS_GPT
#define GPT_PART_TABLE IMAGE_CFG_JSON_PARTS_GPT
#else
#define GPT_PART_TABLE ""
#endif

/*
 * UPG_PROTO_CMD_SET_FWC_META:
 *   -> [CMD HEADER]
 *   -> [DATA from host]
 *   <- [RESP HEADER]
 *
 * In RAM device, it only receive and process one type of FWC:
 *   - Firmware information
 *   This Firmware Component describes which storage device type firmware will
 *   be written.
 */
static struct fwc_info *fwc_info_data;
static void set_fwc_meta_cmdstart(struct upg_cmd *cmd, s32 cmd_data_len)
{
    pr_debug("%s data len %d\n", __func__, cmd_data_len);
    if (cmd->cmd != UPG_PROTO_CMD_SET_FWC_META)
        return;

    cmd_state_init(cmd, CMD_STATE_START);
    if (fwc_info_data == NULL) {
        fwc_info_data = malloc(sizeof(struct fwc_info));
        if (fwc_info_data == NULL) {
            pr_debug("%s fwc_info_data = NULL\n", __func__);
            return;
        }
    }

    memset(fwc_info_data, 0, sizeof(struct fwc_info));
    fwc_info_data->burn_result = -1;
    fwc_info_data->run_result = -1;
    cmd->priv = fwc_info_data;
}

static s32 set_fwc_meta_cmdwrite_input_data(struct upg_cmd *cmd, u8 *buf,
                                             s32 len)
{
    struct fwc_info *fwc;
    s32 clen = 0;

    pr_debug("%s\n", __func__);
    fwc = (struct fwc_info *)cmd->priv;
    if (fwc == NULL)
        return 0;

    if (cmd->state == CMD_STATE_START)
        cmd_state_set_next(cmd, CMD_STATE_DATA_IN);

    if (cmd->state == CMD_STATE_DATA_IN) {
        /* Data length should be FWC_META_DATA_LEN */
        if (len > FWC_META_DATA_LEN)
            len = FWC_META_DATA_LEN;
        memcpy(&fwc->meta, buf, len);
        clen = len;
        cmd_state_set_next(cmd, CMD_STATE_RESP);
    }

    return clen;
}

static s32 set_fwc_meta_cmdread_output_data(struct upg_cmd *cmd, u8 *buf,
                                             s32 len)
{
    struct resp_header resp;
    struct fwc_info *fwc;
    s32 siz = 0;

    fwc = (struct fwc_info *)cmd->priv;
    if (fwc == NULL)
        return 0;

    if (cmd->state == CMD_STATE_RESP) {
        /*
         * Enter read RESP state, to make it simple, HOST should read
         * RESP in one read operation.
         */
        aicupg_gen_resp(&resp, cmd->cmd, UPG_RESP_OK, 0);
        siz = sizeof(struct resp_header);
        memcpy(buf, &resp, siz);
        cmd_state_set_next(cmd, CMD_STATE_END);
    }

    return siz;
}

static void set_fwc_meta_cmdend(struct upg_cmd *cmd)
{
    enum upg_dev_type dev_type;
    struct fwc_info *fwc;

    fwc = (struct fwc_info *)cmd->priv;
    if (fwc == NULL)
        return;

    printf("Firmware Component:\n");
    printf("    name:      %s\n", fwc->meta.name);
    printf("    partition: %s\n", fwc->meta.partition);
    printf("    attr:      %s\n", fwc->meta.attr);
    if (cmd->state == CMD_STATE_END) {
        if (!memcmp(fwc->meta.name, "image.updater", 13)) {
            set_current_device_type(UPG_DEV_TYPE_RAM);
            set_current_device_id(0);
        }

        fwc->start_us = aic_get_time_us();

        dev_type = get_current_device_type();
        printf("    Media:     %s(%d)\n", get_current_device_name(dev_type),
               dev_type);
        switch (dev_type) {
            case UPG_DEV_TYPE_RAM:
                ram_fwc_start(fwc);
                break;
#if defined(AICUPG_MMC_ARTINCHIP)
            case UPG_DEV_TYPE_MMC:
                mmc_fwc_start(fwc);
                break;
#endif
#if defined(AICUPG_NAND_ARTINCHIP)
            case UPG_DEV_TYPE_RAW_NAND:
            case UPG_DEV_TYPE_SPI_NAND:
                nand_fwc_start(fwc);
                break;
#endif
#if defined(AICUPG_NOR_ARTINCHIP)
            case UPG_DEV_TYPE_SPI_NOR:
                nor_fwc_start(fwc);
                break;
#endif
            case UPG_DEV_TYPE_UNKNOWN:
            default:
                break;
        }
        cmd->priv = 0;
        cmd_state_set_next(cmd, CMD_STATE_IDLE);
    }
}

/*
 * UPG_PROTO_CMD_GET_BLOCK_SIZE:
 *   -> [CMD HEADER]
 *   <- [RESP HEADER]
 *   <- [BLOCK SIZE]
 */
static void get_block_size_cmd_start(struct upg_cmd *cmd, s32 cmd_data_len)
{
    pr_debug("%s\n", __func__);
    if (cmd->cmd != UPG_PROTO_CMD_GET_BLOCK_SIZE)
        return;
    /* fwc meta data should be sent before this command */
    if (fwc_info_data == NULL)
        return;
    cmd_state_init(cmd, CMD_STATE_START);
    cmd->priv = fwc_info_data;
}

static s32 get_block_size_cmd_write_input_data(struct upg_cmd *cmd, u8 *buf,
                                               s32 len)
{
    /* No input data for this command */
    return 0;
}

static s32 get_block_size_cmd_read_output_data(struct upg_cmd *cmd, u8 *buf,
                                               s32 len)
{
    struct resp_header resp;
    struct fwc_info *fwc;
    u32 siz = 0, val = 0;

    pr_debug("%s\n", __func__);
    fwc = (struct fwc_info *)cmd->priv;
    if (fwc == NULL)
        return 0;

    if (cmd->state == CMD_STATE_START)
        cmd_state_set_next(cmd, CMD_STATE_RESP);

    if (cmd->state == CMD_STATE_RESP) {
        /*
         * Enter read RESP state, to make it simple, HOST should read
         * RESP in one read operation.
         */
        aicupg_gen_resp(&resp, cmd->cmd, 0, 4);
        siz = sizeof(struct resp_header);
        memcpy(buf, &resp, siz);
        cmd_state_set_next(cmd, CMD_STATE_DATA_OUT);
    }
    if (siz == len)
        return siz;

    if (cmd->state == CMD_STATE_DATA_OUT) {
        val = fwc->block_size;
        memcpy(buf, &val, 4);
        siz += 4;
        cmd_state_set_next(cmd, CMD_STATE_END);
    }

    return siz;
}

static void get_block_size_cmd_end(struct upg_cmd *cmd)
{
    pr_debug("%s\n", __func__);
    cmd->priv = 0;
    cmd_state_set_next(cmd, CMD_STATE_IDLE);
}

/*
 * UPG_PROTO_CMD_SEND_FWC_DATA:
 *   -> [CMD HEADER]
 *   -> [DATA PKT]
 *   -> [DATA PKT]
 *   -> ...
 *   <- [RESP HEADER]
 */
static void send_fwc_data_cmd_start(struct upg_cmd *cmd, s32 cmd_data_len)
{
    pr_debug("%s\n", __func__);
    if (cmd->cmd != UPG_PROTO_CMD_SEND_FWC_DATA) {
        pr_debug("cmd is not UPG_PROTO_CMD_SEND_FWC_DATA\n");
        return;
    }
    /* fwc meta data should be sent before SEND_FWC_DATA */
    if (fwc_info_data == NULL) {
        pr_debug("fwc_info_data is NULL\n");
        return;
    }

    cmd_state_init(cmd, CMD_STATE_START);
    cmd->priv = fwc_info_data;
}

static s32 send_fwc_data_cmd_write_input_data(struct upg_cmd *cmd, u8 *buf,
                                              s32 len)
{
    enum upg_dev_type dev_type;
    struct fwc_info *fwc;
    s32 clen = 0, ret = 0;

    pr_debug("%s\n", __func__);
    fwc = (struct fwc_info *)cmd->priv;
    if (fwc == NULL) {
        pr_debug("fwc info is NULL\n");
        return 0;
    }

    if (cmd->state == CMD_STATE_START)
        cmd_state_set_next(cmd, CMD_STATE_DATA_IN);

    if (cmd->state == CMD_STATE_DATA_IN) {
        dev_type = get_current_device_type();
        switch (dev_type) {
            case UPG_DEV_TYPE_RAM:
                ret = ram_fwc_data_write(fwc, buf, len);
                break;
#if defined(AICUPG_MMC_ARTINCHIP)
            case UPG_DEV_TYPE_MMC:
                ret = mmc_fwc_data_write(fwc, buf, len);
                break;
#endif
#if defined(AICUPG_NAND_ARTINCHIP)
            case UPG_DEV_TYPE_RAW_NAND:
            case UPG_DEV_TYPE_SPI_NAND:
                ret = nand_fwc_data_write(fwc, buf, len);
                break;
#endif
#if defined(AICUPG_NOR_ARTINCHIP)
            case UPG_DEV_TYPE_SPI_NOR:
                ret = nor_fwc_data_write(fwc, buf, len);
                break;
#endif
            case UPG_DEV_TYPE_UNKNOWN:
                pr_err("Unknown device.\n");
            default:
                break;
        }

        clen = ret;
        if (fwc->trans_size >= fwc->meta.size) {
            fwc->burn_result = 0;
            fwc->run_result = 0;
            cmd_state_set_next(cmd, CMD_STATE_RESP);
        }
    }

    pr_debug("%s, l: %d\n", __func__, __LINE__);
    return clen;
}

static s32 send_fwc_data_cmd_read_output_data(struct upg_cmd *cmd, u8 *buf,
                                              s32 len)
{
    struct resp_header resp;
    struct fwc_info *fwc;
    u32 siz = 0;

    pr_debug("%s\n", __func__);
    fwc = (struct fwc_info *)cmd->priv;
    if (fwc == NULL) {
        pr_debug("fwc info is NULL\n");
        return 0;
    }

    if (cmd->state == CMD_STATE_RESP) {
        /*
         * Enter read RESP state, to make it simple, HOST should read
         * RESP in one read operation.
         */
        aicupg_gen_resp(&resp, cmd->cmd, UPG_RESP_OK, 0);
        siz = sizeof(struct resp_header);
        memcpy(buf, &resp, siz);
        cmd_state_set_next(cmd, CMD_STATE_END);
    } else if (cmd->state == CMD_STATE_DATA_IN) {
        /* It must be something wrong. */
        aicupg_gen_resp(&resp, cmd->cmd, UPG_RESP_FAIL, 0);
        siz = sizeof(struct resp_header);
        memcpy(buf, &resp, siz);
        cmd_state_set_next(cmd, CMD_STATE_END);
    }
    return siz;
}

static void send_fwc_data_cmd_end(struct upg_cmd *cmd)
{
    enum upg_dev_type dev_type;
    struct fwc_info *fwc;
    u64 total_len = 0;
    u32 start_us;

    fwc = (struct fwc_info *)cmd->priv;

    pr_debug("%s\n", __func__);
    if (cmd->state == CMD_STATE_END) {
        dev_type = get_current_device_type();
        switch (dev_type) {
            case UPG_DEV_TYPE_RAM:
                ram_fwc_data_end(fwc);
                break;
#if defined(AICUPG_MMC_ARTINCHIP)
            case UPG_DEV_TYPE_MMC:
                mmc_fwc_data_end(fwc);
                break;
#endif
#if defined(AICUPG_NAND_ARTINCHIP)
            case UPG_DEV_TYPE_RAW_NAND:
            case UPG_DEV_TYPE_SPI_NAND:
                nand_fwc_data_end(fwc);
                break;
#endif
#if defined(AICUPG_NOR_ARTINCHIP)
            case UPG_DEV_TYPE_SPI_NOR:
                nor_fwc_data_end(fwc);
                break;
#endif
            case UPG_DEV_TYPE_UNKNOWN:
            default:
                break;
        }
        cmd->priv = 0;
        cmd_state_set_next(cmd, CMD_STATE_IDLE);
    }

    total_len = fwc->trans_size;
    start_us = aic_get_time_us() - fwc->start_us;
    pr_info("    Partition: %s programming done.\n", fwc->meta.partition);
    pr_info("    Used time: %u.%u sec, Speed: %lu.%lu KB/s.\n",
            start_us / 1000000, start_us / 1000 % 1000,
            (ulong)((total_len * 1000000) / start_us / 1024),
            (ulong)((total_len * 1000000) / start_us % 1024));

    pr_debug("%s, l: %d\n", __func__, __LINE__);
}

/*
 * UPG_PROTO_CMD_GET_FWC_CRC:
 *   -> [CMD HEADER]
 *   <- [RESP HEADER]
 *   <- [FWC CRC]
 */
static void get_fwc_crc_cmd_start(struct upg_cmd *cmd, s32 cmd_data_len)
{
    pr_debug("%s\n", __func__);
    if (cmd->cmd != UPG_PROTO_CMD_GET_FWC_CRC)
        return;
    /* fwc meta data should be sent before this command */
    if (fwc_info_data == NULL)
        return;
    cmd_state_init(cmd, CMD_STATE_START);
    cmd->priv = fwc_info_data;
}

static s32 get_fwc_crc_cmd_write_input_data(struct upg_cmd *cmd, u8 *buf,
                                            s32 len)
{
    /* No input data for this command */
    return 0;
}

static s32 get_fwc_crc_cmd_read_output_data(struct upg_cmd *cmd, u8 *buf,
                                            s32 len)
{
    struct resp_header resp;
    struct fwc_info *fwc;
    u32 siz = 0, val = 0;

    pr_debug("%s\n", __func__);
    fwc = (struct fwc_info *)cmd->priv;
    if (fwc == NULL)
        return 0;

    if (cmd->state == CMD_STATE_START)
        cmd_state_set_next(cmd, CMD_STATE_RESP);

    if (cmd->state == CMD_STATE_RESP) {
        /*
         * Enter read RESP state, to make it simple, HOST should read
         * RESP in one read operation.
         */
        aicupg_gen_resp(&resp, cmd->cmd, 0, 4);
        siz = sizeof(struct resp_header);
        memcpy(buf, &resp, siz);
        cmd_state_set_next(cmd, CMD_STATE_DATA_OUT);
    }
    if (siz == len)
        return siz;

    if (cmd->state == CMD_STATE_DATA_OUT) {
        val = fwc->calc_partition_crc;
        memcpy(buf, &val, 4);
        siz += 4;
        cmd_state_set_next(cmd, CMD_STATE_END);
    }
    return siz;
}

static void get_fwc_crc_cmd_end(struct upg_cmd *cmd)
{
    pr_debug("%s\n", __func__);
    cmd->priv = 0;
    cmd_state_set_next(cmd, CMD_STATE_IDLE);
}

/*
 * UPG_PROTO_CMD_GET_FWC_BURN_RESULT:
 *   -> [CMD HEADER]
 *   <- [RESP HEADER]
 *   <- [FWC BURN RESULT]
 */
static void get_fwc_burn_result_cmd_start(struct upg_cmd *cmd, s32 cmd_data_len)
{
    pr_debug("%s\n", __func__);
    if (cmd->cmd != UPG_PROTO_CMD_GET_FWC_BURN_RESULT)
        return;
    /* fwc meta data should be sent before this command */
    if (fwc_info_data == NULL)
        return;
    cmd_state_init(cmd, CMD_STATE_START);
    cmd->priv = fwc_info_data;
}

static s32 get_fwc_burn_result_cmd_write_input_data(struct upg_cmd *cmd,
                                                    u8 *buf, s32 len)
{
    /* No input data for this command */
    return 0;
}

static s32 get_fwc_burn_result_cmd_read_output_data(struct upg_cmd *cmd,
                                                    u8 *buf, s32 len)
{
    struct resp_header resp;
    struct fwc_info *fwc;
    u32 siz = 0, val = 0;

    pr_debug("%s\n", __func__);
    fwc = (struct fwc_info *)cmd->priv;
    if (fwc == NULL)
        return 0;

    if (cmd->state == CMD_STATE_START)
        cmd_state_set_next(cmd, CMD_STATE_RESP);

    if (cmd->state == CMD_STATE_RESP) {
        /*
         * Enter read RESP state, to make it simple, HOST should read
         * RESP in one read operation.
         */
        aicupg_gen_resp(&resp, cmd->cmd, 0, 4);
        siz = sizeof(struct resp_header);
        memcpy(buf, &resp, siz);
        cmd_state_set_next(cmd, CMD_STATE_DATA_OUT);
    }
    if (siz == len)
        return siz;

    if (cmd->state == CMD_STATE_DATA_OUT) {
        val = fwc->burn_result ? 1 : 0;
        memcpy(buf, &val, 4);
        siz += 4;
        cmd_state_set_next(cmd, CMD_STATE_END);
    }
    return siz;
}

static void get_fwc_burn_result_cmd_end(struct upg_cmd *cmd)
{
    pr_debug("%s\n", __func__);
    cmd->priv = 0;
    cmd_state_set_next(cmd, CMD_STATE_IDLE);
}

/*
 * UPG_PROTO_CMD_GET_FWC_RUN_RESULT:
 *   -> [CMD HEADER]
 *   <- [RESP HEADER]
 *   <- [FWC RUN RESULT]
 */
static void get_fwc_run_result_cmd_start(struct upg_cmd *cmd, s32 cmd_data_len)
{
    pr_debug("%s\n", __func__);
    if (cmd->cmd != UPG_PROTO_CMD_GET_FWC_RUN_RESULT)
        return;
    /* fwc meta data should be sent before GET_FWC_CRC */
    if (fwc_info_data == NULL)
        return;
    cmd_state_init(cmd, CMD_STATE_START);
    cmd->priv = fwc_info_data;
}

static s32 get_fwc_run_result_cmd_write_input_data(struct upg_cmd *cmd, u8 *buf,
                                                   s32 len)
{
    /* No input data for this command */
    return 0;
}

static s32 get_fwc_run_result_cmd_read_output_data(struct upg_cmd *cmd, u8 *buf,
                                                   s32 len)
{
    struct resp_header resp;
    struct fwc_info *fwc;
    u32 siz = 0, val = 0;

    pr_debug("%s\n", __func__);
    fwc = (struct fwc_info *)cmd->priv;
    if (fwc == NULL)
        return 0;

    if (cmd->state == CMD_STATE_START)
        cmd_state_set_next(cmd, CMD_STATE_RESP);

    if (cmd->state == CMD_STATE_RESP) {
        /*
         * Enter read RESP state, to make it simple, HOST should read
         * RESP in one read operation.
         */
        aicupg_gen_resp(&resp, cmd->cmd, 0, 4);
        siz = sizeof(struct resp_header);
        memcpy(buf, &resp, siz);
        cmd_state_set_next(cmd, CMD_STATE_DATA_OUT);
    }
    if (siz == len)
        return siz;

    if (cmd->state == CMD_STATE_DATA_OUT) {
        val = fwc->run_result ? 1 : 0;
        memcpy(buf, &val, 4);
        siz += 4;
        cmd_state_set_next(cmd, CMD_STATE_END);
    }
    return siz;
}

static void get_fwc_run_result_cmd_end(struct upg_cmd *cmd)
{
    pr_debug("%s\n", __func__);
    cmd->priv = 0;
    cmd_state_set_next(cmd, CMD_STATE_IDLE);
}

/*
 * UPG_PROTO_CMD_GET_STORAGE_MEDIA:
 *   -> [CMD HEADER]
 *   <- [RESP HEADER]
 *   <- [MEDIA LIST]
 */
static void get_storage_media_cmd_start(struct upg_cmd *cmd, s32 cmd_data_len)
{
    pr_debug("%s\n", __func__);
    if (cmd->cmd != UPG_PROTO_CMD_GET_STORAGE_MEDIA)
        return;
    cmd_state_init(cmd, CMD_STATE_START);
}

static s32 get_storage_media_cmd_write_input_data(struct upg_cmd *cmd, u8 *buf,
                                                  s32 len)
{
    /* No input data for this command */
    return 0;
}

static s32 get_storage_media_cmd_read_output_data(struct upg_cmd *cmd, u8 *buf,
                                                  s32 len)
{
    struct resp_header resp;
    struct storage_media medias[3] = { 0 };
    u32 siz = 0, i = 0;

    pr_debug("%s\n", __func__);
#if defined(AICUPG_MMC_ARTINCHIP)
    if (mmc_is_exist()) {
        medias[i].media_dev_id = 0;
        strcpy(medias[i++].media_type, "mmc");
    }
#endif
#if defined(AICUPG_NAND_ARTINCHIP)
    if (nand_is_exist()) {
        medias[i].media_dev_id = 0;
        strcpy(medias[i++].media_type, "spi-nand");
    }
#endif
#if defined(AICUPG_NOR_ARTINCHIP)
    if (nor_is_exist()) {
        medias[i].media_dev_id = 0;
        strcpy(medias[i++].media_type, "spi-nor");
    }
#endif

    if (cmd->state == CMD_STATE_START)
        cmd_state_set_next(cmd, CMD_STATE_RESP);

    if (cmd->state == CMD_STATE_RESP) {
        /*
         * Enter read RESP state, to make it simple, HOST should read
         * RESP in one read operation.
         */
        siz = sizeof(struct storage_media) * i;
        aicupg_gen_resp(&resp, cmd->cmd, 0, siz);
        siz = sizeof(struct resp_header);
        memcpy(buf, &resp, siz);
        cmd_state_set_next(cmd, CMD_STATE_DATA_OUT);
    }
    if (siz == len)
        return siz;

    if (cmd->state == CMD_STATE_DATA_OUT) {
        memcpy(buf, &medias, sizeof(struct storage_media) * i);
        siz += sizeof(struct storage_media) * i;
        cmd_state_set_next(cmd, CMD_STATE_END);
    }
    return siz;
}

static void get_storage_media_cmd_end(struct upg_cmd *cmd)
{
    pr_debug("%s\n", __func__);
    cmd->priv = 0;
    cmd_state_set_next(cmd, CMD_STATE_IDLE);
}
/*
 * UPG_PROTO_CMD_GET_PARTITION_TABLE:
 *   -> [CMD HEADER]
 *   <- [RESP HEADER]
 *   <- [PARTITION TABLE]
 */
static void get_partition_table_cmd_start(struct upg_cmd *cmd, s32 cmd_data_len)
{
    static struct storage_media media = { 0 };

    pr_debug("%s\n", __func__);
    if (cmd->cmd != UPG_PROTO_CMD_GET_PARTITION_TABLE)
        return;

    cmd->priv = &media;
    cmd_state_init(cmd, CMD_STATE_START);
}

static s32 get_partition_table_cmd_write_input_data(struct upg_cmd *cmd,
                                                    u8 *buf, s32 len)
{
    struct storage_media *priv;
    u32 clen = 0, siz = 0;

    priv = (struct storage_media *)cmd->priv;
    if (priv == NULL)
        return 0;

    if (cmd->state == CMD_STATE_START)
        cmd_state_set_next(cmd, CMD_STATE_ARG);
    if (cmd->state == CMD_STATE_ARG) {
        siz = sizeof(struct storage_media);
        if (len < siz)
            return 0;
        memcpy(priv, buf, siz);
        clen += siz;
        cmd_state_set_next(cmd, CMD_STATE_RESP);
    }

    return clen;
}

static s32 get_partition_table_cmd_read_output_data(struct upg_cmd *cmd,
                                                    u8 *buf, s32 len)
{
    struct resp_header resp;
    struct storage_media *priv;
    char *part_table = "";
    u32 siz = 0;

    pr_debug("%s\n", __func__);
    priv = (struct storage_media *)cmd->priv;
    if (priv == NULL)
        return 0;

    if (cmd->state == CMD_STATE_RESP) {
        /*
         * Enter read RESP state, to make it simple, HOST should read
         * RESP in one read operation.
         */
        aicupg_gen_resp(&resp, cmd->cmd, 0, PARTITION_TABLE_LEN);
        siz = sizeof(struct resp_header);
        memcpy(buf, &resp, siz);
        cmd_state_set_next(cmd, CMD_STATE_DATA_OUT);
    }
    if (siz == len)
        return siz;

    printf("media_type:%s\n", priv->media_type);
    if (cmd->state == CMD_STATE_DATA_OUT) {
        if (priv->media_type == NULL)
            return 0;
#if defined(AICUPG_MMC_ARTINCHIP)
        else if (!memcmp(priv->media_type, "mmc", 3))
            part_table = GPT_PART_TABLE;
#endif
#if defined(AICUPG_NAND_ARTINCHIP)
        else if (!memcmp(priv->media_type, "spi-nand", 8))
            part_table = IMAGE_CFG_JSON_PARTS_MTD;
#endif
#if defined(AICUPG_NOR_ARTINCHIP)
        else if (!memcmp(priv->media_type, "spi-nor", 7))
            part_table = IMAGE_CFG_JSON_PARTS_MTD;
#endif
        else
            return 0;
        printf("part table:%s\n", part_table);
        memcpy(buf, part_table, PARTITION_TABLE_LEN);
        siz += PARTITION_TABLE_LEN;
        cmd_state_set_next(cmd, CMD_STATE_END);
    }

    return siz;
}

static void get_partition_table_cmd_end(struct upg_cmd *cmd)
{
    pr_debug("%s\n", __func__);
    cmd->priv = 0;
    cmd_state_set_next(cmd, CMD_STATE_IDLE);
}

/*
 * UPG_PROTO_CMD_READ_FWC_DATA:
 *   -> [CMD HEADER]
 *   <- [RESP HEADER]
 *   <- [FWC PARTITION DATA]
 */
static void read_fwc_data_cmd_start(struct upg_cmd *cmd, s32 cmd_data_len)
{
    pr_debug("%s\n", __func__);
    if (cmd->cmd != UPG_PROTO_CMD_READ_FWC_DATA)
        return;

    if (fwc_info_data == NULL) {
        fwc_info_data = malloc(sizeof(struct fwc_info));
        if (fwc_info_data == NULL) {
            pr_debug("%s fwc_info_data = NULL\n", __func__);
            return;
        }
    }

    memset(fwc_info_data, 0, sizeof(struct fwc_info));
    cmd->priv = fwc_info_data;
    cmd_state_init(cmd, CMD_STATE_START);
}

static s32 read_fwc_data_cmd_write_input_data(struct upg_cmd *cmd, u8 *buf,
                                              s32 len)
{
    struct fwc_info *fwc;
    s32 clen = 0, siz = 0;

    pr_debug("%s\n", __func__);
    fwc = (struct fwc_info *)cmd->priv;
    if (fwc == NULL)
        return 0;

    if (cmd->state == CMD_STATE_START)
        cmd_state_set_next(cmd, CMD_STATE_ARG);

    if (cmd->state == CMD_STATE_ARG) {
        siz = sizeof(struct media_partition);
        if (len < siz)
            return 0;
        memcpy(&(fwc->mpart), buf, siz);
        clen += siz;
        cmd_state_set_next(cmd, CMD_STATE_RESP);

        strcpy(fwc->meta.partition, fwc->mpart.part.name);
        strcpy(fwc->meta.attr, fwc->mpart.attr);
        fwc->meta.offset = fwc->mpart.part.start;
        fwc->meta.size = fwc->mpart.part.size;
    }

    return clen;
}

static s32 read_fwc_data_cmd_read_output_data(struct upg_cmd *cmd, u8 *buf,
                                              s32 len)
{
    enum upg_dev_type dev_type;
    struct resp_header resp;
    struct fwc_info *fwc;
    u32 siz = 0, ret = 0, clen = 0;

    pr_debug("%s\n", __func__);
    fwc = (struct fwc_info *)cmd->priv;
    if (fwc == NULL)
        return 0;

    if (cmd->state == CMD_STATE_RESP) {
        /*
         * Enter read RESP state, to make it simple, HOST should read
         * RESP in one read operation.
         */
        siz = fwc->mpart.part.size;
        aicupg_gen_resp(&resp, cmd->cmd, 0, siz);
        siz = sizeof(struct resp_header);
        memcpy(buf, &resp, siz);
        cmd_state_set_next(cmd, CMD_STATE_DATA_OUT);

        dev_type = get_media_type(fwc);
        set_current_device_type(dev_type);
        pr_info("    Media: %s(%d)\n", get_current_device_name(dev_type),
                dev_type);

        switch (dev_type) {
#if defined(AICUPG_MMC_ARTINCHIP)
            case UPG_DEV_TYPE_MMC:
                ret = mmc_fwc_prepare(fwc, 0);
                if (ret < 0)
                    pr_err("NAND prepare failed\n");
                mmc_fwc_start(fwc);
                break;
#endif
#if defined(AICUPG_NAND_ARTINCHIP)
            case UPG_DEV_TYPE_RAW_NAND:
            case UPG_DEV_TYPE_SPI_NAND:
                ret = nand_fwc_prepare(fwc, 0);
                if (ret < 0)
                    pr_err("NAND prepare failed\n");
                nand_fwc_start(fwc);
                break;
#endif
#if defined(AICUPG_NOR_ARTINCHIP)
            case UPG_DEV_TYPE_SPI_NOR:
                ret = nor_fwc_prepare(fwc, 0);
                if (ret < 0)
                    pr_err("NOR prepare failed\n");
                nor_fwc_start(fwc);
                break;
#endif
            case UPG_DEV_TYPE_UNKNOWN:
            default:
                break;
        }
    }

    if (siz == len)
        return siz;

    if (cmd->state == CMD_STATE_DATA_OUT) {
        dev_type = get_current_device_type();
        switch (dev_type) {
#if defined(AICUPG_MMC_ARTINCHIP)
            case UPG_DEV_TYPE_MMC:
                ret = mmc_fwc_data_read(fwc, buf, len);
                break;
#endif
#if defined(AICUPG_NAND_ARTINCHIP)
            case UPG_DEV_TYPE_RAW_NAND:
            case UPG_DEV_TYPE_SPI_NAND:
                ret = nand_fwc_data_read(fwc, buf, len);
                break;
#endif
#if defined(AICUPG_NOR_ARTINCHIP)
            case UPG_DEV_TYPE_SPI_NOR:
                ret = nor_fwc_data_read(fwc, buf, len);
                break;
#endif
            case UPG_DEV_TYPE_UNKNOWN:
                pr_err("Unknown device.\n");
            default:
                break;
        }
        clen = ret;
        if (fwc->trans_size >= fwc->mpart.part.size)
            cmd_state_set_next(cmd, CMD_STATE_END);
    }

    return clen;
}

static void read_fwc_data_cmd_end(struct upg_cmd *cmd)
{
    enum upg_dev_type dev_type;
    struct fwc_info *fwc;

    fwc = (struct fwc_info *)cmd->priv;
    (void)fwc;

    pr_debug("%s\n", __func__);
    if (cmd->state == CMD_STATE_END) {
        dev_type = get_current_device_type();
        switch (dev_type) {
            case UPG_DEV_TYPE_RAM:
                break;
#if defined(AICUPG_MMC_ARTINCHIP)
            case UPG_DEV_TYPE_MMC:
                mmc_fwc_data_end(fwc);
                break;
#endif
#if defined(AICUPG_NAND_ARTINCHIP)
            case UPG_DEV_TYPE_RAW_NAND:
            case UPG_DEV_TYPE_SPI_NAND:
                nand_fwc_data_end(fwc);
                break;
#endif
#if defined(AICUPG_NOR_ARTINCHIP)
            case UPG_DEV_TYPE_SPI_NOR:
                nor_fwc_data_end(fwc);
                break;
#endif
            case UPG_DEV_TYPE_UNKNOWN:
            default:
                break;
        }
        cmd->priv = 0;
        cmd_state_set_next(cmd, CMD_STATE_IDLE);
    }
    pr_debug("%s, l: %d\n", __func__, __LINE__);
}

/*
 * UPG_PROTO_CMD_SET_UART_ARGS:
 *   -> [CMD HEADER]
 *   <- [RESP HEADER]
 *   <- [FWC PARTITION DATA]
 */
static void set_uart_args_cmd_start(struct upg_cmd *cmd, s32 cmd_data_len)
{
    pr_debug("%s data len %d\n", __func__, cmd_data_len);
    if (cmd->cmd != UPG_PROTO_CMD_SET_UART_ARGS)
        return;

    cmd->priv = 0;
    cmd_state_init(cmd, CMD_STATE_START);
}

static s32 set_uart_args_cmd_write_input_data(struct upg_cmd *cmd, u8 *buf, s32 len)
{
    s32 clen = 0;
    u32 baudrate = 0;

    pr_debug("%s\n", __func__);
    if (cmd->state == CMD_STATE_START)
        cmd_state_set_next(cmd, CMD_STATE_ARG);

    if (cmd->state == CMD_STATE_ARG) {
        /*
         * Enter recv argument state
         */
        if (len < 4)
            return 0;
        memcpy(&baudrate, buf, 4);
        clen = 4;
        cmd_state_set_next(cmd, CMD_STATE_RESP);
    }
    cmd->priv = (void *)(uintptr_t)baudrate;

    return clen;
}

static s32 set_uart_args_cmd_read_output_data(struct upg_cmd *cmd, u8 *buf, s32 len)
{
    struct resp_header resp;
    s32 siz = 0;

    if (cmd->state == CMD_STATE_RESP) {
        /*
         * Enter read RESP state, to make it simple, HOST should read
         * RESP in one read operation.
         */
        aicupg_gen_resp(&resp, cmd->cmd, (unsigned long)UPG_RESP_OK, 0);
        siz = sizeof(struct resp_header);
        memcpy(buf, &resp, siz);
        cmd_state_set_next(cmd, CMD_STATE_END);
    }

    return siz;
}

extern void aic_upg_uart_baudrate_update(int baudrate);
static void set_uart_args_cmd_end(struct upg_cmd *cmd)
{
    pr_debug("%s\n", __func__);
    if (cmd->state == CMD_STATE_END) {
#ifdef AICUPG_UART_ENABLE
        int baudrate = (uintptr_t)cmd->priv;
        aic_upg_uart_baudrate_update(baudrate);
#endif
        cmd->priv = 0;
        cmd_state_set_next(cmd, CMD_STATE_IDLE);
    }
}

static struct upg_cmd fwc_cmd_list[] = {
    {
        UPG_PROTO_CMD_SET_FWC_META,
        set_fwc_meta_cmdstart,
        set_fwc_meta_cmdwrite_input_data,
        set_fwc_meta_cmdread_output_data,
        set_fwc_meta_cmdend,
    },
    {
        UPG_PROTO_CMD_GET_BLOCK_SIZE,
        get_block_size_cmd_start,
        get_block_size_cmd_write_input_data,
        get_block_size_cmd_read_output_data,
        get_block_size_cmd_end,
    },
    {
        UPG_PROTO_CMD_SEND_FWC_DATA,
        send_fwc_data_cmd_start,
        send_fwc_data_cmd_write_input_data,
        send_fwc_data_cmd_read_output_data,
        send_fwc_data_cmd_end,
    },
    {
        UPG_PROTO_CMD_GET_FWC_CRC,
        get_fwc_crc_cmd_start,
        get_fwc_crc_cmd_write_input_data,
        get_fwc_crc_cmd_read_output_data,
        get_fwc_crc_cmd_end,
    },
    {
        UPG_PROTO_CMD_GET_FWC_BURN_RESULT,
        get_fwc_burn_result_cmd_start,
        get_fwc_burn_result_cmd_write_input_data,
        get_fwc_burn_result_cmd_read_output_data,
        get_fwc_burn_result_cmd_end,
    },
    {
        UPG_PROTO_CMD_GET_FWC_RUN_RESULT,
        get_fwc_run_result_cmd_start,
        get_fwc_run_result_cmd_write_input_data,
        get_fwc_run_result_cmd_read_output_data,
        get_fwc_run_result_cmd_end,
    },
    {
        UPG_PROTO_CMD_GET_STORAGE_MEDIA,
        get_storage_media_cmd_start,
        get_storage_media_cmd_write_input_data,
        get_storage_media_cmd_read_output_data,
        get_storage_media_cmd_end,
    },
    {
        UPG_PROTO_CMD_GET_PARTITION_TABLE,
        get_partition_table_cmd_start,
        get_partition_table_cmd_write_input_data,
        get_partition_table_cmd_read_output_data,
        get_partition_table_cmd_end,
    },
    {
        UPG_PROTO_CMD_READ_FWC_DATA,
        read_fwc_data_cmd_start,
        read_fwc_data_cmd_write_input_data,
        read_fwc_data_cmd_read_output_data,
        read_fwc_data_cmd_end,
    },
    {
        UPG_PROTO_CMD_SET_UART_ARGS,
        set_uart_args_cmd_start,
        set_uart_args_cmd_write_input_data,
        set_uart_args_cmd_read_output_data,
        set_uart_args_cmd_end,
    },
};

struct upg_cmd *find_fwc_command(struct cmd_header *h)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(fwc_cmd_list); i++) {
        if (fwc_cmd_list[i].cmd == (u32)h->command)
            return &fwc_cmd_list[i];
    }

    return NULL;
}