/* Copyright Statement:
 *
 * (C) 2005-2016  MediaTek Inc. All rights reserved.
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. ("MediaTek") and/or its licensors.
 * Without the prior written permission of MediaTek and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 * You may only use, reproduce, modify, or distribute (as applicable) MediaTek Software
 * if you have agreed to and been bound by the applicable license agreement with
 * MediaTek ("License Agreement") and been granted explicit permission to do so within
 * the License Agreement ("Permitted User").  If you are not a Permitted User,
 * please cease any access or use of MediaTek Software immediately.
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT MEDIATEK SOFTWARE RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES
 * ARE PROVIDED TO RECEIVER ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 */


#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>

#include <hal_flash.h>
#ifndef MTK_BOOTLOADER_SUPPORT_PARTITION_FOTA
#include <hal_sha.h>
#else /* MTK_BOOTLOADER_SUPPORT_PARTITION_FOTA */

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#if defined(MBEDTLS_PLATFORM_C)
#include "mbedtls/platform.h"
#else
#include <stdio.h>
#include <stdlib.h>
#endif

#if !defined(MBEDTLS_SHA1_C)
#error "MBEDTLS_SHA1_C is not enabled"
#else
#include "mbedtls/sha1.h"
#endif
#endif /* MTK_BOOTLOADER_SUPPORT_PARTITION_FOTA */
#include "hal_wdt.h"
#include "hal_lzma_decode_interface.h"

#include "fota_platform.h"
#include "fota_config.h"
#include "fota_internal.h"
#include "fota_port.h"


#define FOTA_HEADER_MAGIC                   0x004D4D4D
#define FOTA_HEADER_MAGIC_END_MARK          0x45454545
#define FOTA_HEADER_GET_MAGIC(magic_ver)    ((magic_ver)&0x00FFFFFF)
#define FOTA_HEADER_GET_VER(magic_ver)      ((magic_ver)>>24)

#define FOTA_SIGNATURE_SIZE                 (20)    /* SHA1 */
#define FOTA_UPDATE_INFO_RESERVE_SIZE           (512)
#define FOTA_UPDATING_MARKER                    (0x544e5546)

#define FOTA_BIN_NUMBER_MAX                     4


typedef struct {
    int32_t m_magic_ver;
    int32_t m_fota_triggered;
} fota_trigger_info_t;

typedef struct {
    int32_t m_ver;
    int32_t m_error_code;
    int32_t m_behavior;
    int32_t m_is_read;
    char  m_marker[32];
    int32_t reserved[4];
} fota_update_info_t;

typedef struct {
    uint32_t m_bin_offset;
    uint32_t m_bin_start_addr;
    uint32_t m_bin_length;
    uint32_t m_partition_length;
    uint32_t m_sig_offset;
    uint32_t m_sig_length;
    uint32_t m_is_compressed;
    uint8_t m_bin_reserved[4];
} fota_bin_info_t;

typedef struct {
    uint32_t m_magic_ver;
    uint32_t m_bin_num;
    fota_bin_info_t m_bin_info[FOTA_BIN_NUMBER_MAX];
} fota_header_info_t;


static fota_flash_t *flash_s;


/**
 * Check whether <i>number</i> is power-of-2.
 *
 * Examples: 1, 2, 4, 8, ..., and so on.
 *
 * @retval true if <i>number</i> is power-of-2.
 * @retval false if <i>number</i> is not power-of-2.
 */
static bool _fota_is_power_of_2(uint32_t number)
{
    number &= number - 1;
    return (number == 0);
}


static fota_partition_t *_fota_find_partition(uint32_t partition)
{
    size_t  i;

    for (i = 0; i < flash_s->table_entries; i++) {
        if (flash_s->table[i].id == partition) {
            return &flash_s->table[i];
        }
    }

    return NULL;
}


fota_status_t fota_read_update_status(uint32_t partition,
                                            int32_t *status)
{
    fota_update_info_t fota_update_info = {0};
    fota_status_t ret = FOTA_STATUS_OK;
    fota_partition_t *p;

    if (!status)
    {
        return FOTA_STATUS_ERROR_INVALD_PARAMETER;
    }

    /* get fota partition ptr */
    if ((p = _fota_find_partition(partition)) == NULL) {
        FOTA_LOG_MSGID_E("fota_read_update_status() ret:%d line:%d",2, FOTA_STATUS_ERROR_UNKNOWN_ID, __LINE__);
        return FOTA_STATUS_ERROR_UNKNOWN_ID;
    }

    /* seek and read status */
    ret = fota_seek( partition, (p->length - FOTA_UPDATE_INFO_RESERVE_SIZE) );    
    if (ret != FOTA_STATUS_OK) {
        FOTA_LOG_MSGID_E("fota_read_update_status() ret:%d line:%d",2, ret, __LINE__);
        return ret;
    }

    ret = fota_read(partition, (uint8_t *)&fota_update_info, sizeof(fota_update_info_t));
    if (ret != FOTA_STATUS_OK) {
        FOTA_LOG_MSGID_E("fota_read_update_status() ret:%d line:%d",2, ret, __LINE__);
        return FOTA_STATUS_ERROR_FLASH_OP;
    }

    *status = fota_update_info.m_error_code;
    return FOTA_STATUS_OK;
}


static fota_status_t _fota_erase_final_block(uint32_t partition)
{
    fota_partition_t *p;
    uint32_t addr;

    FOTA_LOG_MSGID_I("_fota_erase_final_block() partition:%d",1, partition);
    p = _fota_find_partition(partition);
    /* erase final block (4K) */
    addr = p->address + p->length - 0x1000;
    if ((addr % flash_s->block_size) == 0) {
        fota_port_isr_disable();
        if (hal_flash_erase(addr, HAL_FLASH_BLOCK_4K) < 0) {
            fota_port_isr_enable();
            FOTA_LOG_MSGID_E("_fota_erase_final_block() ret:%d line:%d",2, FOTA_STATUS_ERROR_FLASH_OP, __LINE__);
            return FOTA_STATUS_ERROR_FLASH_OP;
        }

        fota_port_isr_enable();
        return FOTA_STATUS_OK;
    }
    
    FOTA_LOG_MSGID_E("_fota_erase_final_block() ret:%d line:%d",2, FOTA_STATUS_ERROR_FLASH_OP, __LINE__);
    return FOTA_STATUS_ERROR_FLASH_OP;
}


