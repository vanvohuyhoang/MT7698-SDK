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

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "tftp_fludrive.h"

/* device.h includes */
#include "aw7698.h"

/* hal includes */
#include "hal.h"

#include "sys_init.h"
#include "task_def.h"
#include "diskio_sd.h"
#ifdef MTK_WIFI_ENABLE
#include "wifi_nvdm_config.h"
#include "wifi_lwip_helper.h"
#endif
#if defined(MTK_MINICLI_ENABLE)
#include "cli_def.h"
#endif

#include "bsp_gpio_ept_config.h"
#include "hal_sleep_manager.h"

#ifdef MTK_WIFI_ENABLE
#include "connsys_profile.h"
#include "wifi_api.h"

#ifdef MTK_BWCS_ENABLE
#include "bwcs_api.h"
#endif
#endif

#ifdef MTK_SYSTEM_HANG_TRACER_ENABLE
#include "systemhang_tracer.h"
#endif /* MTK_SYSTEM_HANG_TRACER_ENABLE */
#include "wifi_config.h"
#include "lwip_server.h"
#include "sdcard.h"

// #define WIFI_SSID              ("SQA_TEST_AP")
// #define WIFI_PASSWORD          ("77777777")

#define WIFI_SSID              ("SCOM 2.4Ghz")
#define WIFI_PASSWORD          ("trekvn2015")




#ifdef MTK_WIFI_ENABLE
int32_t wifi_station_port_secure_event_handler(wifi_event_t event, uint8_t *payload, uint32_t length);
int32_t wifi_scan_complete_handler(wifi_event_t event, uint8_t *payload, uint32_t length);
#endif


void vApplicationIdleHook(void)
{
#ifdef MTK_SYSTEM_HANG_TRACER_ENABLE
    systemhang_wdt_count = 0;
    hal_wdt_feed(HAL_WDT_FEED_MAGIC);
#endif /* MTK_SYSTEM_HANG_TRACER_ENABLE */
}

#ifdef MTK_WIFI_ENABLE
int32_t wifi_init_done_handler(wifi_event_t event,
                                      uint8_t *payload,
                                      uint32_t length)
{
    LOG_I(common, "WiFi Init Done: port = %d", payload[6]);
    return 1;
}

#ifdef MTK_USER_FAST_TX_ENABLE
#include "type_def.h"

#define DemoPktLen 64
extern UINT_8 DemoPkt[];

extern uint32_t g_FastTx_Channel;
extern PUINT_8 g_PktForSend;
extern UINT_32 g_PktLen;
static void fastTx_init(uint32_t channel, PUINT_8 pPktContent, UINT_32 PktLen)
{
    g_FastTx_Channel = channel;
    g_PktForSend = pPktContent;
    g_PktLen = PktLen;
}
#endif
#endif

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
#ifdef MTK_WIFI_ENABLE
#ifdef MTK_USER_FAST_TX_ENABLE
    /* Customize Packet Content and Length */
    fastTx_init(11, DemoPkt, DemoPktLen);
#endif
#endif

    /* Do system initialization, eg: hardware, nvdm, logging and random seed. */
    system_init();
    /* Call this function to indicate the system initialize done. */
    SysInitStatus_Set();

#ifdef MTK_SYSTEM_HANG_CHECK_ENABLE
#ifdef HAL_WDT_MODULE_ENABLED
    wdt_init();
#endif
#endif

    wifi_config(WIFI_SSID, WIFI_PASSWORD);
    //wifi_ap_config();
    
    if(sdcard_init() != 0){
        LOG_I(common, "mount sd card failure \r\n");
        return 0;
    }

    create_lwip_task();

    create_sdcard_task();
    
   /* Link the SD Card disk I/O driver */
   //FATFS_LinkDriver(&SD_Driver, SD_Path);

    //tftp_fludriver_init_server();
    //LOG_I(common, "Started tftp driver .... \r\n");

    vTaskStartScheduler();

    /* If all is well, the scheduler will now be running, and the following line
    will never be reached.  If the following line does execute, then there was
    insufficient FreeRTOS heap memory available for the idle and/or timer tasks
    to be created.  See the memory management section on the FreeRTOS web site
    for more details. */
    for (;;);
}

