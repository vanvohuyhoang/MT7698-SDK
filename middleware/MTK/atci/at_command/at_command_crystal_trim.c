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

// For Register AT command handler
// System head file

#include "FreeRTOS.h"
#include "task.h"
#include <string.h>
#include <stdio.h>
#include "at_command.h"
#include "syslog.h"
#include <stdlib.h>

#if defined(ATCI_CMD_FOR_FACTORY_TEST) && defined(HAL_UART_MODULE_ENABLED)
#include "hal_gpt.h"
#include "hal_uart.h"
#include "hal_wdt.h"

/*Function Declare*/
extern hal_uart_status_t   hal_uart_ext_get_uart_config(hal_uart_port_t uart_port, hal_uart_config_t  *config);
extern bool                hal_uart_ext_is_dma_mode(hal_uart_port_t uart_port);
extern hal_uart_status_t   hal_uart_ext_set_baudrate(hal_uart_port_t uart_port, uint32_t baudrate);

atci_status_t              atci_cmd_hdlr_crystal_trim(atci_parse_cmd_param_t *parse_cmd);

/* Private variable declare  */
static  volatile    bool    flg_timeout   = false;
static  uint8_t             chr_pattern[] = {0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55};

/* AT command handler  */
void    cystal_trim_timer_callback(void *args)
{
    flg_timeout = true;
}


bool    send_pattern_char(uint8_t port, uint32_t baudrate, uint32_t timeout_s)
{
    hal_uart_config_t  config;
    uint32_t           handle;
    bool               wdt_en = false;
    printf("Port:%d,Baudrate:%d,timeout:%ds\r\n", (int)port, (int)baudrate, (int)timeout_s);
    if (HAL_GPT_STATUS_OK != hal_gpt_sw_get_timer(&handle) ) {
        return false;
    }
    if(HAL_GPT_STATUS_OK !=hal_gpt_sw_start_timer_ms(handle, timeout_s*1000, cystal_trim_timer_callback, NULL)){
        return false;
    }
    flg_timeout = false;
    if(hal_wdt_get_enable_status() == true) {
        hal_wdt_disable(HAL_WDT_DISABLE_MAGIC);
        wdt_en = true;
    }
    if(hal_uart_ext_get_uart_config(port, &config) != HAL_UART_STATUS_OK){
        config.baudrate = HAL_UART_BAUDRATE_38400;
        config.parity   = HAL_UART_PARITY_NONE;
        config.stop_bit = HAL_UART_STOP_BIT_1;
        config.word_length = HAL_UART_WORD_LENGTH_8;
        printf("uart %d not initialized!", port);
        hal_uart_init(port, &config);
        hal_uart_ext_set_baudrate(port, baudrate);
        while(flg_timeout == false){
            hal_uart_send_polling(port, (const uint8_t *)chr_pattern, sizeof(chr_pattern));
        }
        hal_uart_deinit(port);
    }else {
        printf("uart %d initialized!", port);
        hal_uart_ext_set_baudrate(port, baudrate);
        if(hal_uart_ext_is_dma_mode(port) == true){
            printf("uart %d in dma mode!", port);
            while(flg_timeout == false){
                hal_uart_send_dma(port, (const uint8_t *)chr_pattern, sizeof(chr_pattern));
            }
        } else {
            printf("uart %d in fifo mode!", port);
            while(flg_timeout == false){
                hal_uart_send_polling(port, chr_pattern, sizeof(chr_pattern));
            }
        }
        /*restore baudrate to previou*/
        hal_uart_set_baudrate(port, config.baudrate);
    }
    hal_gpt_sw_free_timer(handle);
    if(wdt_en == true) {
        hal_wdt_enable(HAL_WDT_ENABLE_MAGIC);
    }
    return true;
}

atci_status_t atci_cmd_hdlr_crystal_trim(atci_parse_cmd_param_t *parse_cmd)
{
    atci_response_t response = {{0}};
    //char            *param = NULL;
    //char            param_val;
    uint32_t        para[4],i;
    char            *ptr = NULL;
    //bool            arg_valid = false;
    char            *cmd_str;


    LOG_MSGID_I(common, "atci_cmd_hdlr_crystal_trim \r\n", 0);
    cmd_str = (char *)parse_cmd->string_ptr;
    response.response_flag = 0; /*    Command Execute Finish.  */
    #ifdef ATCI_APB_PROXY_ADAPTER_ENABLE
    response.cmd_id = parse_cmd->cmd_id;
    #endif

    switch (parse_cmd->mode) {
        case ATCI_CMD_MODE_TESTING:    /* rec: AT+EWDT=?   */
            strcpy((char *)response.response_buf, "+TRIM=(\"1: trigger wdt reset\")\r\nOK\r\n");
            response.response_flag |= ATCI_RESPONSE_FLAG_APPEND_OK;
            response.response_len = strlen((char *)response.response_buf);
            atci_send_response(&response);
            break;
        /*AT+TRIM=CRYSTAL,<uart_port>,<baudrate>,<running time>*/
        case ATCI_CMD_MODE_EXECUTION: /* rec: AT+TRIM=<op>  the handler need to parse the parameters  */
            if (strncmp(cmd_str, "AT+TRIM=CRYSTAL,", strlen("AT+TRIM=CRYSTAL,")) == 0)
            {
                ptr = strtok(parse_cmd->string_ptr,",");
                for(i=0; i<3; i++) {
                    ptr = strtok(NULL, ",");
                    if(ptr != NULL){
                        para[i] = atoi(ptr);
                    }
                    else {
                        goto error;
                    }
                }
                if(para[0] >= HAL_UART_MAX){
                    goto error;
                }
                if(send_pattern_char(para[0],para[1],para[2]) == true){
                    response.response_flag |= ATCI_RESPONSE_FLAG_APPEND_OK;
                } else {
                    response.response_flag |= ATCI_RESPONSE_FLAG_APPEND_ERROR;
                }
                response.response_len = strlen((char *)response.response_buf);
                atci_send_response(&response);
                return ATCI_STATUS_OK;
            }
error:
            strcpy((char *)response.response_buf, "Invalid Command.\r\n");
            response.response_flag |= ATCI_RESPONSE_FLAG_APPEND_ERROR;
            response.response_len = strlen((char *)response.response_buf);
            atci_send_response(&response);
            break;
        default :
            /* others are invalid command format */
            strcpy((char *)response.response_buf, "ERROR Command.\r\n");
            response.response_flag |= ATCI_RESPONSE_FLAG_APPEND_ERROR;
            response.response_len = strlen((char *)response.response_buf);
            atci_send_response(&response);
            break;
    }
    return ATCI_STATUS_OK;
}
#endif