static fota_status_t _fota_write_update_status(uint32_t partition, int32_t error_code)
{
    fota_status_t status = FOTA_STATUS_OK;
    fota_update_info_t iot_fota_update_info;
    void *iot_fota_update_info_buf;
    fota_partition_t    *p;

    FOTA_LOG_MSGID_I("_fota_write_update_status() partition:%d error_code:%d",2, partition, error_code);
    /* erase final block in TMP partition */
    _fota_erase_final_block(partition);

    /* set update status in final block */
    iot_fota_update_info_buf = &iot_fota_update_info;
    iot_fota_update_info.m_ver = 0;
    iot_fota_update_info.m_error_code = error_code;
    iot_fota_update_info.m_behavior = 0;
    iot_fota_update_info.m_is_read = 0;

    p = _fota_find_partition(partition);
    status = fota_seek( partition, (p->length - FOTA_UPDATE_INFO_RESERVE_SIZE) );
    if ( status != FOTA_STATUS_OK) {
        FOTA_LOG_MSGID_E("_fota_write_update_status() status:%d line:%d",2, status, __LINE__);
        return status;
    }

    status = fota_write( partition, (uint8_t *)iot_fota_update_info_buf, sizeof(fota_update_info_t));
    if ( status != FOTA_STATUS_OK) {
        FOTA_LOG_MSGID_E("_fota_write_update_status() status:%d line:%d",2, status, __LINE__);
        return status;
    }

    FOTA_LOG_MSGID_I("_fota_write_update_status() status:%d",1, status);
    return status;
}


#if defined(MTK_SEC_FLASH_SCRAMBLE_ENABLE)
static bool fota_scramble_flag = true;
#endif
#if defined(MOD_CFG_FOTA_BL_RESERVED)
static fota_header_info_t fota_head;

/* LZMA porting ( decompress a bin always use 4 buffer ) */
static int lzma_alloc_count = 0;
#if defined(MTK_FOTA_ON_7686) && defined(__ICCARM__)
ATTR_ZIDATA_IN_TCM static uint8_t lzma_buf_0[16384]; /* 15980 */
ATTR_ZIDATA_IN_TCM static uint8_t lzma_buf_1[16384]; /* 16384 */
ATTR_ZIDATA_IN_TCM static uint8_t lzma_buf_2[4096];  /* 4096 */
ATTR_ZIDATA_IN_TCM static uint8_t lzma_buf_3[4096];  /* 4096 */
#else
static uint8_t lzma_buf_0[16384]; /* 15980 */
static uint8_t lzma_buf_1[16384]; /* 16384 */
static uint8_t lzma_buf_2[4096];  /* 4096 */
static uint8_t lzma_buf_3[4096];  /* 4096 */
#endif
void *_bl_alloc(void *p, size_t size)
{
    FOTA_LOG_MSGID_I("_bl_alloc size = %d",1, size);
    FOTA_LOG_MSGID_I("_bl_alloc lzma_alloc_count = %d",1, lzma_alloc_count);
    switch (lzma_alloc_count) {
        case 0:
            lzma_alloc_count++;
            return &lzma_buf_0;
        case 1:
            lzma_alloc_count++;
            return &lzma_buf_1;
        case 2:
            lzma_alloc_count++;
            return &lzma_buf_2;
        case 3:
            lzma_alloc_count = 0;
            return &lzma_buf_3;
        default:
            FOTA_LOG_MSGID_E("_bl_alloc() fail", 0);
            return NULL;
    }
}
void _bl_free(void *p, void *address)
{
    FOTA_LOG_MSGID_I("_bl_free", 0);
}
lzma_alloc_t lzma_alloc = { _bl_alloc, _bl_free };


static fota_status_t _fota_check_updating_marker(uint32_t partition)
{
    fota_update_info_t fota_update_info;
    void *fota_update_info_buf;
    fota_status_t status = FOTA_STATUS_OK;
    int marker_found = 1;
    fota_partition_t *p;
    int i;

    fota_update_info_buf = &fota_update_info;
    marker_found = 1;

    /* get fota partition ptr */
    if ((p = _fota_find_partition(partition)) == NULL) {
        FOTA_LOG_MSGID_E("_fota_check_updating_marker() ret:%d line:%d",2, FOTA_STATUS_ERROR_UNKNOWN_ID, __LINE__);
        return FOTA_STATUS_ERROR_UNKNOWN_ID;
    }

    /* seek and read marker */
    status = fota_seek( partition, (p->length - FOTA_UPDATE_INFO_RESERVE_SIZE) );
    if (status != FOTA_STATUS_OK) {
        FOTA_LOG_MSGID_E("_fota_check_updating_marker() ret:%d line:%d",2, status, __LINE__);
        return status;
    }

    status = fota_read(partition, (uint8_t *)fota_update_info_buf, sizeof(fota_update_info_t));
    if (status != FOTA_STATUS_OK) {
        FOTA_LOG_MSGID_E("_fota_check_updating_marker() ret:%d line:%d",2, status, __LINE__);
        return FOTA_STATUS_ERROR_FLASH_OP;
    }

    /* check marker */
    for (i = 0; i < sizeof(fota_update_info.m_marker) / 4; i++) {
        if (*((uint32_t *)(fota_update_info.m_marker) + i) != FOTA_UPDATING_MARKER) {
            marker_found = 0;
        }
    }
    if (marker_found == 1) {
        FOTA_LOG_MSGID_I("_fota_check_updating_marker() is_set", 0);
        return FOTA_STATUS_IS_FULL;
    }

    FOTA_LOG_MSGID_I("_fota_check_updating_marker() is_empty", 0);
    return FOTA_STATUS_IS_EMPTY;
}


