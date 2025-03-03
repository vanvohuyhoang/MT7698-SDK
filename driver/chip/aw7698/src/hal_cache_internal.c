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

#include "hal_cache.h"

#ifdef HAL_CACHE_MODULE_ENABLED

#include "hal_cache_internal.h"
#include "memory_attribute.h"
#include "memory_map.h"

#ifdef __cplusplus
extern "C" {
#endif


ATTR_ZIDATA_IN_TCM CACHE_CON_Type g_cache_con;
ATTR_ZIDATA_IN_TCM CACHE_REGION_EN_Type g_cache_region_en;
ATTR_ZIDATA_IN_TCM CACHE_ENTRY_Type g_cache_entry[HAL_CACHE_REGION_MAX];

#ifdef HAL_SLEEP_MANAGER_ENABLED

/* flush and invalidate before enter deepsleep!!! */
ATTR_TEXT_IN_TCM void  cache_status_save(void)
{
    /* flush all */
    CACHE->CACHE_OP &= ~CACHE_OP_OP_MASK;
    CACHE->CACHE_OP |= ((CACHE_FLUSH_ALL_LINES << CACHE_OP_OP_OFFSET) | CACHE_OP_EN_MASK);
    /* invalidate all */
    CACHE->CACHE_OP &= ~CACHE_OP_OP_MASK;
    CACHE->CACHE_OP |= ((CACHE_INVALIDATE_ALL_LINES << CACHE_OP_OP_OFFSET) | CACHE_OP_EN_MASK);
    /* cache disable */
    CACHE->CACHE_CON &= ~CACHE_CON_MCEN_MASK;
}

/* restores only regions that are enabled before entering into deepsleep */
ATTR_TEXT_IN_TCM void  cache_status_restore(void)
{
    CACHE_REGION_EN_Type cache_region_en;
    hal_cache_region_t region;

    cache_region_en = g_cache_region_en;

    /* invalidate all */
    CACHE->CACHE_OP &= ~CACHE_OP_OP_MASK;
    CACHE->CACHE_OP |= ((CACHE_INVALIDATE_ALL_LINES << CACHE_OP_OP_OFFSET) | CACHE_OP_EN_MASK);

    for (region = HAL_CACHE_REGION_0; region < HAL_CACHE_REGION_MAX; region ++) {
        /* If the region is enabled, restore the previous setting of the corresponding region*/
        if (cache_region_en & 1) {
            CACHE->CACHE_ENTRY_N[region] = g_cache_entry[region].cache_entry_n.w;
            CACHE->CACHE_END_ENTRY_N[region] = g_cache_entry[region].cache_end_entry_n.w;
        }
        cache_region_en = cache_region_en >> 1;
    }
    CACHE->CACHE_REGION_EN = g_cache_region_en;
    CACHE->CACHE_CON = g_cache_con.w;
}

#endif

#ifdef __cplusplus
}
#endif

#endif /* HAL_CACHE_MODULE_ENABLED */

