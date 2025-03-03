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

/****************************************************************************
    Module Name:
    Truely Random Number Generator

***************************************************************************/

#include "hal_trng.h"

#ifdef HAL_TRNG_MODULE_ENABLED
#include <stdio.h>
#include <string.h>
#include "type_def.h"
#include "mt7687_cm4_hw_memmap.h"
#include "timer.h"
#include "hal_nvic.h"
#include "hal_nvic_internal.h"

#define TRNG_IDLE  0
#define TRNG_BUSY  1

volatile static uint8_t trng_status = 0;

extern void delay_time(kal_uint32 count);
/**
 * @brief    This function is mainly used to initialize TRUE RANDOM NUMBER GENERATOR hardware .
 * @return    To indicate whether this function call success or not.
 *            If the return value is #HAL_TRNG_OK,it means success;
 * @sa  #hal_trng_deinit
 * @ Example     hal_trng_init();
 */

hal_trng_status_t hal_trng_init(void)
{
    return HAL_TRNG_STATUS_OK;
}

/**
 * @brief   This function is mainly used to de-initialize TRUE RANDOM NUMBER GENERATOR hardware.
 * @return    To indicate whether this function call success or not.
 *            If the return value is #HAL_TRNG_OK,it means success;
 * @sa  #hal_trng_init
 * @Example  hal_trng_deinit();
 */
hal_trng_status_t hal_trng_deinit(void)
{
    return HAL_TRNG_STATUS_OK;
}

/**
 * @brief  This function is mainly used to get  random number which trng generated
 * @param[out]   *random_number is  get random number and which puts the value in *random_number.
 * @return    To indicate whether this function call success or not.
 *               If the return value is #HAL_TRNG_OK,it means success;
 * @ Example   hal_trng_get_generated_random_number(&random_number);

 */

hal_trng_status_t hal_trng_get_generated_random_number(uint32_t *random_number)
{
    UINT16 cnt = 0;
    uint32_t saved_mask;

    saved_mask = save_and_set_interrupt_mask();
    if(trng_status != TRNG_IDLE)
    {
        restore_interrupt_mask(saved_mask);
        return HAL_TRNG_STATUS_ERROR;
    }
    else
    {
        trng_status = TRNG_BUSY;
        restore_interrupt_mask(saved_mask);
    }

    mSetHWEntry(IOT_CRYPTO_TRNG_CTRL_ENABLE, 0);
    mSetHWEntry(IOT_CRYPTO_TRNG_INT_CLR, 0x3);

    // mSetHWEntry(IOT_CRYPTO_TRNG_CONF,0x3FFDC);
    // clear state machine
    *random_number = mGetHWEntry(IOT_CRYPTO_TRNG_DATA);

    mSetHWEntry(IOT_CRYPTO_TRNG_CTRL_ENABLE, 1);

    while (cnt < 100) {
        if (mGetHWEntry(IOT_CRYPTO_TRNG_INT_SET) & 0x1) {
            break;
        }
        delay_time(32);
        cnt++;
    }

    *random_number = mGetHWEntry(IOT_CRYPTO_TRNG_DATA);
  
    mSetHWEntry(IOT_CRYPTO_TRNG_CTRL_ENABLE, 0);
    mSetHWEntry(IOT_CRYPTO_TRNG_INT_CLR, 0x3);

    trng_status = TRNG_IDLE;

    if (cnt >= 100) {
        *random_number = 0x0;
        return HAL_TRNG_STATUS_ERROR;
    } else {
        return HAL_TRNG_STATUS_OK;
    }
}

#endif