static fota_status_t _fota_check_fota_triggered(uint32_t partition)
{
    fota_status_t status = FOTA_STATUS_OK;
    fota_trigger_info_t fota_triiger_info;
    void *fota_triiger_info_buf;
    fota_partition_t *p;

    fota_triiger_info_buf = &fota_triiger_info;

    /* get fota partition ptr */
    if ((p = _fota_find_partition(partition)) == NULL) {
        FOTA_LOG_MSGID_E("_fota_check_fota_triggered() ret:%d line:%d",2, FOTA_STATUS_ERROR_UNKNOWN_ID, __LINE__);
        return FOTA_STATUS_ERROR_UNKNOWN_ID;
    }

    /* seek and read fota triggered */
    status = fota_seek( partition, (p->length - FOTA_UPDATE_INFO_RESERVE_SIZE) );
    if (status != FOTA_STATUS_OK) {
        FOTA_LOG_MSGID_E("_fota_check_fota_triggered() ret:%d line:%d",2, status, __LINE__);
        return status;
    }

    status = fota_read(partition, (uint8_t *)fota_triiger_info_buf, sizeof(fota_trigger_info_t));
    if (status != FOTA_STATUS_OK) {
        FOTA_LOG_MSGID_E("_fota_check_fota_triggered() ret:%d line:%d",2, status, __LINE__);
        return FOTA_STATUS_ERROR_FLASH_OP;
    }

    if ( FOTA_HEADER_GET_MAGIC(fota_triiger_info.m_magic_ver) != FOTA_HEADER_MAGIC ) {
        FOTA_LOG_MSGID_I("_fota_check_fota_triggered() is_empty", 0);
        return FOTA_STATUS_IS_EMPTY;
    }

    FOTA_LOG_MSGID_I("_fota_check_fota_triggered() is_set", 0);
    return FOTA_STATUS_IS_FULL;
}


static fota_status_t _fota_write_marker(uint32_t partition)
{
    fota_status_t status = FOTA_STATUS_OK;
    fota_partition_t *p;
    fota_update_info_t iot_fota_update_info;
    void *iot_fota_update_info_buf;
    int i;

    FOTA_LOG_MSGID_I("_fota_write_marker() partition:%d",1, partition);
    /* erase final block in TMP partition */
    _fota_erase_final_block(partition);

    /* write marker*/
    iot_fota_update_info_buf = &iot_fota_update_info;
    memset(&iot_fota_update_info, 0x0, sizeof(fota_update_info_t));
    for (i = 0; i < sizeof(iot_fota_update_info.m_marker) / 4; i++) {
        *((uint32_t *)(iot_fota_update_info.m_marker) + i) = FOTA_UPDATING_MARKER;
    }
    p = _fota_find_partition(partition);
    status = fota_seek( partition, (p->length - FOTA_UPDATE_INFO_RESERVE_SIZE) );
    if ( status != FOTA_STATUS_OK) {
        FOTA_LOG_MSGID_E("_fota_write_marker() status:%d line:%d",2, status, __LINE__);
        return status;
    }
    
    status = fota_write( partition, (uint8_t *)iot_fota_update_info_buf, sizeof(fota_update_info_t));
    if ( status != FOTA_STATUS_OK) {
        FOTA_LOG_MSGID_E("_fota_write_marker() status:%d line:%d",2, status, __LINE__);
    }

    return status;
}

