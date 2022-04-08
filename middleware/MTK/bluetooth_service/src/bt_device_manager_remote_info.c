/* Copyright Statement:
 *
 * (C) 2019  Airoha Technology Corp. All rights reserved.
 *
 * This software/firmware and related documentation ("Airoha Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to Airoha Technology Corp. ("Airoha") and/or its licensors.
 * Without the prior written permission of Airoha and/or its licensors,
 * any reproduction, modification, use or disclosure of Airoha Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 * You may only use, reproduce, modify, or distribute (as applicable) Airoha Software
 * if you have agreed to and been bound by the applicable license agreement with
 * Airoha ("License Agreement") and been granted explicit permission to do so within
 * the License Agreement ("Permitted User").  If you are not a Permitted User,
 * please cease any access or use of Airoha Software immediately.
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT AIROHA SOFTWARE RECEIVED FROM AIROHA AND/OR ITS REPRESENTATIVES
 * ARE PROVIDED TO RECEIVER ON AN "AS-IS" BASIS ONLY. AIROHA EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES AIROHA PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH AIROHA SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN AIROHA SOFTWARE. AIROHA SHALL ALSO NOT BE RESPONSIBLE FOR ANY AIROHA
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND AIROHA'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO AIROHA SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT AIROHA'S OPTION, TO REVISE OR REPLACE AIROHA SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * AIROHA FOR SUCH AIROHA SOFTWARE AT ISSUE.
 */

#ifdef MTK_NVDM_ENABLE
#include "nvkey.h"
#include "nvkey_id_list.h"
#endif /* MTK_NVDM_ENABLE */
#include "bt_gap.h"
#include "bt_gap_le.h"
#include "bt_device_manager.h"
#include "bt_callback_manager.h"
#include "bt_device_manager_config.h"
#include "bt_device_manager_internal.h"
#include "bt_device_manager_db.h"
#ifdef MTK_AWS_MCE_ENABLE
#include "bt_aws_mce_report.h"
#endif


#ifndef PACKED
#define PACKED  __attribute__((packed))
#endif

typedef struct {
    bt_gap_link_key_type_t   key_type;
    bt_key_t                 key;
    char name[BT_GAP_MAX_DEVICE_NAME_LENGTH + 1];
} PACKED bt_device_manager_db_remote_paired_info_internal_t;

typedef struct {
    bt_bd_addr_t address;
    bt_device_manager_remote_info_mask_t info_valid_flag;
    bt_device_manager_db_remote_paired_info_internal_t paired_info;
    bt_device_manager_db_remote_version_info_t version_info;
    bt_device_manager_db_remote_pnp_info_t pnp_info;
    bt_device_manager_db_remote_profile_info_t profile_info;
} PACKED bt_device_manager_db_remote_info_t;

static uint8_t bt_dm_remote_sequence[BT_DEVICE_MANAGER_MAX_PAIR_NUM];
static bt_device_manager_db_remote_info_t bt_dm_remote_list_cnt[BT_DEVICE_MANAGER_MAX_PAIR_NUM];

#ifdef MTK_AWS_MCE_ENABLE
static void bt_devoce_manager_aws_mce_packet_callback(bt_aws_mce_report_info_t *para);
static void bt_device_manager_remote_aws_sync_db(bt_device_manager_db_type_t type, uint8_t data_length, uint8_t *data);
#endif

