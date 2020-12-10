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
#ifndef __BSP_LED_H__
#define __BSP_LED_H__
#include "hal_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup BSP
 * @{
 * @addtogroup LED
 * @{
 * This section describes the programming interfaces of the BSP LED driver, Which provides the driver to operate LED.
 * 
 * @section BSP_LED_Features_Chapter Supported features
 * - \b Support \b Breath \b and \b Blink \b modes. \n
 *   Based on user configuration, LED can operates in different modes:
 * - \b LED \b Breath Mode \n
 * Function #bsp_led_set_breath() should be called, if user want to using LED breath mode which change brightness gradually.
 * - \b LED \b Blink Mode \n
 * Function #bsp_led_set_blink() should be called, if user want to using LED blink mode which led have two state(ON or OFF).
 *
 * @section BSP_LED_Driver_Usage_Chapter How to use the interface of LED
 * - Configure GPIO pin and mode which used by LED. Those information is defined in bsp_led_internal.c file which should be accordingly with Board.
 * - This api shoud be call after BT initialized done, or all LED setting will be ignore.
 * - The user must pre-define BSP_LED_MAX macro which indicate how many led that board support.
 * - For SIP flash, the flash operation address is just a physical address from 0 to FLASH size.
 * - Use Bsp led as breath or blink mode. \n
 *   - Step 1: Call #bsp_led_init() to initialize the SIP FLASH or external FLASH module.
 *   - Step 2: Call #bsp_led_set_breath() or #bsp_led_set_blink() to setup led hardware configure.
 *   - Step 3: Call #bsp_led_start() to start breath or blink.
 *   - Step 4: Call #bsp_led_stop() to stop breath or blink.
 *   - Step 5: Call #bsp_led_deinit() to de-initialize the driver.
 *  @code
 *    void bsp_led_example(void)
 *    {
 *        bsp_led_config_t    led_cfg;
 *        uint8_t             led_idx;
 *
 *        led_cfg.tm_dly_sta    = 0;
 *        led_cfg.tm_on         = 1000;
 *        led_cfg.tm_off        = 1000;
 *        led_cfg.tms_rpt_onoff = 2;
 *        led_cfg.tm_dly_aft_rpt= 2000;
 *        led_cfg.is_always_rpt = true;
 *  
 *        for(led_idx = BSP_LED_0; led_idx < BSP_LED_MAX; led_idx++)
 *        {
 *            bsp_led_init(led_idx);
 *            if(led_idx%2)
 *                bsp_led_set_breath(led_idx, &led_cfg);//breath mode
 *            else
 *                bsp_led_set_blink(led_idx, &led_cfg);//blink mode
 *            bsp_led_start(led_idx);
 *        }
 *        vTaskDelay(10000);
 *        for(led_idx = BSP_LED_0; led_idx < BSP_LED_MAX; led_idx++)
 *        {
 *            bsp_led_stop(led_idx);
 *            bsp_led_deinit(led_idx);
 *         }
 *    }
 *
 *   @endcode
 */




/** @defgroup bsp_led_enum Enum
  * @{
  */
/** @brief This enum defines led channel. */
typedef enum {
    BSP_LED_0 = 0,
    BSP_LED_1 = 1,
    BSP_LED_2 = 2,
    BSP_LED_3 = 3,
    BSP_LED_MAX,
}bsp_led_id_t;

/** @brief This enum defines led state. */
typedef enum {
    BSP_LED_OFF  = 0,               /**< define led state of off */
    BSP_LED_ON = 1                  /**< define led state of on */
} bsp_led_state_t;

/**
  * @}
  */


/** @defgroup bsp_led_struct Struct
  * @{
  */
/** @brief This struct defines LED configure parameters. */

typedef struct {
    uint32_t    tm_dly_sta;     /**< T0: delay start time */
    uint32_t    tm_on;          /**< T1: led on time */
    uint32_t    tm_off;         /**< T2: led off time */
    uint32_t    tm_dly_aft_rpt; /**< T3: time delay after led finish t1t2 repeat */
    uint8_t     tms_rpt_onoff;  /**< T1T2_Repeat: t1t2 repeat times */
    bool        is_always_rpt;  /**<  1: always repeat,0:repeat once */
}bsp_led_config_t;
/** @brief This enum define API status of LED */
typedef enum {
    BSP_LED_ERROR             = -2,         /**< This value means a led function EEROR */
    BSP_LED_INVALID_PARAMETER = -1,         /**< This value means an invalid parameter */
    BSP_LED_OK                = 0           /**< This value meeas no error happen during the function call*/
} bsp_led_status_t;

/**
  * @}
  */


/**
 * @brief     This function for initialize LED setting
 * @param[in] led_number   initializes the specified LED number.
 * @return
 *                #BSP_LED_OK , if the operation completed successfully. \n
 *                #BSP_LED_INVALID_PARAMETER , if parameter is invalid. \n
 * @par       Example
 * @code 
 *       bsp_led_init(BSP_LED_MAX);
 * @endcode
 */
bsp_led_status_t bsp_led_init(uint32_t led_number);


/**
 * @brief     This function for configure LED to breath mode
 * @param[in] led_number   initializes the specified LED number.
 * @param[in] config   specifies a user defined led parameter.
 * @return
 *                #BSP_LED_OK , if the operation completed successfully. \n
 *                #BSP_LED_INVALID_PARAMETER , if parameter is invalid. \n
 * @par       Example
 * @code 
 *        bsp_led_config_t    led_cfg;
 *
 *        led_cfg.tm_dly_sta    = 0;
 *        led_cfg.tm_on         = 1000;
 *        led_cfg.tm_off        = 1000;
 *        led_cfg.tms_rpt_onoff = 2;
 *        led_cfg.tm_dly_aft_rpt= 2000;
 *        led_cfg.is_always_rpt = true;
 *        bsp_led_set_breath(led_idx, &led_cfg);
 * @endcode
 */
bsp_led_status_t bsp_led_set_breath(uint32_t led_number, bsp_led_config_t  *config);

/**
 * @brief     This function for configure LED to Blink mode
 * @param[in] led_number   initializes the specified LED number.
 * @param[in] config   specifies a user defined led parameter.
 * @return
 *                #BSP_LED_OK , if the operation completed successfully. \n
 *                #BSP_LED_INVALID_PARAMETER , if parameter is invalid. \n
 * @par       Example
 * @code 
 *        bsp_led_config_t    led_cfg;
 *
 *        led_cfg.tm_dly_sta    = 0;
 *        led_cfg.tm_on         = 1000;
 *        led_cfg.tm_off        = 1000;
 *        led_cfg.tms_rpt_onoff = 2;
 *        led_cfg.tm_dly_aft_rpt= 2000;
 *        led_cfg.is_always_rpt = true;
 *        bsp_led_set_blink(led_idx, &led_cfg);
 * @endcode
 */
bsp_led_status_t bsp_led_set_blink(uint32_t  led_number, bsp_led_config_t  *config);

/**
 * @brief     This function for start led twinkle
 * @param[in] led_number   initializes the specified LED number.
 * @return
 *                #BSP_LED_OK , if the operation completed successfully. \n
 *                #BSP_LED_INVALID_PARAMETER , if parameter is invalid. \n
 * @par       Example
 * @code 
 *        bsp_led_start(led_idx);
 * @endcode
 */
bsp_led_status_t bsp_led_start(uint32_t led_number);
/**
 * @brief     This function for stop led twinkle
 * @param[in] led_number   initializes the specified LED number.
 * @return
 *                #BSP_LED_OK , if the operation completed successfully. \n
 *                #BSP_LED_INVALID_PARAMETER , if parameter is invalid. \n
 * @par       Example
 * @code 
 *        bsp_led_stop(led_idx);
 * @endcode
 */
bsp_led_status_t bsp_led_stop(uint32_t led_number);

/**
 * @brief     This function for de-initial led
 * @param[in] led_number   initializes the specified LED number.
 * @return
 *                #BSP_LED_OK , if the operation completed successfully. \n
 *                #BSP_LED_INVALID_PARAMETER , if parameter is invalid. \n
 * @par       Example
 * @code 
 *        bsp_led_deinit(led_idx);
 * @endcode
 */
bsp_led_status_t bsp_led_deinit(uint32_t led_number);



/**
* @}
* @}
*/
#ifdef __cplusplus
}
#endif
#endif /* __BSP_LED_H__ */