static fota_status_t _fota_parse_header(uint32_t partition)
{
    fota_status_t  fota_status = FOTA_STATUS_OK;
    void *fota_head_buf;
    int i;
#ifndef MTK_BOOTLOADER_SUPPORT_PARTITION_FOTA
    hal_sha_status_t sha_status;
    hal_sha1_context_t sha1_context;
#else
    mbedtls_sha1_context ctx;
#endif
    uint8_t header_sha1_checksum[64] = {0};
    uint8_t header_checksum[FOTA_SIGNATURE_SIZE];

    FOTA_LOG_MSGID_I("_fota_parse_header() partition:%d",1, partition);
    fota_head_buf = &fota_head;
    fota_status = fota_read(partition, (uint8_t *)fota_head_buf, sizeof(fota_header_info_t));
    if (fota_status == FOTA_STATUS_OK) {
        /* calculate header checksum */
        FOTA_LOG_MSGID_I("header sha1 init", 0);
#ifndef MTK_BOOTLOADER_SUPPORT_PARTITION_FOTA
        sha_status = hal_sha1_init(&sha1_context);
        if (sha_status != HAL_SHA_STATUS_OK) {
            fota_status = FOTA_STATUS_ERROR_INVALD_PARAMETER;
            FOTA_LOG_MSGID_E("_fota_parse_header() sha_status:%d ret:%d line:%d",3, sha_status, fota_status, __LINE__);
            return fota_status;
        }
        sha_status = hal_sha1_append(&sha1_context, fota_head_buf, sizeof(fota_header_info_t));
        if (sha_status != HAL_SHA_STATUS_OK) {
            fota_status = FOTA_STATUS_ERROR_INVALD_PARAMETER;
            FOTA_LOG_MSGID_E("_fota_parse_header() sha_status:%d ret:%d line:%d",3, sha_status, fota_status, __LINE__);
        }
        FOTA_LOG_MSGID_I("header end", 0);
        sha_status = hal_sha1_end(&sha1_context, header_sha1_checksum);
        if ( sha_status != HAL_SHA_STATUS_OK) {
            fota_status = FOTA_STATUS_ERROR_INVALD_PARAMETER;
            FOTA_LOG_MSGID_E("_fota_parse_header() sha_status:%d ret:%d line:%d",3, sha_status, fota_status, __LINE__);
            return fota_status;
        }
#else
        mbedtls_sha1_init( &ctx );
        mbedtls_sha1_starts( &ctx );
        mbedtls_sha1_update( &ctx, fota_head_buf, sizeof(fota_header_info_t));
        mbedtls_sha1_finish( &ctx, header_sha1_checksum );        
#endif
        FOTA_LOG_MSGID_I("header checksum = ", 0);
        for (i = 0; i < FOTA_SIGNATURE_SIZE; i++) {
            FOTA_LOG_MSGID_I("%x",1, header_sha1_checksum[i]);
        }

        /* read header checksum */
        fota_status = fota_read(partition, header_checksum, FOTA_SIGNATURE_SIZE);
        if (fota_status != FOTA_STATUS_OK) {
            FOTA_LOG_MSGID_E("_fota_parse_header() ret:%d line:%d",2, fota_status, __LINE__);
            fota_status = FOTA_STATUS_ERROR_FLASH_OP;
            return fota_status;
        }
        FOTA_LOG_MSGID_I("header checksum = ", 0);
        for (i = 0; i < FOTA_SIGNATURE_SIZE; i++) {
            FOTA_LOG_MSGID_I("%x",1, header_checksum[i]);
        }

        /* compare checksum */
        for (i = 0; i < FOTA_SIGNATURE_SIZE; i++) {
            if (header_sha1_checksum[i] != header_checksum[i]) {
                FOTA_LOG_MSGID_E("header integrity check fail", 0);
                return FOTA_STATUS_ERROR_INVALD_PARAMETER;
            }
        }

        /* dump fota header */
        FOTA_LOG_MSGID_I("fota_head.m_magic_ver                    = %x",1, fota_head.m_magic_ver);
        FOTA_LOG_MSGID_I("fota_head.m_bin_num                      = %x",1, fota_head.m_bin_num);
        for (i = 0; i < FOTA_BIN_NUMBER_MAX; i++) {
            FOTA_LOG_MSGID_I("fota_head.m_bin_info[%d].m_bin_length     = %x",2, i, fota_head.m_bin_info[i].m_bin_length);
            FOTA_LOG_MSGID_I("fota_head.m_bin_info[%d].m_bin_offset     = %x",2, i, fota_head.m_bin_info[i].m_bin_offset);
            FOTA_LOG_MSGID_I("fota_head.m_bin_info[%d].m_bin_start_addr = %x",2, i, fota_head.m_bin_info[i].m_bin_start_addr);
            FOTA_LOG_MSGID_I("fota_head.m_bin_info[%d].m_partition_len  = %x",2, i, fota_head.m_bin_info[i].m_partition_length);
            FOTA_LOG_MSGID_I("fota_head.m_bin_info[%d].m_sig_length     = %x",2, i, fota_head.m_bin_info[i].m_sig_length);
            FOTA_LOG_MSGID_I("fota_head.m_bin_info[%d].m_sig_offset     = %x",2, i, fota_head.m_bin_info[i].m_sig_offset);
            FOTA_LOG_MSGID_I("fota_head.m_bin_info[%d].m_is_compressed  = %x",2, i, fota_head.m_bin_info[i].m_is_compressed);
        }

        if ( fota_head.m_bin_num > FOTA_BIN_NUMBER_MAX ) {
            fota_status = FOTA_STATUS_ERROR_INVALD_PARAMETER;
            FOTA_LOG_MSGID_E("_fota_parse_header() ret:%d line:%d",2, fota_status, __LINE__);
        }

        if ( FOTA_HEADER_GET_MAGIC(fota_head.m_magic_ver) != FOTA_HEADER_MAGIC ) {
            fota_status = FOTA_STATUS_ERROR_INVALD_PARAMETER;
            FOTA_LOG_MSGID_E("_fota_parse_header() ret:%d",1, fota_status);
        }
    } else {
        FOTA_LOG_MSGID_E("_fota_parse_header() ret:%d line:%d",2, fota_status, __LINE__);
    }

    FOTA_LOG_MSGID_I("_fota_parse_header() exit", 0);
    return fota_status;
}


static void _fota_show_progress(uint32_t current, uint32_t max)
{
    uint32_t percentage = (current * 100) / max;

    /* feed watchdog regularly. */
    hal_wdt_feed(HAL_WDT_FEED_MAGIC);

    FOTA_LOG_MSGID_I("progress = %d/100",1, percentage);
}