void bt_device_manager_remote_info_init(void)
{
    bt_dmgr_report_id("[BT_DM][REMOTE][I] Remote info init", 0);
    bt_device_manager_db_storage_t remote_storage = {
        .auto_gen = true,
        .storage_type = BT_DEVICE_MANAGER_DB_STORAGE_TYPE_NVKEY,
        .nvkey_id = NVKEYID_BT_HOST_LINK_KEY_RECORD_ID_01
    };
    if (BT_DEVICE_MANAGER_MAX_PAIR_NUM > 16) {
        bt_dmgr_report_id("[BT_DM][REMOTE][W] Storage can only support 16 remote devices now", 0);
        bt_device_manager_assert(0 && "Device maximum number exceed the storage can support");
    } else if (0 == BT_DEVICE_MANAGER_MAX_PAIR_NUM) {
        bt_dmgr_report_id("[BT_DM][REMOTE][W] Maximum pair num is 0", 0);
        return;
    }
    memset(bt_dm_remote_list_cnt, 0, sizeof(bt_dm_remote_list_cnt));
    memset(bt_dm_remote_sequence, 0, sizeof(bt_dm_remote_sequence));
    for (uint8_t index = 0; index < BT_DEVICE_MANAGER_MAX_PAIR_NUM; index++) {
        remote_storage.nvkey_id = NVKEYID_BT_HOST_LINK_KEY_RECORD_ID_01 + index;
        bt_device_manager_db_init(BT_DEVICE_MANAGER_DB_TYPE_REMOTE_DEVICE0_INFO + index,
            &remote_storage, &bt_dm_remote_list_cnt[index], sizeof(bt_dm_remote_list_cnt[index]));
    }
    remote_storage.nvkey_id = NVKEYID_BT_HOST_LINK_KEY_RECORD_SEQUENCE;
    bt_device_manager_db_init(BT_DEVICE_MANAGER_DB_TYPE_REMOTE_SEQUENCE_INFO,
            &remote_storage, bt_dm_remote_sequence, sizeof(bt_dm_remote_sequence));
#ifdef MTK_AWS_MCE_ENABLE
    bt_aws_mce_report_register_callback(BT_AWS_MCE_REPORT_MODULE_DM, bt_devoce_manager_aws_mce_packet_callback);
#endif
}

