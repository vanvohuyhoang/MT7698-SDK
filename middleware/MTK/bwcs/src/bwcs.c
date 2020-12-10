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

#include "bwcs.h"
#include "bwcs_wifi_interface.h"
#include "bwcs_bt_interface.h"
#include "wifi_os_api.h"
#include "os_util.h"


extern bwcs_status_t g_bwcs_status;
extern QueueHandle_t g_bwcs_queue;


/* proxy function implementation */
void bwcs_handler(void)
{
    bwcs_queue_t qBuf;
    while(1) {
        if(wifi_os_queue_receive(g_bwcs_queue, &qBuf, NULL) == 0 ) {
            if(qBuf.event_type == BWCS_EVENT_TYPE_WIFI) {
                switch(qBuf.event)
                {
                    case BWCS_WIFI_EVENT_OFF:
                    {
                        printf("BWCS event BWCS_WIFI_EVENT_OFF.\r\n");
                        g_bwcs_status.wifi_status.wifi_current_status = BWCS_WIFI_EVENT_OFF;
                        //disable PTA
                        if(g_bwcs_status.bt_status.bt_current_status == BWCS_BT_EVENT_ON) {
                            bwcs_config_bt_cmd(BWCS_BT_CMD_OFF);
                            //give the ANT to Wi-Fi if single ANT
                        }
                        break;
                    }
                    case BWCS_WIFI_EVENT_ON:
                    {
                        printf("BWCS event BWCS_WIFI_EVENT_ON.\r\n");
                        g_bwcs_status.wifi_status.wifi_current_status = BWCS_WIFI_EVENT_ON;
                        if(g_bwcs_status.bt_status.bt_current_status == BWCS_BT_EVENT_ON && g_bwcs_status.pta_cm_mode == PTA_CM_MODE_TDD) {
                            //enable PTA
                            bwcs_config_wifi_cmd(BWCS_WIFI_CMD_ON);
                            bwcs_config_bt_cmd(BWCS_BT_CMD_ON);
                        }
                        break;
                    }
                    case BWCS_WIFI_EVENT_CONNECTING:
                    {
                        printf("BWCS event BWCS_WIFI_EVENT_CONNECTING.\r\n");
                        //do privilege process
                        if(g_bwcs_status.wifi_status.wifi_current_status == BWCS_WIFI_EVENT_CONNECTING) {
                            break;
                        }else {
                            g_bwcs_status.wifi_status.wifi_current_status = BWCS_WIFI_EVENT_CONNECTING;
                            bwcs_config_bt_cmd(BWCS_BT_CMD_ENTER_PRIVILEGE);
                            break;
                        }
                    }
                    case BWCS_WIFI_EVENT_CONNECTED:
                    {
                        printf("BWCS event BWCS_WIFI_EVENT_CONNECTED.\r\n");
                        //exit privilege process
                        if(g_bwcs_status.wifi_status.wifi_current_status == BWCS_WIFI_EVENT_CONNECTED) {
                            break;
                        }else {
                            g_bwcs_status.wifi_status.wifi_current_status = BWCS_WIFI_EVENT_CONNECTED;
                            bwcs_config_bt_cmd(BWCS_BT_CMD_EXIT_PRIVILEGE);
                            break;
                        }
                    }
                    case BWCS_WIFI_EVENT_CH_UPDATE:
                    {
                        printf("BWCS event BWCS_WIFI_EVENT_CH_UPDATE: %d.\r\n",qBuf.payload[0]);
                        g_bwcs_status.wifi_status.wifi_channel = qBuf.payload[0];
                        //compare the channel of BT
                        //switch TDD/FDD, enable/disable PTA
                        break;
                    }
                }
            }else if(qBuf.event_type == BWCS_EVENT_TYPE_BT) {
                switch(qBuf.event)
                {
                    case BWCS_BT_EVENT_OFF:
                    {
                        printf("BWCS event BWCS_BT_EVENT_OFF.\r\n");
                        g_bwcs_status.bt_status.bt_current_status = BWCS_BT_EVENT_OFF;
                        //disable PTA
                        if(g_bwcs_status.wifi_status.wifi_current_status == BWCS_WIFI_EVENT_ON) {
                            bwcs_config_wifi_cmd(BWCS_WIFI_CMD_OFF);
                            //give the ANT to Wi-Fi if single ANT
                        }
                        break;
                    }
                    case BWCS_BT_EVENT_ON:
                    {
                        printf("BWCS event BWCS_BT_EVENT_ON.\r\n");
                        g_bwcs_status.bt_status.bt_current_status = BWCS_BT_EVENT_ON;
                        if(g_bwcs_status.wifi_status.wifi_current_status == BWCS_WIFI_EVENT_ON && g_bwcs_status.pta_cm_mode == PTA_CM_MODE_TDD) {
                        //enable PTA
                        bwcs_config_wifi_cmd(BWCS_WIFI_CMD_ON);
                        bwcs_config_bt_cmd(BWCS_BT_CMD_ON);
                        }
                        break;
                    }
                    case BWCS_BT_EVENT_CH_UPDATE:
                    {
                        printf("BWCS event BWCS_BT_EVENT_CH_UPDATE.\r\n");
                        //os_memcpy(g_bwcs_status.bt_status.bt_channel[0],qBuf.payload[0],sizeof(g_bwcs_status.bt_status.bt_channel));
                        //compare the channel of Wi-Fi
                        //switch TDD/FDD
                        break;
                    }
                }
            }else {
                printf("unkown event type.\r\n");
            }
        }
    }
    return;
}