static fota_status_t _fota_integrity_check(uint32_t source, uint8_t  *buffer, uint32_t length, uint32_t bin_number)
{
    fota_status_t  status;
    fota_status_t  fota_status = FOTA_STATUS_OK;
#ifndef MTK_BOOTLOADER_SUPPORT_PARTITION_FOTA
    hal_sha_status_t sha_status;
    hal_sha1_context_t sha1_context;
#else
    mbedtls_sha1_context ctx;
#endif
    uint32_t bin_counter;
    uint8_t bin_sha1_checksum[64] = {0};
    int i;
    uint8_t fota_checksum[FOTA_SIGNATURE_SIZE];
    /* calculate os bin checksum */
    FOTA_LOG_MSGID_I("sha1 init", 0);
#ifndef MTK_BOOTLOADER_SUPPORT_PARTITION_FOTA
    sha_status = hal_sha1_init(&sha1_context);
    if ( sha_status != HAL_SHA_STATUS_OK) {
        FOTA_LOG_MSGID_E("_fota_integrity_check() sha_status:%d line:%d",2, sha_status, __LINE__);
        fota_status = FOTA_STATUS_ERROR_FLASH_OP;
        return fota_status;
    }
#else
    mbedtls_sha1_init( &ctx );
    mbedtls_sha1_starts( &ctx );        
#endif
    bin_counter = fota_head.m_bin_info[bin_number].m_bin_length;
    status = fota_seek(source, fota_head.m_bin_info[bin_number].m_bin_offset);
    if (status != FOTA_STATUS_OK) {
        FOTA_LOG_MSGID_E("_fota_integrity_check() ret:%d line:%d",2, status, __LINE__);
        fota_status = FOTA_STATUS_ERROR_FLASH_OP;
        return fota_status;
    }
    FOTA_LOG_MSGID_I("calculate os bin checksum :", 0);
    _fota_show_progress(fota_head.m_bin_info[bin_number].m_bin_length - bin_counter, fota_head.m_bin_info[bin_number].m_bin_length);
    while (bin_counter != 0) {

        if (bin_counter >= length) {
            status = fota_read(source, buffer, length);
            if (status == FOTA_STATUS_OK) {
#ifndef MTK_BOOTLOADER_SUPPORT_PARTITION_FOTA
                sha_status = hal_sha1_append(&sha1_context, buffer, length);
#else
                mbedtls_sha1_update( &ctx, buffer, length);
#endif
            }
            bin_counter -= length;
        } else {
            status = fota_read(source, buffer, bin_counter);
            if (status == FOTA_STATUS_OK) {
#ifndef MTK_BOOTLOADER_SUPPORT_PARTITION_FOTA
                sha_status = hal_sha1_append(&sha1_context, buffer, bin_counter);
#else
                mbedtls_sha1_update( &ctx, buffer, bin_counter);
#endif
            }
            bin_counter = 0;
        }

        _fota_show_progress(fota_head.m_bin_info[bin_number].m_bin_length - bin_counter, fota_head.m_bin_info[bin_number].m_bin_length);

        if (status != FOTA_STATUS_OK) {
            fota_status = status;
            FOTA_LOG_MSGID_E("_fota_integrity_check() ret:%d line:%d",2, fota_status, __LINE__);
            break;
        }

#ifndef MTK_BOOTLOADER_SUPPORT_PARTITION_FOTA
        if (sha_status != HAL_SHA_STATUS_OK) {
            fota_status = FOTA_STATUS_ERROR_FLASH_OP;
            FOTA_LOG_MSGID_E("_fota_integrity_check() sha_status:%d line:%d",2, sha_status, __LINE__);
            break;
        }
#endif
    }
    FOTA_LOG_MSGID_I("sha1 end", 0);
#ifndef MTK_BOOTLOADER_SUPPORT_PARTITION_FOTA
    sha_status = hal_sha1_end(&sha1_context, bin_sha1_checksum);
    if (sha_status != HAL_SHA_STATUS_OK) {
        FOTA_LOG_MSGID_E("_fota_integrity_check() sha_status:%d line:%d",2, sha_status, __LINE__);
        fota_status = FOTA_STATUS_ERROR_FLASH_OP;
        return fota_status;
    }
#else
    mbedtls_sha1_finish( &ctx, bin_sha1_checksum );
#endif

    if (fota_status != FOTA_STATUS_OK) {
        FOTA_LOG_MSGID_E("_fota_integrity_check() ret:%d line:%d",2, fota_status, __LINE__);
        return fota_status;
    }
    FOTA_LOG_MSGID_I("os bin checksum   = ", 0);
    for (i = 0; i < FOTA_SIGNATURE_SIZE; i++) {
        FOTA_LOG_MSGID_I("%x",1, bin_sha1_checksum[i]);
    }

    /* read checksum from fota bin */
    status = fota_seek(source, fota_head.m_bin_info[bin_number].m_sig_offset);
    if ( status != FOTA_STATUS_OK) {
        FOTA_LOG_MSGID_E("_fota_integrity_check() ret:%d line:%d",2, status, __LINE__);
        fota_status = FOTA_STATUS_ERROR_FLASH_OP;
        return fota_status;
    }
    status = fota_read(source, fota_checksum, fota_head.m_bin_info[bin_number].m_sig_length);
    if ( status != FOTA_STATUS_OK) {
        FOTA_LOG_MSGID_E("_fota_integrity_check() ret:%d line:%d",2, status, __LINE__);
        fota_status = FOTA_STATUS_ERROR_FLASH_OP;
        return fota_status;
    }
    FOTA_LOG_MSGID_I("fota bin checksum = ", 0);
    for (i = 0; i < FOTA_SIGNATURE_SIZE; i++) {
        FOTA_LOG_MSGID_I("%x",1, fota_checksum[i]);
    }

    /* compare checksum */
    for (i = 0; i < FOTA_SIGNATURE_SIZE; i++) {
        if (fota_checksum[i] != bin_sha1_checksum[i]) {
            FOTA_LOG_MSGID_E("integrity check fail", 0);
            fota_status = FOTA_STATUS_ERROR_FLASH_OP;
        }
    }

    if (fota_status == FOTA_STATUS_OK) {
        FOTA_LOG_MSGID_I("integrity check pass", 0);
    }
    return fota_status;
}


fota_status_t _fota_uncompress_update(uint32_t target, uint32_t source, uint8_t  *buffer, uint32_t length , uint32_t bin_number)
{
    fota_status_t  fota_status = FOTA_STATUS_OK;
    fota_partition_t    *p;
    uint32_t bin_counter;
    /* change os start address , use fota header */
    p = _fota_find_partition(target);
    FOTA_LOG_MSGID_I("CM4 partition hard code addr = %x",1, p->address);
    p->address = fota_head.m_bin_info[bin_number].m_bin_start_addr;
#ifdef MTK_FOTA_ON_7686
    if (bl_custom_rom_baseaddr() <= p->address)
    {
        p->address -= bl_custom_rom_baseaddr();
    }
#endif
    p->length = fota_head.m_bin_info[bin_number].m_bin_length;
    FOTA_LOG_MSGID_I("CM4 partition fota head addr = %x",1, p->address);

    fota_seek(target, 0);
    fota_seek(source, fota_head.m_bin_info[bin_number].m_bin_offset);
    bin_counter = fota_head.m_bin_info[bin_number].m_bin_length;
    FOTA_LOG_MSGID_I("start fota update :", 0);


    /* run fota update */
    while (1) {
        _fota_show_progress(fota_head.m_bin_info[bin_number].m_bin_length - bin_counter, fota_head.m_bin_info[bin_number].m_bin_length);
        if (bin_counter >= length) {
            fota_status = fota_read(source, buffer, length);
            if (fota_status != FOTA_STATUS_OK) {
                FOTA_LOG_MSGID_E("_fota_uncompress_update() ret:%d line:%d",2, fota_status, __LINE__);
                return fota_status;
            }
            fota_status = fota_write(target, buffer, length);
            bin_counter -= length;
        } else {
            fota_status = fota_read(source, buffer, bin_counter);
            if (fota_status != FOTA_STATUS_OK) {
                FOTA_LOG_MSGID_E("_fota_uncompress_update() ret:%d line:%d",2, fota_status, __LINE__);
                return fota_status;
            }
            fota_status = fota_write(target, buffer, bin_counter);
            bin_counter = 0;
        }

        if (fota_status != FOTA_STATUS_OK) {
            FOTA_LOG_MSGID_E("_fota_uncompress_update() ret:%d line:%d",2, fota_status, __LINE__);
            return fota_status;
        }

        if (bin_counter == 0) {
            FOTA_LOG_MSGID_I("fota break", 0);
            break;
        }
    }
    return fota_status;
}

