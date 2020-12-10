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
#include "bsp_led_internal.h"
#include "bsp_led.h"

bsp_led_status_t bsp_led_init(uint32_t led_number)
{
    hal_gpio_status_t   ret_gpio;
    hal_pinmux_status_t ret_pinmux;

    if (led_number >= BSP_LED_MAX) {
        return BSP_LED_INVALID_PARAMETER;
    }
    ret_gpio = hal_gpio_init(led_gpio_number[led_number]);
    if (HAL_GPIO_STATUS_OK != ret_gpio) {
        return BSP_LED_ERROR;
    }
    ret_pinmux = hal_pinmux_set_function(led_gpio_number[led_number], led_gpio_function[led_number]);
    if (HAL_PINMUX_STATUS_OK != ret_pinmux) {
        return BSP_LED_ERROR;
    }
    return BSP_LED_OK;
}

bsp_led_status_t bsp_led_set_breath(uint32_t led_number, bsp_led_config_t  *config)
{
    if (led_number >= BSP_LED_MAX) {
        return BSP_LED_INVALID_PARAMETER;
    }
    return bsp_led_internal_setting(led_number, config, true);
}


bsp_led_status_t bsp_led_set_blink(uint32_t  led_number, bsp_led_config_t  *config)
{
    if (led_number >= BSP_LED_MAX) {
        return BSP_LED_INVALID_PARAMETER;
    }
    return bsp_led_internal_setting(led_number, config, false);
}


bsp_led_status_t bsp_led_start(uint32_t led_number)
{
    if (led_number >= BSP_LED_MAX) {
        return BSP_LED_INVALID_PARAMETER;
    }
    bsp_led_internal_start(led_number);
    return BSP_LED_OK;
}



bsp_led_status_t bsp_led_stop(uint32_t led_number)
{
    if (led_number >= BSP_LED_MAX) {
        return BSP_LED_INVALID_PARAMETER;
    }
    bsp_led_internal_stop(led_number);
    return BSP_LED_OK;
}



bsp_led_status_t bsp_led_deinit(uint32_t led_number)
{
    hal_gpio_status_t   ret_gpio;

    if (led_number >= BSP_LED_MAX) {
        return BSP_LED_INVALID_PARAMETER;
    }
    ret_gpio = hal_gpio_deinit(led_gpio_number[led_number]);
    if (HAL_GPIO_STATUS_OK != ret_gpio) {
        return BSP_LED_ERROR;
    }
    return BSP_LED_OK;
}