bt_status_t bt_device_manager_remote_delete_info(bt_bd_addr_t *addr, bt_device_manager_remote_info_mask_t info_mask)
{
    bt_device_manager_db_remote_info_t *temp_remote = &bt_dm_remote_list_cnt[0];
    bt_dmgr_report_id("[BT_DM][REMOTE][I] Delete info addr %p, info mask %d", 2, addr, info_mask);
    for(uint8_t index = 0; index < BT_DEVICE_MANAGER_MAX_PAIR_NUM; temp_remote++, index++) {
        if (!bt_dm_remote_sequence[index]) {
            continue;
        }
        if (info_mask == 0 && addr == NULL) {
            bt_dm_remote_sequence[index] = 0;
            bt_device_manager_db_update(BT_DEVICE_MANAGER_DB_TYPE_REMOTE_SEQUENCE_INFO);
        #ifdef MTK_AWS_MCE_ENABLE
            bt_device_manager_remote_aws_sync_db(BT_DEVICE_MANAGER_DB_TYPE_REMOTE_SEQUENCE_INFO,
                sizeof(bt_dm_remote_sequence), (void*)bt_dm_remote_sequence);
        #endif
            continue;
        } else if (addr == NULL) {
            temp_remote->info_valid_flag &= (~info_mask);
            bt_device_manager_db_update(BT_DEVICE_MANAGER_DB_TYPE_REMOTE_DEVICE0_INFO + index);
        #ifdef MTK_AWS_MCE_ENABLE
            bt_device_manager_remote_aws_sync_db(BT_DEVICE_MANAGER_DB_TYPE_REMOTE_DEVICE0_INFO + index,
                sizeof(bt_device_manager_db_remote_info_t), (void*)(bt_dm_remote_list_cnt + index));
        #endif
            continue;
        } else if (info_mask == 0 && !memcmp(&(temp_remote->address), addr, sizeof(bt_bd_addr_t))) {
            for (uint8_t i = 0; i < BT_DEVICE_MANAGER_MAX_PAIR_NUM; i++) {
                if (bt_dm_remote_sequence[i] > bt_dm_remote_sequence[index]) {
                    bt_dm_remote_sequence[i]--;
                }
            }
            bt_dm_remote_sequence[index] = 0;
            bt_device_manager_db_update(BT_DEVICE_MANAGER_DB_TYPE_REMOTE_SEQUENCE_INFO);
        #ifdef MTK_AWS_MCE_ENABLE
            bt_device_manager_remote_aws_sync_db(BT_DEVICE_MANAGER_DB_TYPE_REMOTE_SEQUENCE_INFO,
                sizeof(bt_dm_remote_sequence), (void*)bt_dm_remote_sequence);
        #endif
            break;
        } else if (!memcmp(&(temp_remote->address), addr, sizeof(bt_bd_addr_t))) {
            temp_remote->info_valid_flag &= (~info_mask);
            bt_device_manager_db_update(BT_DEVICE_MANAGER_DB_TYPE_REMOTE_DEVICE0_INFO + index);
        #ifdef MTK_AWS_MCE_ENABLE
            bt_device_manager_remote_aws_sync_db(BT_DEVICE_MANAGER_DB_TYPE_REMOTE_DEVICE0_INFO + index,
                sizeof(bt_device_manager_db_remote_info_t), (void*)(bt_dm_remote_list_cnt + index));
        #endif
            break;
        }
    }
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_device_manager_remote_top(bt_bd_addr_t addr)
{
    uint8_t index = 0;
    uint8_t find_index = 0xFF;
    bt_device_manager_db_remote_info_t *temp_remote = &bt_dm_remote_list_cnt[0];
    bt_dmgr_report_id("[BT_DM][REMOTE][I] Top device", 0);
    for (index = 0; index < BT_DEVICE_MANAGER_MAX_PAIR_NUM; index++, temp_remote++) {
        if (bt_dm_remote_sequence[index] && !memcmp(temp_remote->address, addr, sizeof(bt_bd_addr_t))) {
            find_index = index;
            break;
        }
    }
    if (0xFF == find_index) {
        bt_dmgr_report_id("[BT_DM][REMOTE][W] Top device fail not find dev", 0);
        return BT_STATUS_FAIL;
    }
    if (1 == bt_dm_remote_sequence[find_index]) {
        return BT_STATUS_SUCCESS;
    }
    for (index = 0; index < BT_DEVICE_MANAGER_MAX_PAIR_NUM; index++) {
        if (bt_dm_remote_sequence[index] && bt_dm_remote_sequence[index] < bt_dm_remote_sequence[find_index]) {
            bt_dm_remote_sequence[index]++;
        }
    }
    bt_dm_remote_sequence[find_index] = 1;
    bt_device_manager_db_update(BT_DEVICE_MANAGER_DB_TYPE_REMOTE_SEQUENCE_INFO);
#ifdef MTK_AWS_MCE_ENABLE
    bt_device_manager_remote_aws_sync_db(BT_DEVICE_MANAGER_DB_TYPE_REMOTE_SEQUENCE_INFO,
        sizeof(bt_dm_remote_sequence), (void*)bt_dm_remote_sequence);
#endif
    return BT_STATUS_SUCCESS;
}

bt_bd_addr_t *bt_device_manager_remote_get_dev_by_seq_num(uint8_t sequence)
{
    bt_device_manager_db_remote_info_t *temp_remote = &bt_dm_remote_list_cnt[0];
    bt_dmgr_report_id("[BT_DM][REMOTE][I] Get dev by seq num %d", 1, sequence);
    for (uint8_t index = 0; sequence && index < BT_DEVICE_MANAGER_MAX_PAIR_NUM; index++, temp_remote++) {
        if (sequence == bt_dm_remote_sequence[index]) {
            return &(temp_remote->address);
        }
    }
    return NULL;
}

uint32_t bt_device_manager_remote_get_paired_num(void)
{
    uint32_t ret = 0;
    bt_device_manager_db_remote_info_t *temp_remote = &bt_dm_remote_list_cnt[0];
    bt_dmgr_report_id("[BT_DM][REMOTE][I] Get paiared num", 0);
    for (uint8_t index = 0; index < BT_DEVICE_MANAGER_MAX_PAIR_NUM; index++, temp_remote++) {
        if (bt_dm_remote_sequence[index] && (temp_remote->info_valid_flag & BT_DEVICE_MANAGER_REMOTE_INFO_MASK_PAIRED)) {
            ret++;
        }
    }
    return ret;
}

void bt_device_manager_remote_get_paired_list(bt_device_manager_paired_infomation_t* info, uint32_t* read_count)
{
    uint32_t count = 0;
    bt_device_manager_db_remote_info_t *temp_remote = &bt_dm_remote_list_cnt[0];
    bt_dmgr_report_id("[BT_DM][REMOTE][I] Get Paired list", 0);
    if (NULL == info || *read_count == 0) {
        bt_dmgr_report_id("[BT_DM][REMOTE][E] Get Paired list error param info buffer : %p, read count : %d", 2, info, *read_count);
        return;
    }
    for (uint8_t index = 0; index < BT_DEVICE_MANAGER_MAX_PAIR_NUM; index++, temp_remote++) {
        if ((count < *read_count) && bt_dm_remote_sequence[index] && (temp_remote->info_valid_flag & BT_DEVICE_MANAGER_REMOTE_INFO_MASK_PAIRED)) {
            memcpy(info->address, temp_remote->address, sizeof(bt_bd_addr_t));
            memcpy(info->name, temp_remote->paired_info.name, BT_GAP_MAX_DEVICE_NAME_LENGTH);
            count++;
        }
    }
    *read_count = count;
}

bt_status_t bt_device_manager_remote_find_paired_info_by_seq_num(uint8_t sequence, bt_device_manager_db_remote_paired_info_t *info)
{
    bt_device_manager_db_remote_info_t *temp_remote = &bt_dm_remote_list_cnt[0];
    bt_dmgr_report_id("[BT_DM][REMOTE][I] Find paired info by sequence num %d", 1, sequence);
    if (NULL == info) {
        bt_dmgr_report_id("[BT_DM][REMOTE][E] Find paired info buffer is null", 0);
        return BT_STATUS_FAIL;
    }
    for (uint8_t index = 0; sequence && index < BT_DEVICE_MANAGER_MAX_PAIR_NUM; index++, temp_remote++) {
        if (bt_dm_remote_sequence[index] == sequence) {
            if (temp_remote->info_valid_flag & BT_DEVICE_MANAGER_REMOTE_INFO_MASK_PAIRED) {
                memcpy(info->name, temp_remote->paired_info.name, BT_GAP_MAX_DEVICE_NAME_LENGTH);
                memcpy(info->paired_key.address, temp_remote->address, sizeof(bt_bd_addr_t));
                memcpy(info->paired_key.key, temp_remote->paired_info.key, sizeof(bt_key_t));
                info->paired_key.key_type = temp_remote->paired_info.key_type;
                bt_dmgr_report_id("[BT_DM][REMOTE][I] Find paired info success", 0);
                return BT_STATUS_SUCCESS;
            }
            break;
        }
    }
    return BT_STATUS_FAIL;
}

static bt_status_t bt_device_manager_remote_info_update(bt_bd_addr_t addr, bt_device_manager_remote_info_mask_t type, void *data)
{
    bt_dmgr_report_id("[BT_DM][REMOTE][I] Update info type 0x%x", 1, type);
    if (0 == BT_DEVICE_MANAGER_MAX_PAIR_NUM) {
        return BT_STATUS_FAIL;
    }
    if (NULL == data) {
        bt_dmgr_report_id("[BT_DM][REMOTE][E] Update info buffer is null", 0);
        return BT_STATUS_FAIL;
    }
    uint8_t index = 0;
    uint8_t item_index = 0xFF;
    bt_device_manager_db_remote_info_t *temp_remote = &bt_dm_remote_list_cnt[0];
    bt_device_manager_db_remote_info_t *find_remote = NULL;
    for (index = 0; index < BT_DEVICE_MANAGER_MAX_PAIR_NUM; index++, temp_remote++) {
        if (bt_dm_remote_sequence[index] && !memcmp(temp_remote->address, addr, sizeof(bt_bd_addr_t))) {
            find_remote = temp_remote;
            item_index = index;
            break;
        } else if (item_index == 0xFF && (!bt_dm_remote_sequence[index] || BT_DEVICE_MANAGER_MAX_PAIR_NUM == bt_dm_remote_sequence[index])) {
            find_remote = temp_remote;
            item_index = index;
        }
    }
    if (BT_DEVICE_MANAGER_MAX_PAIR_NUM == index) {
        memcpy(find_remote->address, addr, sizeof(bt_bd_addr_t));
        find_remote->info_valid_flag = type;
        bt_dm_remote_sequence[item_index] = 1;
        for (index = 0; index < BT_DEVICE_MANAGER_MAX_PAIR_NUM; index++) {
            if (index != item_index && bt_dm_remote_sequence[index]) {
                bt_dm_remote_sequence[index]++;
            }
        }
        bt_device_manager_db_update(BT_DEVICE_MANAGER_DB_TYPE_REMOTE_SEQUENCE_INFO);
    #ifdef MTK_AWS_MCE_ENABLE
        bt_device_manager_remote_aws_sync_db(BT_DEVICE_MANAGER_DB_TYPE_REMOTE_SEQUENCE_INFO,
            sizeof(bt_dm_remote_sequence), (void*)bt_dm_remote_sequence);
    #endif
    } else {
        find_remote->info_valid_flag |= type;
    }
    switch (type) {
        case BT_DEVICE_MANAGER_REMOTE_INFO_MASK_PAIRED: {
            bt_device_manager_db_remote_paired_info_t *info = (void *)data;
            memcpy(find_remote->paired_info.name, info->name, BT_GAP_MAX_DEVICE_NAME_LENGTH);
            memcpy(find_remote->paired_info.key, info->paired_key.key, sizeof(bt_key_t));
            find_remote->paired_info.key_type = info->paired_key.key_type;
            break;
        }
        case BT_DEVICE_MANAGER_REMOTE_INFO_MASK_VERSION: {
            bt_device_manager_db_remote_version_info_t *info = (void*)data;
            memcpy(&(find_remote->version_info), info, sizeof(bt_device_manager_db_remote_version_info_t));
            break;
        }
        case BT_DEVICE_MANAGER_REMOTE_INFO_MASK_PROFILE: {
            bt_device_manager_db_remote_profile_info_t *info = (void*)data;
            memcpy(&(find_remote->profile_info), info, sizeof(bt_device_manager_db_remote_profile_info_t));
            break;
        }
        case BT_DEVICE_MANAGER_REMOTE_INFO_MASK_PNP: {
            bt_device_manager_db_remote_pnp_info_t *info = (void*)data;
            find_remote->pnp_info.product_id = info->product_id;
            find_remote->pnp_info.vender_id = info->vender_id;
            break;
        }
        default:
            bt_dmgr_report_id("[BT_DM][REMOTE][E] Update info type is invalid", 0);
            return BT_STATUS_FAIL;
    }
    bt_device_manager_db_update(BT_DEVICE_MANAGER_DB_TYPE_REMOTE_DEVICE0_INFO + item_index);
#ifdef MTK_AWS_MCE_ENABLE
    bt_device_manager_remote_aws_sync_db(BT_DEVICE_MANAGER_DB_TYPE_REMOTE_DEVICE0_INFO + index,
        sizeof(bt_device_manager_db_remote_info_t), (void*)(bt_dm_remote_list_cnt + index));
#endif
    return BT_STATUS_SUCCESS;
}

static bt_status_t bt_device_manager_remote_info_find(bt_bd_addr_t addr, bt_device_manager_remote_info_mask_t type, void *data)
{
    bt_dmgr_report_id("[BT_DM][REMOTE][I] Find find info type 0x%x", 1, type);
    if (0 == BT_DEVICE_MANAGER_MAX_PAIR_NUM) {
        return BT_STATUS_FAIL;
    }
    if (NULL == data) {
        bt_dmgr_report_id("[BT_DM][REMOTE][E] Find info buffer is null", 0);
        return BT_STATUS_FAIL;
    }
    bt_device_manager_db_remote_info_t *temp_remote = &bt_dm_remote_list_cnt[0];
    for (uint8_t index = 0; index < BT_DEVICE_MANAGER_MAX_PAIR_NUM; index++, temp_remote++) {
        if (bt_dm_remote_sequence[index] && !memcmp(temp_remote->address, addr, sizeof(bt_bd_addr_t)) &&
            (temp_remote->info_valid_flag & type)) {
            break;
        } else if (BT_DEVICE_MANAGER_MAX_PAIR_NUM == (index + 1)) {
            bt_dmgr_report_id("[BT_DM][REMOTE][I] Find info fail", 0);
            return BT_STATUS_FAIL;
        }
    }
    switch (type) {
        case BT_DEVICE_MANAGER_REMOTE_INFO_MASK_PAIRED: {
            bt_device_manager_db_remote_paired_info_t *info = (void *)data;
            memcpy(info->name, temp_remote->paired_info.name, BT_GAP_MAX_DEVICE_NAME_LENGTH);
            memcpy(info->paired_key.address, temp_remote->address, sizeof(bt_bd_addr_t));
            memcpy(info->paired_key.key, temp_remote->paired_info.key, sizeof(bt_key_t));
            info->paired_key.key_type = temp_remote->paired_info.key_type;
            break;
        }
        case BT_DEVICE_MANAGER_REMOTE_INFO_MASK_VERSION: {
            bt_device_manager_db_remote_version_info_t *info = (void*)data;
            memcpy(info, &(temp_remote->version_info), sizeof(bt_device_manager_db_remote_version_info_t));
            break;
        }
        case BT_DEVICE_MANAGER_REMOTE_INFO_MASK_PROFILE: {
            bt_device_manager_db_remote_profile_info_t *info = (void*)data;
            memcpy(info, &(temp_remote->profile_info), sizeof(bt_device_manager_db_remote_profile_info_t));
            break;
        }
        case BT_DEVICE_MANAGER_REMOTE_INFO_MASK_PNP: {
            bt_device_manager_db_remote_pnp_info_t *info = (void*)data;
            info->product_id = temp_remote->pnp_info.product_id;
            info->vender_id = temp_remote->pnp_info.vender_id;
            break;
        }
        default:
            bt_dmgr_report_id("[BT_DM][REMOTE][E] find info type is invalid", 0);
            return BT_STATUS_FAIL;
    }
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_device_manager_remote_find_paired_info(bt_bd_addr_t addr, bt_device_manager_db_remote_paired_info_t *info)
{
    return bt_device_manager_remote_info_find(addr, BT_DEVICE_MANAGER_REMOTE_INFO_MASK_PAIRED, (void*)info);
}

bt_status_t bt_device_manager_remote_update_paired_info(bt_bd_addr_t addr, bt_device_manager_db_remote_paired_info_t *info)
{
    return bt_device_manager_remote_info_update(addr, BT_DEVICE_MANAGER_REMOTE_INFO_MASK_PAIRED, (void*)info);
}

bt_status_t bt_device_manager_remote_find_version_info(bt_bd_addr_t addr, bt_device_manager_db_remote_version_info_t *info)
{
    return bt_device_manager_remote_info_find(addr, BT_DEVICE_MANAGER_REMOTE_INFO_MASK_VERSION, (void*)info);
}

bt_status_t bt_device_manager_remote_update_version_info(bt_bd_addr_t addr, bt_device_manager_db_remote_version_info_t *info)
{
    return bt_device_manager_remote_info_update(addr, BT_DEVICE_MANAGER_REMOTE_INFO_MASK_VERSION, (void*)info);
}

bt_status_t bt_device_manager_remote_find_profile_info(bt_bd_addr_t addr, bt_device_manager_db_remote_profile_info_t *info)
{
    return bt_device_manager_remote_info_find(addr, BT_DEVICE_MANAGER_REMOTE_INFO_MASK_PROFILE, (void*)info);
}

bt_status_t bt_device_manager_remote_update_profile_info(bt_bd_addr_t addr, bt_device_manager_db_remote_profile_info_t *info)
{
    return bt_device_manager_remote_info_update(addr, BT_DEVICE_MANAGER_REMOTE_INFO_MASK_PROFILE, (void*)info);
}

bt_status_t bt_device_manager_remote_find_pnp_info(bt_bd_addr_t addr, bt_device_manager_db_remote_pnp_info_t *info)
{
    return bt_device_manager_remote_info_find(addr, BT_DEVICE_MANAGER_REMOTE_INFO_MASK_PNP, (void*)info);
}

bt_status_t bt_device_manager_remote_update_pnp_info(bt_bd_addr_t addr, bt_device_manager_db_remote_pnp_info_t *info)
{
    return bt_device_manager_remote_info_update(addr, BT_DEVICE_MANAGER_REMOTE_INFO_MASK_PNP, (void*)info);
}

#ifdef MTK_AWS_MCE_ENABLE
static void bt_devoce_manager_aws_mce_packet_callback(bt_aws_mce_report_info_t *para)
{
    if (NULL == para) {
        bt_dmgr_report_id("[BT_DM][REMOTE][AWS][I] Packet callback para is null !!!", 0);
        return;
    }
    bt_dmgr_report_id("[BT_DM][REMOTE][AWS][I] Packet callback module_id:0x%x, is_sync:%d, sync_time:%d, param_len:%d!!!", 4,
        para->module_id, para->is_sync, para->sync_time, para->param_len);
    if (BT_AWS_MCE_REPORT_MODULE_DM != para->module_id) {
        bt_dmgr_report_id("[BT_DM][REMOTE][AWS][I] Packet callback module is not DM!!!", 0);
        return;
    }
    bt_device_manager_db_type_t type = ((uint8_t *)para->param)[0];
    bt_dmgr_report_id("[BT_DM][REMOTE][AWS][I] Packet callback type %d!!!", 1, type);
    if (BT_DEVICE_MANAGER_DB_TYPE_REMOTE_SEQUENCE_INFO == type) {
        memcpy(&bt_dm_remote_sequence, ((uint8_t *)para->param) + 1, sizeof(bt_dm_remote_sequence));
    } else if (BT_DEVICE_MANAGER_DB_TYPE_REMOTE_DEVICE0_INFO <= type && BT_DEVICE_MANAGER_DB_TYPE_REMOTE_DEVICE_MAX >= type) {
        uint8_t sequence_num = type - BT_DEVICE_MANAGER_DB_TYPE_REMOTE_DEVICE0_INFO;
        memcpy(&(bt_dm_remote_list_cnt[sequence_num]), ((uint8_t *)para->param) + 1, sizeof(bt_device_manager_db_remote_info_t));
    } else {
        bt_dmgr_report_id("[BT_DM][REMOTE][AWS][I] Unkown packet type 0x%02X", 1, type);
        return;
    }
    bt_device_manager_db_update(type);
}

static void bt_device_manager_remote_aws_sync_db(bt_device_manager_db_type_t type, uint8_t data_length, uint8_t *data)
{
    bt_status_t status;
    uint32_t report_length = sizeof(bt_aws_mce_report_info_t) + data_length + 1;
    uint8_t temp_buffer[report_length];
    bt_aws_mce_report_info_t *dm_report = (void *)temp_buffer;
    uint8_t *data_payload = temp_buffer + sizeof(bt_aws_mce_report_info_t);

    bt_dmgr_report_id("[BT_DM][REMOTE][AWS][I] Sync db type 0x%02X", 1, type);
    memset(temp_buffer, 0, sizeof(temp_buffer));
    dm_report->module_id = BT_AWS_MCE_REPORT_MODULE_DM;
    dm_report->param_len = data_length + 1;
    dm_report->param = (void *)data_payload;
    data_payload[0] = type;
    memcpy(data_payload + 1, (void*)data, data_length);
    if (BT_STATUS_SUCCESS != (status = bt_aws_mce_report_send_event(dm_report))) {
        bt_dmgr_report_id("[BT_DM][REMOTE][AWS][W] Sync db failed status 0x%x!!!", 1, status);
    }
}

void bt_device_manager_remote_aws_sync_to_partner(void)
{
    bt_dmgr_report_id("[BT_DM][REMOTE][AWS][I] Sync to partner", 0);
    if (0 == BT_DEVICE_MANAGER_MAX_PAIR_NUM) {
        return;
    }
    // remote device info sync.
    for (uint8_t index = 0; index < BT_DEVICE_MANAGER_MAX_PAIR_NUM; index++) {
        if (bt_dm_remote_sequence[index]) {
            bt_device_manager_remote_aws_sync_db(BT_DEVICE_MANAGER_DB_TYPE_REMOTE_DEVICE0_INFO + index,
                sizeof(bt_device_manager_db_remote_info_t), (void*)(bt_dm_remote_list_cnt + index));
        }
    }
    // sequence info sync.
    bt_device_manager_remote_aws_sync_db(BT_DEVICE_MANAGER_DB_TYPE_REMOTE_SEQUENCE_INFO,
                sizeof(bt_dm_remote_sequence), (void*)bt_dm_remote_sequence);
}
#endif