fota_status_t _fota_compress_update(uint32_t source, uint32_t bin_number)
{
    int ret = 0;
    fota_partition_t    *p;
    uint32_t lzma_source ;
    uint32_t lzma_dest_addr;
    uint32_t lzma_dest_size;
    p = _fota_find_partition(source);
    lzma_source = p->address + fota_head.m_bin_info[bin_number].m_bin_offset;
    lzma_dest_addr = fota_head.m_bin_info[bin_number].m_bin_start_addr;
    lzma_dest_size = fota_head.m_bin_info[bin_number].m_partition_length;

    FOTA_LOG_MSGID_I("p addr                                    = %x",1, p->address);
    FOTA_LOG_MSGID_I("fota_head.m_bin_info[%d].m_bin_length     = %x",2, bin_number, fota_head.m_bin_info[bin_number].m_bin_length);
    FOTA_LOG_MSGID_I("fota_head.m_bin_info[%d].m_bin_offset     = %x",2, bin_number, fota_head.m_bin_info[bin_number].m_bin_offset);
    FOTA_LOG_MSGID_I("fota_head.m_bin_info[%d].m_bin_start_addr = %x",2, bin_number, fota_head.m_bin_info[bin_number].m_bin_start_addr);
    FOTA_LOG_MSGID_I("fota_head.m_bin_info[%d].m_partition_len  = %x",2, bin_number, fota_head.m_bin_info[bin_number].m_partition_length);

    FOTA_LOG_MSGID_I("lzma_source             = %x",1, lzma_source);
    FOTA_LOG_MSGID_I("lzma_dest_size          = %x",1, lzma_dest_size);
    FOTA_LOG_MSGID_I("lzma_dest_addr          = %x",1, lzma_dest_addr);

#ifdef MTK_FOTA_ON_7686
    if (bl_custom_rom_baseaddr() <= lzma_dest_addr)
    {
        lzma_dest_addr -= bl_custom_rom_baseaddr();
    }
#endif

    ret = lzma_decode2flash(
              (uint8_t *)lzma_dest_addr,
              lzma_dest_size,
              (uint8_t *)lzma_source,
              &lzma_alloc);

    if ( ret != LZMA_OK) {
        FOTA_LOG_MSGID_E("_fota_compress_update() ret:%d line:%d",2, ret, __LINE__);
        return FOTA_STATUS_ERROR_FLASH_OP;
    }
    return FOTA_STATUS_OK;
}

fota_status_t fota_copy(uint32_t target,
                        uint32_t source,
                        uint8_t  *buffer,
                        uint32_t length)
{
    fota_status_t  fota_status = FOTA_STATUS_OK;
    int i;

    FOTA_LOG_MSGID_I("fota_copy() target:%d source:%d buffer:%x length:%d",4, target, source, buffer, length);

    /* Check the validity of parameters. */
    if (buffer == NULL || length == 0) {
        fota_status = FOTA_STATUS_ERROR_INVALD_PARAMETER;
        FOTA_LOG_MSGID_E("fota_copy() ret:%d line:%d",2, fota_status, __LINE__);
    }

    /* fota parse header and header integrity check*/
    if (fota_status == FOTA_STATUS_OK) {
        fota_status = _fota_parse_header(source);
        if (fota_status != FOTA_STATUS_OK) {
            FOTA_LOG_MSGID_E("fota_copy() ret:%d line:%d",2, fota_status, __LINE__);
        }
    }

    /* Integrity check */
    if (fota_status == FOTA_STATUS_OK) {
        for (i = 0; i < fota_head.m_bin_num; i++) {
            fota_status = _fota_integrity_check(source, buffer, length, i);
            if (fota_status != FOTA_STATUS_OK) {
                FOTA_LOG_MSGID_E("fota_copy() ret:%d line:%d",2, fota_status, __LINE__);
                break;
            }
        }
    }

    /* error handling */
    if (fota_status != FOTA_STATUS_OK) {
        _fota_write_update_status(source, fota_status);
    }

    /** start fota update **/
    /* set updating marker ( if updating marker exist , do not write marker again )*/
    if ( _fota_check_updating_marker(source) == FOTA_STATUS_IS_FULL) {
        FOTA_LOG_MSGID_I("updating marker exists", 0);
    }else{
        if (fota_status == FOTA_STATUS_OK) {
            fota_status = _fota_write_marker(source);
            if ( fota_status != FOTA_STATUS_OK) {
                FOTA_LOG_MSGID_E("fota_copy() ret:%d line:%d",2, fota_status, __LINE__);
            }
        }
    }

    /* update OS partition */
    if (fota_status == FOTA_STATUS_OK) {
        for (i = 0; i < fota_head.m_bin_num; i++) {
            if (fota_head.m_bin_info[i].m_is_compressed == 1) {
                fota_status = _fota_compress_update(source, i);
            } else {
                fota_status = _fota_uncompress_update(target, source, buffer, length, i);
            }
            if (fota_status != FOTA_STATUS_OK) {
                FOTA_LOG_MSGID_E("fota_copy() ret:%d line:%d",2, fota_status, __LINE__);
                break;
            }
        }
    }

    /* update finish , clean marker and set update status */
    if (fota_status == FOTA_STATUS_OK) {
        fota_status = _fota_write_update_status(source, fota_status);
        if (fota_status != FOTA_STATUS_OK) {
            FOTA_LOG_MSGID_E("fota_copy() ret:%d line:%d",2, fota_status, __LINE__);
        }
    }

    return fota_status;
}


