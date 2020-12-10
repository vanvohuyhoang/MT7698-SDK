/* Copyright Statement:
 *
 * (C) 2017  Airoha Technology Corp. All rights reserved.
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
#ifdef MTK_LED_ENABLE

#include "hal_gpio.h"
#include "bsp_led_internal.h"
#include "bsp_led.h"
#include "hal_log.h"


extern void bt_driver_hal_led_set(uint8_t idx, uint8_t *led_para);
extern void bt_driver_hal_led_start(uint8_t idx);
extern void bt_driver_hal_led_stop(uint8_t idx);
                                                /*LED_0      LED_1      LED_2,     LED_3*/
hal_gpio_pin_t  led_gpio_number[BSP_LED_MAX]   = {EX_GPIO_6, EX_GPIO_1, EX_GPIO_2, EX_GPIO_3};    /**< The data of this array means pin number corresponding to LED, it is decided by configuration of the board */
uint8_t         led_gpio_function[BSP_LED_MAX] = {EX_GPIO_MODE_LED0, EX_GPIO_MODE_LED1, EX_GPIO_MODE_LED2,EX_GPIO_MODE_LED3};



void  default_bt_driver_hal_led_start(uint8_t idx)
{
    LOG_E(common, "bt_driver_hal_led_start() not defined in BT lib");
    return;
}
void  default_bt_driver_hal_led_stop(uint8_t idx)
{
    LOG_E(common, "bt_driver_hal_led_stop() not defined in BT lib");
    return;
}
void  default_bt_driver_hal_led_set(uint8_t idx, uint8_t *para)
{
    LOG_E(common, "bt_driver_hal_led_set_parameter() not defined in BT lib");
    return;
}

#pragma weak bt_driver_hal_led_start = default_bt_driver_hal_led_start
#pragma weak bt_driver_hal_led_stop  = default_bt_driver_hal_led_stop
#pragma weak bt_driver_hal_led_set   = default_bt_driver_hal_led_set


bsp_led_status_t bsp_led_internal_setting(uint8_t led_number, bsp_led_config_t *config, bool isBreath)
{
    uint8_t parm[8] = {0};
    uint32_t max_time = 0;
    uint32_t t1 = config->tm_on;
    uint32_t t2 = config->tm_off;
    uint32_t t3 = config->tm_dly_aft_rpt;
    uint8_t unit = 1;

    max_time = t1 > t2 ? t1 : t2;
    max_time = max_time > t3 ? max_time : t3;

    if (max_time > 16384) {
        return BSP_LED_INVALID_PARAMETER;
    }

    while (max_time > 256) {
        max_time >>= 1;
        t1 >>= 1;
        t2 >>= 1;
        t3 >>= 1;
        unit <<= 1;
    }

    /* ledTimeStep */
    parm[0] = unit;                     //unit_ms
    parm[1] = config->tms_rpt_onoff-1;  //t1t2_repeat_cnt
    parm[2] = 0;                        //t0_cnt;
    parm[3] = t1;                       //t1_cnt;
    parm[4] = t2;                       //t2_cnt;
    parm[5] = t3;                       //t3_cnt;
    parm[6] = 0;                        //follow_idx;

    /* parm[7] = Bit0: repeat, Bit1: breath, Bit2: inverse, Bit3~6: brightness */
    /* Bit0: repeat */
    parm[7] = config->is_always_rpt;

    /* Bit1: breath */
    if (isBreath) {
        parm[7] |= 0x02;
    }

    /* Bit2: inverse */

    /* Bit3~6: brightness */
    parm[7] |= 0x78;

    bt_driver_hal_led_set(led_number, parm);

    return BSP_LED_OK;
}

void bsp_led_internal_start(uint8_t led_number)
{
    bt_driver_hal_led_start(led_number);
    return;
}

void bsp_led_internal_stop(uint8_t led_number)
{
    bt_driver_hal_led_stop(led_number);
    return;
}

#endif /*MTK_LED_ENABLE*/
