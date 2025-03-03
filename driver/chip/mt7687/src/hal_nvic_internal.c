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

#include "hal_nvic_internal.h"

#ifdef HAL_NVIC_MODULE_ENABLED
#include "hal_nvic.h"
#include "memory_attribute.h"
#include "hal_nvic_internal.h"

#ifdef __cplusplus
extern "C" {
#endif

static void software_trigger_irq(hal_nvic_irq_t irq_number)
{
    NVIC->STIR = (irq_number << NVIC_STIR_INTID_Pos) & NVIC_STIR_INTID_Msk;
    __DSB();
}

void nvic_mask_all_interrupt(void)
{
    hal_nvic_irq_t irq_number;
    uint32_t mask = save_and_set_interrupt_mask();
    for (irq_number = (hal_nvic_irq_t)0; irq_number < IRQ_NUMBER_MAX; irq_number++) {
        NVIC_DisableIRQ(irq_number);
    }
    restore_interrupt_mask(mask);
}

void nvic_unmask_all_interrupt(void)
{
    hal_nvic_irq_t irq_number;
    uint32_t mask = save_and_set_interrupt_mask();

    for (irq_number = (hal_nvic_irq_t)0; irq_number < IRQ_NUMBER_MAX; irq_number++) {
        NVIC_EnableIRQ(irq_number);
    }
    restore_interrupt_mask(mask);
}

void nvic_clear_all_pending_interrupt(void)
{
    hal_nvic_irq_t irq_number;
    for (irq_number = (hal_nvic_irq_t)0; irq_number < IRQ_NUMBER_MAX; irq_number++) {
        NVIC_ClearPendingIRQ(irq_number);
    }
}

hal_nvic_status_t nvic_irq_software_trigger(hal_nvic_irq_t irq_number)
{
    if (irq_number < 0 || irq_number >= IRQ_NUMBER_MAX) {
        return HAL_NVIC_STATUS_INVALID_PARAMETER;
    }

    software_trigger_irq(irq_number);
    return HAL_NVIC_STATUS_OK;
}


#ifdef __cplusplus
}
#endif

#endif /* HAL_NVIC_MODULE_ENABLED */