fota_status_t fota_is_empty(uint32_t partition)
{
    fota_status_t status = FOTA_STATUS_OK;

    FOTA_LOG_MSGID_I("fota_is_empty() partition:%d",1, partition);
    /* Check the validity of parameters. */

    if (flash_s == NULL) {
        FOTA_LOG_MSGID_E("fota_is_empty() ret:%d line:%d",2, FOTA_STATUS_ERROR_NOT_INITIALIZED, __LINE__);
        return FOTA_STATUS_ERROR_NOT_INITIALIZED;
    }

    /* check is updating marker */

    status = _fota_check_updating_marker(partition);
    if ( status == FOTA_STATUS_IS_FULL) {
        FOTA_LOG_MSGID_E("fota_is_empty() ret:%d line:%d",2, FOTA_STATUS_ERROR_NOT_INITIALIZED, __LINE__);
        return status;
    }

    /* check fota triggered */
    status = _fota_check_fota_triggered(partition);
    if ( status == FOTA_STATUS_IS_FULL) {
        FOTA_LOG_MSGID_I("fota is triggered", 0);
        return status;
    }

    FOTA_LOG_MSGID_I("fota is not triggered", 0);
    return FOTA_STATUS_IS_EMPTY;
}

#endif /* MOD_CFG_FOTA_BL_RESERVED */

fota_status_t fota_init(fota_flash_t *flash)
{
    size_t  i;

    FOTA_LOG_MSGID_I("fota_init() flash:%x",1, flash);

    if (!flash || !flash->table || flash->table_entries == 0 ||
            !_fota_is_power_of_2(flash->block_size)) {
        FOTA_LOG_MSGID_E("fota_init() ret:%d line:%d",2, FOTA_STATUS_ERROR_INVALD_PARAMETER, __LINE__);
        return FOTA_STATUS_ERROR_INVALD_PARAMETER;
    }

    flash_s = flash;

    for (i = 0; i < flash->table_entries; i++) {
        if (flash->table[i].address % flash->block_size ||
                flash->table[i].length  % flash->block_size) {
            FOTA_LOG_MSGID_E("fota_init() ret:%d line:%d",2, FOTA_STATUS_ERROR_BLOCK_ALIGN, __LINE__);
            return FOTA_STATUS_ERROR_BLOCK_ALIGN;
        }
    }

    hal_flash_init();

    return FOTA_STATUS_OK;
}


/**
 * Make the <i>partition</i> empty (remove first block) and make
 * fota_is_empty() detects the partition is empty.
 *
 * @retval FOTA_STATUS_ERROR_UNKNOWN_ID if the <i>partition</i> is not in
 * partition table.
 *
 * @retval FOTA_STATUS_ERROR_NOT_INITIALIZED if FOTA was not initialized.
 */
fota_status_t fota_make_empty(uint32_t partition)
{
    fota_partition_t    *p;

    FOTA_LOG_MSGID_I("fota_make_empty() partition:%d",1, partition);

    /* 1. Check the validity of parameters. */

    if (flash_s == NULL) {
        FOTA_LOG_MSGID_E("fota_make_empty() ret:%d line:%d",2, FOTA_STATUS_ERROR_NOT_INITIALIZED, __LINE__);
        return FOTA_STATUS_ERROR_NOT_INITIALIZED;
    }

    if ((p = _fota_find_partition(partition)) == NULL) {
        FOTA_LOG_MSGID_E("fota_make_empty() ret:%d line:%d",2, FOTA_STATUS_ERROR_UNKNOWN_ID, __LINE__);
        return FOTA_STATUS_ERROR_UNKNOWN_ID;
    }

    /*
     * 2. Erase first block.
     */

    fota_port_isr_disable();

    if (hal_flash_erase(p->address, HAL_FLASH_BLOCK_4K) < 0) {
        fota_port_isr_enable();
        FOTA_LOG_MSGID_E("fota_make_empty() ret:%d line:%d",2, FOTA_STATUS_ERROR_FLASH_OP, __LINE__);
        return FOTA_STATUS_ERROR_FLASH_OP;
    }

    fota_port_isr_enable();

    return FOTA_STATUS_OK;
}


fota_status_t fota_seek(uint32_t partition, uint32_t offset)
{
    fota_partition_t    *p;

    FOTA_LOG_MSGID_I("fota_seek() partition:%d offset:%d",2, partition, offset);

    /* 1. Check the validity of parameters. */

    if (flash_s == NULL) {
        FOTA_LOG_MSGID_E("fota_seek() ret:%d line:%d",2, FOTA_STATUS_ERROR_NOT_INITIALIZED, __LINE__);
        return FOTA_STATUS_ERROR_NOT_INITIALIZED;
    }

    if ((p = _fota_find_partition(partition)) == NULL) {
        FOTA_LOG_MSGID_E("fota_seek() ret:%d line:%d",2, FOTA_STATUS_ERROR_UNKNOWN_ID, __LINE__);
        return FOTA_STATUS_ERROR_UNKNOWN_ID;
    }

    if (offset >= p->length) {
        FOTA_LOG_MSGID_E("fota_seek() ret:%d line:%d",2, FOTA_STATUS_ERROR_OUT_OF_RANGE, __LINE__);
        return FOTA_STATUS_ERROR_OUT_OF_RANGE;
    }

    p->offset = offset;
    return FOTA_STATUS_OK;
}


fota_status_t fota_read(uint32_t partition, uint8_t *buffer, uint32_t length)
{
    fota_partition_t    *p;

    FOTA_LOG_MSGID_I("fota_read() partition:%d buffer:%x length:%d",3, partition, buffer, length);

    /* 1. Check the validity of parameters. */

    if (flash_s == NULL) {
        FOTA_LOG_MSGID_E("fota_read() ret:%d line:%d",2, FOTA_STATUS_ERROR_NOT_INITIALIZED, __LINE__);
        return FOTA_STATUS_ERROR_NOT_INITIALIZED;
    }

    if (buffer == 0 || length == 0) {
        FOTA_LOG_MSGID_E("fota_read() ret:%d line:%d",2, FOTA_STATUS_ERROR_INVALD_PARAMETER, __LINE__);
        return FOTA_STATUS_ERROR_INVALD_PARAMETER;
    }

    if ((p = _fota_find_partition(partition)) == NULL) {
        FOTA_LOG_MSGID_E("fota_read() ret:%d line:%d",2, FOTA_STATUS_ERROR_UNKNOWN_ID, __LINE__);
        return FOTA_STATUS_ERROR_UNKNOWN_ID;
    }

    if ((p->offset + length) > p->length) {
        FOTA_LOG_MSGID_E("fota_read() ret:%d line:%d",2, FOTA_STATUS_ERROR_OUT_OF_RANGE, __LINE__);
        return FOTA_STATUS_ERROR_OUT_OF_RANGE;
    }

    /*
     * 2. Read from flash.
     */

    fota_port_isr_disable();

    if (hal_flash_read(p->address + p->offset, buffer, length) < 0) {
        fota_port_isr_enable();
        FOTA_LOG_MSGID_E("fota_read() ret:%d line:%d",2, FOTA_STATUS_ERROR_FLASH_OP, __LINE__);
        return FOTA_STATUS_ERROR_FLASH_OP;
    }

    fota_port_isr_enable();
    p->offset += length;

    return FOTA_STATUS_OK;
}


fota_status_t fota_write(uint32_t partition, const uint8_t *buffer, uint32_t length)
{
    fota_partition_t    *p;
    uint32_t            addr;
    uint32_t            block_idx_start;
    uint32_t            block_idx_end;
    uint32_t            erase_addr;
    uint32_t            i;

    FOTA_LOG_MSGID_I("fota_write() partition:%d buffer:%x length:%d",3, partition, buffer, length);

    /* 1. Check the validity of parameters. */

    if (flash_s == NULL) {
        FOTA_LOG_MSGID_E("fota_write() ret:%d line:%d",2, FOTA_STATUS_ERROR_NOT_INITIALIZED, __LINE__);
        return FOTA_STATUS_ERROR_NOT_INITIALIZED;
    }

    if (buffer == 0 || length == 0) {
        FOTA_LOG_MSGID_E("fota_write() ret:%d line:%d",2, FOTA_STATUS_ERROR_INVALD_PARAMETER, __LINE__);
        return FOTA_STATUS_ERROR_INVALD_PARAMETER;
    }

    if ((p = _fota_find_partition(partition)) == NULL) {
        FOTA_LOG_MSGID_E("fota_write() ret:%d line:%d",2, FOTA_STATUS_ERROR_UNKNOWN_ID, __LINE__);
        return FOTA_STATUS_ERROR_UNKNOWN_ID;
    }

    if ((p->offset + length) > p->length) {
        FOTA_LOG_MSGID_E("fota_write() ret:%d line:%d",2, FOTA_STATUS_ERROR_OUT_OF_RANGE, __LINE__);
        return FOTA_STATUS_ERROR_OUT_OF_RANGE;
    }

    /*
     * 2. Erase block.
     *
     * if the write is to the block boundary, erase the block
     */

    addr = p->address + p->offset;

    block_idx_start = addr / flash_s->block_size;
    block_idx_end = (addr + length - 1) / flash_s->block_size;

    if ((addr % flash_s->block_size) == 0) {
        fota_port_isr_disable();

        if (hal_flash_erase(addr, HAL_FLASH_BLOCK_4K) < 0) {
            fota_port_isr_enable();
            FOTA_LOG_MSGID_E("fota_write() ret:%d line:%d",2, FOTA_STATUS_ERROR_FLASH_OP, __LINE__);
            return FOTA_STATUS_ERROR_FLASH_OP;
        }

        fota_port_isr_enable();
    }

    i = block_idx_start + 1;
    while (i <= block_idx_end) {
        erase_addr = i * flash_s->block_size;
        
        fota_port_isr_disable();
        if (hal_flash_erase(erase_addr, HAL_FLASH_BLOCK_4K) < 0) {
            fota_port_isr_enable();
            FOTA_LOG_MSGID_E("fota_write() ret:%d line:%d",2, FOTA_STATUS_ERROR_FLASH_OP, __LINE__);
            return FOTA_STATUS_ERROR_FLASH_OP;
        }

        fota_port_isr_enable();
        i++;
    }

    /* 3. Write data. */

    fota_port_isr_disable();

    if (hal_flash_write(addr, (uint8_t *)buffer, length) < 0) {
        fota_port_isr_enable();
        FOTA_LOG_MSGID_E("fota_write() ret:%d line:%d",2, FOTA_STATUS_ERROR_FLASH_OP, __LINE__);
        return FOTA_STATUS_ERROR_FLASH_OP;
    }

    fota_port_isr_enable();

    /* 4. Increment pointer. */

    p->offset += length;
    return FOTA_STATUS_OK;
}


#if defined(MTK_SEC_FLASH_SCRAMBLE_ENABLE)
void fota_set_scramble_flag(bool flag)
{
    fota_scramble_flag = flag;
}
#endif


fota_status_t fota_write_upgrade_status(fota_status_t fota_upgrade_status)
{
    fota_status_t status = FOTA_STATUS_OK;
#if defined(MTK_SEC_FLASH_SCRAMBLE_ENABLE)
    if (sboot_efuse_sbc_state == 1) {
        fota_set_scramble_flag(false);
    }
#endif
    status = _fota_write_update_status(FOTA_PARITION_TMP, (int32_t)fota_upgrade_status);
#if defined(MTK_SEC_FLASH_SCRAMBLE_ENABLE)
    if (sboot_efuse_sbc_state == 1) {
        fota_set_scramble_flag(true);
    }
#endif
    return status;
}

