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

// For Register AT command handler
#include "at_command.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(MTK_LINEIN_PLAYBACK_ENABLE)
#include "hal_audio_cm4_dsp_message.h"
#include "linein_playback.h"
#endif

#include "hal_audio.h"

#ifdef MTK_SMT_AUDIO_TEST
    #include "at_command_audio_ata_test.h"
    #include "Audio_FFT.h"
    #include "hal_nvic.h"

    #define SMT_DROP_CNT 60
    #define SMT_SAVE 61
    #define SMT_UL_CNT_LIMIT 65

#endif
#if !defined(MTK_AUDIO_AT_COMMAND_DISABLE) && defined(HAL_AUDIO_MODULE_ENABLED)
    #include "hal_log.h"
#if ((PRODUCT_VERSION == 1552) || defined(AM255X))
    #include "bt_sink_srv_ami.h"
    #include "bt_sink_srv_common.h"
    #if defined(MTK_PROMPT_SOUND_ENABLE)
      #include "prompt_control.h"
      #include "hal_audio_cm4_dsp_message.h"
    #endif
    #ifdef MTK_ANC_ENABLE
      #include "anc_control.h"
    #endif
    #ifdef MTK_PEQ_ENABLE
      #include "race_cmd_dsprealtime.h"
    #endif
    #ifdef MTK_RECORD_ENABLE
      #include "record_control.h"
    #endif
    #if defined(MTK_LINEIN_PLAYBACK_ENABLE)
      #include "audio_sink_srv_line_in.h"
    #endif
    #include "hal_audio_internal.h"
    #include "hal_dvfs_internal.h"
    #include "hal_gpio.h"
#endif

  #if !defined(MTK_AVM_DIRECT)
    #include "hal_audio_test.h"
    #include "hal_audio_enhancement.h"

    #ifdef MTK_EXTERNAL_DSP_ENABLE
    #include "external_dsp_application.h"
    #include "external_dsp_driver.h"

    #if defined (MTK_NDVC_ENABLE)
    extern uint16_t spe_ndvc_uplink_noise_index_map(uint16_t db);
    bool ndvc_at_test = false;
    #endif /*MTK_NDVC_ENABLE*/

    #endif /*MTK_EXTERNAL_DSP_ENABLE*/

    #if defined(MTK_BT_AWS_ENABLE)
    #include "hal_audio_internal_service.h"
    #endif  /* defined(MTK_BT_AWS_ENABLE)) */

  #if defined(MTK_BT_HFP_CODEC_DEBUG)
  #include "bt_hfp_codec_internal.h"
typedef enum {
    AT_AUDIO_HFP_SAVE_OR_PRINT_METHOD_WRITE_TO_FILE = 0,
    AT_AUDIO_HFP_SAVE_OR_PRINT_METHOD_PRINT_TO_USB_DEBUG_PORT = 1,
} at_audio_hfp_save_or_print_method_t;
  #endif
  #endif /*!defined(MTK_AVM_DIRECT)*/

log_create_module(atcmd_aud, PRINT_LEVEL_INFO);

#define LOGMSGIDE(fmt,arg...)   LOG_MSGID_E(atcmd_aud, "ATCMD_AUD: "fmt,##arg)
#define LOGMSGIDW(fmt,arg...)   LOG_MSGID_W(atcmd_aud, "ATCMD_AUD: "fmt,##arg)
#define LOGMSGIDI(fmt,arg...)   LOG_MSGID_I(atcmd_aud ,"ATCMD_AUD: "fmt,##arg)


#if defined(MTK_PROMPT_SOUND_ENABLE)
bool g_app_voice_prompt_test_off = false;
#endif

#ifdef MTK_PEQ_ENABLE
static uint8_t g_peq_test_coef[] = { //500hz low pass
0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x0D, 0x00, 0x01, 0x00, 0x22, 0x00, 0x00, 0x4F, 0x22, 0x00,
0x00, 0x4F, 0x22, 0x00, 0x00, 0x4F, 0x8E, 0x82, 0x00, 0x8D, 0x86, 0x7B, 0x00, 0x37, 0x4D, 0x02,
0x00, 0xEF, 0x02, 0x00, 0x0D, 0x00, 0x01, 0x00, 0x1D, 0x00, 0x00, 0x01, 0x1D, 0x00, 0x00, 0x01,
0x1D, 0x00, 0x00, 0x01, 0x54, 0x82, 0x00, 0x31, 0xE1, 0x7B, 0x00, 0xAD, 0x4D, 0x02, 0x00, 0xEF, };
#endif


#if ((PRODUCT_VERSION == 1552) || defined(AM255X))
extern HAL_DSP_PARA_AU_AFE_CTRL_t audio_nvdm_HW_config;
extern void ami_hal_audio_status_set_running_flag(audio_message_type_t type, bool is_running);
#ifdef MTK_RECORD_ENABLE
bool record_flag = false;
int16_t g_record_Rdemo_data[128];
#define RECORD_OPUS_DATA_MAX_SIZE_U8    (sizeof(uint8_t) * 80)*4//ENCODER_BITRATE_32KBPS = 80 BYtes,record.frames_per_message = 4
#define RECORD_DATA_MAX_SIZE_U8         (sizeof(int16_t) * 128)
uint16_t g_reocrd_id = 0;
extern uint16_t g_stream_in_sample_rate;
extern uint16_t g_dump;
extern uint16_t g_stream_in_code_type;



void record_control_CCNI_demo_callback(hal_audio_event_t event, void *data)
{
    switch(event){
        case HAL_AUDIO_EVENT_ERROR:
            LOGMSGIDI("[AT][RECORD][CALLBACK]HAL_AUDIO_EVENT_ERROR\r\n", 0);
            break;

        case HAL_AUDIO_EVENT_DATA_NOTIFICATION:
        if(g_stream_in_code_type == AUDIO_DSP_CODEC_TYPE_PCM){
            for(uint16_t i=0; i < 8 ; i++)
            {
                if (RECORD_CONTROL_EXECUTION_SUCCESS == audio_record_control_read_data(&g_record_Rdemo_data, RECORD_DATA_MAX_SIZE_U8)){
                    //LOG_AUDIO_DUMP(g_record_Rdemo_data, sizeof(int16_t) * 128, VOICE_TX_MIC_3);
                }else {
                    LOGMSGIDI("[AT][RECORD][CALLBACK]read stream in failed\r\n", 0);
                }
            }
        }
        if(g_stream_in_code_type == AUDIO_DSP_CODEC_TYPE_OPUS){
            if (RECORD_CONTROL_EXECUTION_SUCCESS == audio_record_control_read_data(&g_record_Rdemo_data, RECORD_OPUS_DATA_MAX_SIZE_U8)){
                    //LOG_AUDIO_DUMP(g_record_Rdemo_data, sizeof(int16_t) * 128, VOICE_TX_MIC_3);
            }else {
                LOGMSGIDI("[AT][RECORD][CALLBACK]read stream in failed\r\n", 0);
            }
        }

        break;
    }

}

void record_control_AM_notify_callback(bt_sink_srv_am_id_t aud_id, bt_sink_srv_am_cb_msg_class_t msg_id, bt_sink_srv_am_cb_sub_msg_t sub_msg, void *parm)
{
    printf("[AT][RECORD]AM_CB, aud_id:%x, msg_id:%x, sub_msg:%x", aud_id, msg_id, sub_msg);
    if (msg_id == AUD_SINK_OPEN_CODEC) {
        if(sub_msg == AUD_CMD_COMPLETE){
            //Start Record Success
        } else if (sub_msg == AUD_CMD_FAILURE) {
            //Start Record Fail
        } else {
            //[error]
        }
    }else if(AUD_SELF_CMD_REQ){
        if(sub_msg == AUD_CMD_COMPLETE){
            //Stop Record Request Success
        } else if (sub_msg == AUD_CMD_FAILURE) {
            //Start Record Request Fail because HFP exist. /*Reject Request.*/
        } else {
            //[error]
        }
    }else if(AUD_SUSPEND_BY_IND){
        if(sub_msg == AUD_SUSPEND_BY_HFP){
            //Suspend record because HFP interrupt.
        } else {
            //[error]
        }
    }else{
        //[error]
    }
}

#endif

#if defined(MTK_PROMPT_SOUND_ENABLE)
#if defined(MTK_AUDIO_AT_CMD_PROMPT_SOUND_ENABLE)
void at_voice_prompt_callback(prompt_control_event_t event_id)
{
    if (event_id == PROMPT_CONTROL_MEDIA_END) {
        LOGMSGIDI("At_command voice prompt stop callback.", 0);
    }
}
#endif
#endif
#endif

// AT command handler
atci_status_t atci_cmd_hdlr_audio(atci_parse_cmd_param_t *parse_cmd)
{
#if defined(__GNUC__)
    atci_response_t *presponse = NULL;
    presponse = (atci_response_t *)pvPortMalloc(sizeof(atci_response_t));
    if (presponse == NULL) {
        LOGMSGIDE("memory malloced failed.\r\n", 0);
        return ATCI_STATUS_ERROR;
    }

    LOGMSGIDI("atci_cmd_hdlr_audio \r\n", 0);

    memset(presponse, 0, sizeof(atci_response_t));
    presponse->response_flag = 0; // Command Execute Finish.

    switch (parse_cmd->mode) {
        case ATCI_CMD_MODE_TESTING:    // rec: AT+ECMP=?
            strcpy((char * restrict)presponse->response_buf, "+EAUDIO =<op>[,<param>]\r\nOK\r\n");
            presponse->response_len = strlen((const char *)presponse->response_buf);
            atci_send_response(presponse);
            break;
        case ATCI_CMD_MODE_EXECUTION: // rec: AT+EAUDIO=<op>  the handler need to parse the parameters
            if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=DUMMY") != NULL) {
                LOGMSGIDI("This is a summy command in order to have \"if\" description for if loop\r\n", 0);
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);

            }
#ifdef MTK_RECORD_ENABLE
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=RECORD") != NULL) {
                LOGMSGIDI("[AT][RECORD]Record start.", 0);
                record_flag = !record_flag;
                LOGMSGIDI("[AT][RECORD]record_flag:%d\r\n", 1, record_flag);
                if(g_reocrd_id == 0){
                #ifdef MTK_RECORD_OPUS_ENABLE
                    record_encoder_cability_t encoder_capability;
                    encoder_capability.codec_type = AUDIO_DSP_CODEC_TYPE_OPUS;
                    encoder_capability.bit_rate = ENCODER_BITRATE_32KBPS;
                    g_reocrd_id = audio_record_control_enabling_encoder_init(record_control_CCNI_demo_callback, NULL, record_control_AM_notify_callback,&encoder_capability);
                #else
                    g_reocrd_id = audio_record_control_init(record_control_CCNI_demo_callback, NULL, record_control_AM_notify_callback);
                #endif
                }
                LOGMSGIDI("[AT][RECORD]g_reocrd_id %x", 1, g_reocrd_id);

                if(record_flag){
                    g_dump = true;
				    audio_record_control_start(g_reocrd_id);
                }else{
				    audio_record_control_stop(g_reocrd_id);
                    g_dump = false;
                }

                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=REC_SET_SR") != NULL) {
                char *chr = NULL;
                chr= strchr(parse_cmd->string_ptr, ',');
                if(chr == NULL){
                    LOGMSGIDE("Set REC sample rate NULL error.", 0);
                }
                chr++;
                if(strstr((char *)chr, "16K") != NULL){
                    g_stream_in_sample_rate = 16000;
                    LOGMSGIDI("[AT][RECORD]Set REC sample rate: %d.", 1, g_stream_in_sample_rate);
                }
                else if(strstr((char *)chr, "32K") != NULL){
                    g_stream_in_sample_rate = 32000;
                    LOGMSGIDI("[AT][RECORD]Set REC sample rate: %d.", 1, g_stream_in_sample_rate);
                }
                else if(strstr((char *)chr, "48K") != NULL){
                    g_stream_in_sample_rate = 48000;
                    LOGMSGIDI("[AT][RECORD]Set REC sample rate: %d.", 1, g_stream_in_sample_rate);
                }
                else {
                    LOGMSGIDE("[AT][RECORD]Set REC sample rate error.", 0);
                }

                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }

#endif
    #ifdef MTK_DSP_SHUTDOWN_SPECIAL_CONTROL_ENABLE
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=AUD_SET_DSP_OFF") != NULL) {
                LOGMSGIDI("DSP OFF.", 0);
                hal_audio_dsp_controller_send_message(MSG_MCU2DSP_COMMON_DUMMY_DSP_SHUTDOWN, 0, 0, true);
                hal_audio_deinit();
                LOGMSGIDI("HAL Deinit end.", 0);
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=AUD_SET_DSP_ON") != NULL) {
                LOGMSGIDI("DSP ON.", 0);
                hal_audio_init();
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
    #endif
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=AUD_SET_DEVICE_LEFT") != NULL) {
                LOGMSGIDI("Set device role: LEFT.", 0);
                if(AUD_EXECUTION_SUCCESS != ami_set_audio_channel(AUDIO_CHANNEL_NONE, AUDIO_CHANNEL_L, REWRITE)){
                    LOGMSGIDE("Set L_Channel error.", 0);
                }
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=AUD_SET_DEVICE_RIGHT") != NULL) {
                LOGMSGIDI("Set device role: RIGHT.", 0);
                if(AUD_EXECUTION_SUCCESS != ami_set_audio_channel(AUDIO_CHANNEL_NONE, AUDIO_CHANNEL_R, REWRITE)){
                    LOGMSGIDE("Set R_Channel error.", 0);
                }
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=AUD_SET_MIC_CHANNEL,") != NULL) {
                char *chr = NULL;
                chr= strchr(parse_cmd->string_ptr, ',');
                if(chr == NULL){
                    LOGMSGIDE("Set MIC_Channel NULL error.", 0);
                }
                chr++;
                if(strstr((char *)chr, "L") != NULL){
                    LOGMSGIDI("Set MIC channel: LEFT.", 0);
                    if(AUD_EXECUTION_SUCCESS != ami_set_audio_channel(AUDIO_CHANNEL_NONE, AUDIO_CHANNEL_L, REWRITE)){
                        LOGMSGIDE("Set L_Channel error.", 0);
                    }
                    if(AUD_EXECUTION_SUCCESS != ami_set_audio_device( STREAM_IN, AU_DSP_VOICE, HAL_AUDIO_DEVICE_MAIN_MIC_L, HAL_AUDIO_INTERFACE_1, REWRITE)){
                        LOGMSGIDE("Set MAIN_MIC_L error\r\n", 0);
                    }
                } else if(strstr((char *)chr, "R") != NULL){
                    LOGMSGIDI("Set MIC channel: RIGHT.", 0);
                    if(AUD_EXECUTION_SUCCESS != ami_set_audio_channel(AUDIO_CHANNEL_SWAP, AUDIO_CHANNEL_R, REWRITE)){
                        LOGMSGIDE("Set R_Channel error.", 0);
                    }
                    if(AUD_EXECUTION_SUCCESS != ami_set_audio_device( STREAM_IN, AU_DSP_VOICE, HAL_AUDIO_DEVICE_MAIN_MIC_R, HAL_AUDIO_INTERFACE_1, REWRITE)){
                        LOGMSGIDE("Set MAIN_MIC_R error\r\n", 0);
                    }
                } else {
                    LOGMSGIDE("SET_MIC_CHANNEL error", 0);
                }
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=AUD_SET_SIDETONE,") != NULL) {
                char *chr = NULL;
                uint32_t HFP_gain;
                nvdm_status_t result;
                chr= strchr(parse_cmd->string_ptr, ',');
                if(chr == NULL){
                    LOGMSGIDE("Set SET_SIDETONE NULL error.", 0);
                }
                chr++;
                if(strstr((char *)chr, "ENABLE") != NULL){
                    LOGMSGIDI("Set SET_SIDETONE: ENABLE.", 0);
                    chr= strchr(chr, ',');
                    chr++;
                    sscanf(chr, "%lu", &HFP_gain);
                    // HFP side tone Enable
                    audio_nvdm_HW_config.Voice_Sidetone_EN = 0x01;
                    audio_nvdm_HW_config.Voice_Sidetone_Gain = (uint8_t)((int32_t)HFP_gain/100);
                    result = nvdm_write_data_item("AB15", HAL_NVDM_ID_HW_TABLE_STRING, NVDM_DATA_ITEM_TYPE_RAW_DATA, (const uint8_t *)&audio_nvdm_HW_config, sizeof(audio_nvdm_HW_config));
                    if(result){
                        LOGMSGIDI("Audio Set SET_SIDETONE NVDM write error", 0);
                    }
                } else if(strstr((char *)chr, "DISABLE") != NULL){
                    LOGMSGIDI("Set SET_SIDETONE: DISABLE.", 0);
                    // HFP side tone disable
                    audio_nvdm_HW_config.Voice_Sidetone_EN = 0x00;
                    result = nvdm_write_data_item("AB15", HAL_NVDM_ID_HW_TABLE_STRING, NVDM_DATA_ITEM_TYPE_RAW_DATA, (const uint8_t *)&audio_nvdm_HW_config, sizeof(audio_nvdm_HW_config));
                    if(result){
                        LOGMSGIDI("Audio Set SET_SIDETONE NVDM write error", 0);
                    }
                } else {
                    LOGMSGIDE("SET_SIDETONE error", 0);
                }
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=AUD_RG_DUMP") != NULL) {
                uint16_t i;
                uint32_t RG_INDEX = 0x70000000;
                for(i = 205;i>0;i--){
                    LOGMSGIDI("[RG dump index(0x%x)](0x%x),(0x%x),(0x%x),(0x%x),(0x%x),(0x%x),(0x%x),(0x%x),(0x%x),(0x%x)", 11, RG_INDEX,
                        *(volatile uint32_t *)(RG_INDEX),      *(volatile uint32_t *)(RG_INDEX + 1) , *(volatile uint32_t *)(RG_INDEX + 2) , *(volatile uint32_t *)(RG_INDEX + 3) , *(volatile uint32_t *)(RG_INDEX + 4),
                        *(volatile uint32_t *)(RG_INDEX + 5) , *(volatile uint32_t *)(RG_INDEX + 6) , *(volatile uint32_t *)(RG_INDEX + 7) , *(volatile uint32_t *)(RG_INDEX + 8) , *(volatile uint32_t *)(RG_INDEX + 9));
                    LOGMSGIDI("[RG dump index(0x%x)](0x%x),(0x%x),(0x%x),(0x%x),(0x%x),(0x%x),(0x%x),(0x%x),(0x%x),(0x%x)", 11, RG_INDEX + 10,
                        *(volatile uint32_t *)(RG_INDEX + 10), *(volatile uint32_t *)(RG_INDEX + 11), *(volatile uint32_t *)(RG_INDEX + 12), *(volatile uint32_t *)(RG_INDEX + 13), *(volatile uint32_t *)(RG_INDEX + 14),
                        *(volatile uint32_t *)(RG_INDEX + 15), *(volatile uint32_t *)(RG_INDEX + 16), *(volatile uint32_t *)(RG_INDEX + 17), *(volatile uint32_t *)(RG_INDEX + 18), *(volatile uint32_t *)(RG_INDEX + 19));
                    RG_INDEX += 20;
                    hal_gpt_delay_ms(2);
                }
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
#if ((PRODUCT_VERSION == 1552) || defined(AM255X))
    #if MTK_SMT_AUDIO_TEST
            else if (strstr((char*)parse_cmd->string_ptr, "AT+EAUDIO=SPEAKER")) {
                char *pch = NULL;
                char *param[3] = {NULL};
                pch = strtok(parse_cmd->string_ptr,"=");
                if (!pch) {
                    sprintf((char *)presponse->response_buf,"parameter error\r\n");
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_ERROR;
                    goto end ;
                }
                char **idx = param;
                uint32_t cnt = 0;
                for (;cnt<3;cnt++) {
                    pch = strtok(NULL,",");
                    *(idx++) = pch;
                }
                char *active = param[2];
                if (strstr(active,"Play_start")) {
                    switch(*param[1]) {
                    case 'L':
                        audio_smt_test_pure_on_off(true,SMT_CH_LEFT);
                        break;
                    case 'R':
                        audio_smt_test_pure_on_off(true,SMT_CH_RIGHT);
                        break;
                    }
                    hal_gpt_delay_ms(100);
                } else if(strstr(active,"Play_stop")) {
                    switch(*param[1]) {
                    case 'L':
                        audio_smt_test_pure_on_off(false,SMT_CH_LEFT);
                        break;
                    case 'R':
                        audio_smt_test_pure_on_off(false,SMT_CH_RIGHT);
                        break;
                    }
                    hal_gpt_delay_ms(100);
                } else {
                    LOGMSGIDE("AT+EAUDIO=SPEAKER,unvalid cmd\r\n", 0);
                }
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                end:
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }

            else if((strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=AMIC-ACC,L,1KTone_Ckeck") != NULL) ||
                    (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=AMIC-ACC,R,1KTone_Ckeck") != NULL) ||
                    (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=AMIC-DCC,L,1KTone_Ckeck") != NULL) ||
                    (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=AMIC-DCC,R,1KTone_Ckeck") != NULL) ||
                    (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=DMIC,L,1KTone_Ckeck") != NULL) ||
                    (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=DMIC,R,1KTone_Ckeck") != NULL))
            {
                char *pch = NULL;
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_ERROR;
                char *param[3] = {NULL};
                char **idx = param;
                int i = 0;
                uint32_t Atcmd_value = *((volatile uint32_t*)(0xA2120B04));
                static fft_buf_t *fft_bufs = NULL;

                if (fft_bufs == NULL) {
                    LOGMSGIDI("ATA loopback create fft_bufs\r\n", 0);
                    fft_bufs = (fft_buf_t *)pvPortMalloc(sizeof(fft_buf_t));
                }
                if (fft_bufs != NULL) {
                    hal_audio_init_stream_buf(fft_bufs);

                    if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=AMIC") != NULL) {
                        ami_set_audio_device(STREAM_IN, AU_DSP_VOICE, HAL_AUDIO_DEVICE_MAIN_MIC_DUAL, HAL_AUDIO_INTERFACE_1, NOT_REWRITE);
                        if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=AMIC-ACC") != NULL) {
                            *((volatile uint32_t*)(0xA2120B04)) |= 0x10;
                        }
                    }

                    if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=DMIC") != NULL) {
                        ami_set_audio_device(STREAM_IN, AU_DSP_VOICE, HAL_AUDIO_DEVICE_DIGITAL_MIC_DUAL, HAL_AUDIO_INTERFACE_1, NOT_REWRITE);
                    }

                    pch = strtok(parse_cmd->string_ptr, "=");
                    while (pch != NULL) {
                        for (i = 0; i < 2; i++) {
                            pch = strtok(NULL,",");
                            *(idx++) = pch;
                        }
                    }

                    if (strchr(param[1], 'L')) {
                        ami_set_audio_channel(AUDIO_CHANNEL_L, AUDIO_CHANNEL_NONE, NOT_REWRITE);
                    } else {
                        ami_set_audio_channel(AUDIO_CHANNEL_R, AUDIO_CHANNEL_NONE, NOT_REWRITE);
                    }

                    KTONE_DL_ON;
                    hal_audio_start_stream_in(HAL_AUDIO_RECORD_VOICE);
                    i=0;
                    while (1) {
                        if (i < (SMT_DROP_CNT)) {
                            hal_audio_read_stream_in(fft_bufs->cpyIdx, FFT_BUFFER_SIZE); //drop
                        } else if(i < SMT_SAVE) {
                            fft_bufs->cpyIdx = fft_bufs->cpyIdx + (FFT_BUFFER_SIZE >> 1);
                        } else if(i < SMT_UL_CNT_LIMIT) {
                            if (HAL_AUDIO_STATUS_OK == hal_audio_read_stream_in(fft_bufs->cpyIdx, FFT_BUFFER_SIZE)){
                                break;
                            }
                        } else {
                            break;
                        }
                        hal_gpt_delay_ms(5);
                        i++;
                    }

                    hal_audio_stop_stream_in();
                    KTONE_DL_OFF;

                    ApplyFFT256(fft_bufs->bitstream_buf, 0, &fft_bufs->u4Freq_data, &fft_bufs->u4Mag_data, 16000);
                    if (FreqCheck(1000, fft_bufs->u4Freq_data)) {
                       presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                    }
                    if (fft_bufs->u4Mag_data < 5000000) {
                       presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_ERROR;
                    }
                    sprintf((char *)presponse->response_buf,"Freq=%d, Mag=%d, ",fft_bufs->u4Freq_data,fft_bufs->u4Mag_data);
                    *((volatile uint32_t*)(0xA2120B04)) = Atcmd_value;
                }
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }

            else if((strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=PASS_THROUGH_TEST_MUTE") != NULL)) {
                PTT_u4Freq_Mag_data result = {0, 0};
                pass_through_test_mic_side mic_side = PTT_L;
                char *chr = NULL;
                chr= strchr(parse_cmd->string_ptr, ',');
                if(chr == NULL){
                    LOGMSGIDE("Set REC sample rate NULL error.", 0);
                }
                chr++;
                if(strstr((char *)chr, "L") != NULL){
                    LOGMSGIDI("[AT][PASS_THROUGH_TEST_MUTE]L mic", 0);
                    mic_side = PTT_L;
                }
                else if(strstr((char *)chr, "R") != NULL){
                    LOGMSGIDI("[AT][PASS_THROUGH_TEST_MUTE]R mic", 0);
                    mic_side = PTT_R;
                }
                else{
                    LOGMSGIDI("[AT][PASS_THROUGH_TEST_MUTE]error setting, default mic L", 0);
                }

                if(pass_through_test(PTT_AMIC_DCC, mic_side, PTT_MUTE, &result) == PTT_SUCCESS) {
                    sprintf((char *)presponse->response_buf,"PASS_THROUGH_TEST_MUTE, Freq=%d, Mag=%d, db=%lf\r\n", result.freq_data, result.mag_data, result.db_data);
                    presponse->response_len = strlen((const char *)presponse->response_buf);
                    atci_send_response(presponse);
                }
            }

            else if((strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=PASS_THROUGH_TEST") != NULL)) {
                PTT_u4Freq_Mag_data result = {0, 0};
                uint32_t PTT_test_cnt = 5;
                uint32_t u4FreqArray[] = {PTT_500HZ, PTT_1000HZ, PTT_2000HZ, PTT_4000HZ, PTT_6000HZ};
                pass_through_test_mic_side mic_side = PTT_L;
                char *chr = NULL;
                chr= strchr(parse_cmd->string_ptr, ',');
                if(chr == NULL){
                    LOGMSGIDE("Set REC sample rate NULL error.", 0);
                }
                chr++;
                if(strstr((char *)chr, "L") != NULL){
                    LOGMSGIDI("[AT][PASS_THROUGH_TEST]L mic", 0);
                    mic_side = PTT_L;
                }
                else if(strstr((char *)chr, "R") != NULL){
                    LOGMSGIDI("[AT][PASS_THROUGH_TEST]R mic", 0);
                    mic_side = PTT_R;
                }
                else{
                    LOGMSGIDI("[AT][PASS_THROUGH_TEST]error setting, default mic L", 0);
                }

                for(int i=0; i<PTT_test_cnt; i++){
                    if(pass_through_test(PTT_AMIC_DCC, mic_side, u4FreqArray[i], &result) == PTT_SUCCESS) {
                        sprintf((char *)presponse->response_buf,"PASS_THROUGH_TEST, Freq=%d, Mag=%d, db=%lf\r\n", result.freq_data, result.mag_data, result.db_data);
                        presponse->response_len = strlen((const char *)presponse->response_buf);
                        atci_send_response(presponse);
                    }
                }
            }

    #endif

            #if defined(MTK_BUILD_SMT_LOAD)
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=AUD_SMT_ON") != NULL) {
                LOGMSGIDI("here", 0);

                audio_smt_test(1);
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=AUD_SMT_OFF") != NULL) {

                audio_smt_test(0);
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            #endif
            #if defined(MTK_PROMPT_SOUND_ENABLE)
	 else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=VP_TEST_OFF") != NULL){
	     g_app_voice_prompt_test_off = true;
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
	 }
           else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=VP_TEST_ON") != NULL){
	     g_app_voice_prompt_test_off = false;
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
	 }
            #if defined(MTK_AUDIO_AT_CMD_PROMPT_SOUND_ENABLE)
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=AUD_VP_MP3") != NULL) {
                #if 0
                char* config_s = NULL;
                unsigned int config = 0;
                config_s = strchr(parse_cmd->string_ptr, ',');
                config_s++;
                sscanf(config_s, "%d", &config);
                #endif
                uint8_t *tone_buf = NULL;
                uint32_t tone_size = 0;
                static const uint8_t voice_prompt_mix_mp3_tone_48k[] = {
                    #include "48k.mp3.hex"
                };
                tone_buf = (uint8_t *)voice_prompt_mix_mp3_tone_48k;
                tone_size = sizeof(voice_prompt_mix_mp3_tone_48k);
                #ifndef MTK_MP3_TASK_DEDICATE
                prompt_control_play_tone(VPC_MP3, tone_buf, tone_size, at_voice_prompt_callback);
                #else
                prompt_control_play_sync_tone(VPC_MP3, tone_buf, tone_size, 0, at_voice_prompt_callback);
                #endif
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
                }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=AUD_VP_TRIGGER") != NULL) {
                hal_audio_dsp_controller_send_message(MSG_MCU2DSP_PROMPT_AWS_SYNC_TRIGGER, 0, 0, false);
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=AUD_VP_CONNECTED_MP3") != NULL) {
                uint8_t *tone_buf = NULL;
                uint32_t tone_size = 0;
                static const uint8_t voice_prompt_mix_mp3_tone_connected[] = {
                    #include "connected_MADPCM.mp3.hex"
                };
                tone_buf = (uint8_t *)voice_prompt_mix_mp3_tone_connected;
                tone_size = sizeof(voice_prompt_mix_mp3_tone_connected);
                #ifndef MTK_MP3_TASK_DEDICATE
                prompt_control_play_tone(VPC_MP3, tone_buf, tone_size, at_voice_prompt_callback);
                #else
                prompt_control_play_sync_tone(VPC_MP3, tone_buf, tone_size, 0, at_voice_prompt_callback);
                #endif
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=AUD_VP_LONG_MP3") != NULL) {
                uint8_t *tone_buf = NULL;
                uint32_t tone_size = 0;
                static const uint8_t voice_prompt_mix_mp3_tone_long[] = {
                    #include "48k.mp3.long.hex"
                };
                tone_buf = (uint8_t *)voice_prompt_mix_mp3_tone_long;
                tone_size = sizeof(voice_prompt_mix_mp3_tone_long);
                #ifndef MTK_MP3_TASK_DEDICATE
                prompt_control_play_tone(VPC_MP3, tone_buf, tone_size, at_voice_prompt_callback);
                #else
                prompt_control_play_sync_tone(VPC_MP3, tone_buf, tone_size, 0, at_voice_prompt_callback);
                #endif
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=AUD_CHECK_GAIN") != NULL) {
                LOGMSGIDI("Gain Check: A(0x%x), G1(0x%x), G2(0x%x)", 3, *(volatile uint32_t *)0x70000f58, *(volatile uint32_t *)0x70000424, *(volatile uint32_t *)0x7000043C);
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            #endif /* MTK_AUDIO_AT_CMD_PROMPT_SOUND_ENABLE */
            #endif /* MTK_PROMPT_SOUND_ENABLE */
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=VOL_STREAM_OUT") != NULL) {
                char* config_s = NULL;
                unsigned int data32;
                unsigned int digital_volume_index, analog_volume_index;

                config_s = strchr((char *)parse_cmd->string_ptr, ',');
                config_s++;
                sscanf(config_s, "%d", &digital_volume_index);

                config_s = strchr(config_s, ',');
                config_s++;
                sscanf(config_s, "%d", &analog_volume_index);
                data32 = (analog_volume_index<<16) | (digital_volume_index & 0xFFFF);
                hal_audio_dsp_controller_send_message(MSG_MCU2DSP_COMMON_SET_OUTPUT_DEVICE_VOLUME, 0, data32, false);
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=VOL_STREAM_IN") != NULL) {
                char* config_s = NULL;
                unsigned int data32;
                unsigned int digital_volume_index, analog_volume_index;

                config_s = strchr((char *)parse_cmd->string_ptr, ',');
                config_s++;
                sscanf(config_s, "%d", &digital_volume_index);

                config_s = strchr(config_s, ',');
                config_s++;
                sscanf(config_s, "%d", &analog_volume_index);
                data32 = (analog_volume_index<<16) | (digital_volume_index & 0xFFFF);
                hal_audio_dsp_controller_send_message(MSG_MCU2DSP_COMMON_SET_INPUT_DEVICE_VOLUME, 0, data32, false);
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }

            #if defined(MTK_LINEIN_PLAYBACK_ENABLE)
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=LINEINPLAYBACK_OPEN") != NULL) {
                LOGMSGIDI("AT+EAUDIO=LINEINPLAYBACK_OPEN", 0);
                ami_hal_audio_status_set_running_flag(AUDIO_MESSAGE_TYPE_LINEIN, true);
                mcu2dsp_open_param_t open_param;
                void *p_param_share;

                open_param.param.stream_in = STREAM_IN_AFE;
                open_param.param.stream_out = STREAM_OUT_AFE;

                open_param.stream_in_param.afe.audio_device = HAL_AUDIO_DEVICE_LINEINPLAYBACK_DUAL;
                open_param.stream_in_param.afe.stream_channel = HAL_AUDIO_DIRECT;
                open_param.stream_in_param.afe.memory = HAL_AUDIO_MEM1 ;
                open_param.stream_in_param.afe.audio_interface = HAL_AUDIO_INTERFACE_1;
                open_param.stream_in_param.afe.format = AFE_PCM_FORMAT_S32_LE;
                open_param.stream_in_param.afe.sampling_rate = 48000;
                open_param.stream_in_param.afe.irq_period = 8;
                open_param.stream_in_param.afe.frame_size = 384;
                open_param.stream_in_param.afe.frame_number = 4;
                open_param.stream_in_param.afe.hw_gain = true;
                open_param.stream_in_param.afe.misc_parms = MICBIAS_SOURCE_ALL | MICBIAS3V_OUTVOLTAGE_2p4v;

                open_param.stream_out_param.afe.audio_device = HAL_AUDIO_DEVICE_I2S_MASTER;
                open_param.stream_out_param.afe.stream_channel = HAL_AUDIO_DIRECT;
                open_param.stream_out_param.afe.memory = HAL_AUDIO_MEM1;
                open_param.stream_out_param.afe.audio_interface = HAL_AUDIO_INTERFACE_1;
                open_param.stream_out_param.afe.format = AFE_PCM_FORMAT_S32_LE;
                open_param.stream_out_param.afe.stream_out_sampling_rate = 48000;
                open_param.stream_out_param.afe.sampling_rate = 48000;
                open_param.stream_out_param.afe.irq_period = 8;
                open_param.stream_out_param.afe.frame_size = 384;
                open_param.stream_out_param.afe.frame_number = 4;
                open_param.stream_out_param.afe.hw_gain = true;
                open_param.stream_out_param.afe.misc_parms = I2S_CLK_SOURCE_DCXO;

                p_param_share = hal_audio_dsp_controller_put_paramter( &open_param, sizeof(mcu2dsp_open_param_t), AUDIO_MESSAGE_TYPE_LINEIN);

                hal_audio_dsp_controller_send_message(MSG_MCU2DSP_LINEIN_PLAYBACK_OPEN, 0, (uint32_t)p_param_share, true);
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=LINEINPLAYBACK_START") != NULL) {
                LOGMSGIDI("AT+EAUDIO=LINEINPLAYBACK_START", 0);
                mcu2dsp_start_param_t start_param;
                void *p_param_share;

                // Collect parameters
                start_param.param.stream_in     = STREAM_IN_AFE;
                start_param.param.stream_out    = STREAM_OUT_AFE;
                start_param.stream_in_param.afe.aws_flag   =  false;
                start_param.stream_out_param.afe.aws_flag   =  false;
                p_param_share = hal_audio_dsp_controller_put_paramter( &start_param, sizeof(mcu2dsp_start_param_t), AUDIO_MESSAGE_TYPE_LINEIN);
                hal_audio_dsp_controller_send_message(MSG_MCU2DSP_LINEIN_PLAYBACK_START, 0, (uint32_t)p_param_share, true);
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=LINEINPLAYBACK_STOP") != NULL) {
                LOGMSGIDI("AT+EAUDIO=LINEINPLAYBACK_STOP", 0);
                hal_audio_dsp_controller_send_message(MSG_MCU2DSP_LINEIN_PLAYBACK_STOP, 0, 0, true);
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=LINEINPLAYBACK_CLOSE") != NULL) {
                LOGMSGIDI("AT+EAUDIO=LINEINPLAYBACK_CLOSE", 0);
                hal_audio_dsp_controller_send_message(MSG_MCU2DSP_LINEIN_PLAYBACK_CLOSE, 0, 0, true);
                ami_hal_audio_status_set_running_flag(AUDIO_MESSAGE_TYPE_LINEIN, false);
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=LINEINPLAYBACK_SUSPEND") != NULL) {
                LOGMSGIDI("AT+EAUDIO=LINEINPLAYBACK_SUSPEND", 0);
                hal_audio_dsp_controller_send_message(MSG_MCU2DSP_LINEIN_PLAYBACK_SUSPEND, 0, 0, true);
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=LINEINPLAYBACK_RESUME") != NULL) {
                LOGMSGIDI("AT+EAUDIO=LINEINPLAYBACK_RESUME", 0);
                hal_audio_dsp_controller_send_message(MSG_MCU2DSP_LINEIN_PLAYBACK_RESUME, 0, 0, true);
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=AUDIO_LINEIN_PLAYBACK_OPEN") != NULL) {
                LOGMSGIDI("AT+EAUDIO=AUDIO_LINEIN_PLAYBACK_OPEN", 0);
                hal_audio_sampling_rate_t fs;
                hal_audio_device_t in_device;
                hal_audio_device_t out_device;

                char* config_s = NULL;

                config_s = strchr((char *)parse_cmd->string_ptr, ',');
                config_s++;
                sscanf(config_s, "%d", (int*)&fs);

                config_s = strchr(config_s, ',');
                config_s++;
                sscanf(config_s, "%d", (int*)&in_device);

                config_s = strchr(config_s, ',');
                config_s++;
                sscanf(config_s, "%d", (int*)&out_device);
#if 0
                audio_linein_playback_open(fs, in_device, out_device);
#else
                audio_sink_srv_line_in_control_action_handler(AUDIO_SINK_SRV_LINE_IN_ACT_DEVICE_PLUG_IN, NULL);
                audio_sink_srv_am_line_in_codec_t Line_in_codec;
                Line_in_codec.codec_cap.in_audio_device    = in_device;
                Line_in_codec.codec_cap.out_audio_device   = out_device;
                Line_in_codec.codec_cap.linein_sample_rate = fs;
                audio_sink_srv_line_in_set_param(&Line_in_codec);
#endif
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=AUDIO_LINEIN_PLAYBACK_START") != NULL) {
                LOGMSGIDI("AT+EAUDIO=AUDIO_LINEIN_PLAYBACK_START", 0);
#if 0
                audio_linein_playback_start();
#else
                audio_sink_srv_line_in_control_action_handler(AUDIO_SINK_SRV_LINE_IN_ACT_TRIGGER_START, NULL);
#endif
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=AUDIO_LINEIN_PLAYBACK_STOP") != NULL) {
                LOGMSGIDI("AT+EAUDIO=AUDIO_LINEIN_PLAYBACK_STOP", 0);
#if 0
                audio_linein_playback_stop();
#else
                audio_sink_srv_line_in_control_action_handler(AUDIO_SINK_SRV_LINE_IN_ACT_TRIGGER_STOP, NULL);
#endif
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=AUDIO_LINEIN_PLAYBACK_CLOSE") != NULL) {
                LOGMSGIDI("AT+EAUDIO=AUDIO_LINEIN_PLAYBACK_CLOSE", 0);
#if 0
                audio_linein_playback_close();
#else
                audio_sink_srv_line_in_control_action_handler(AUDIO_SINK_SRV_LINE_IN_ACT_DEVICE_PLUG_OUT, NULL);
#endif
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=AUDIO_LINEIN_PLAYBACK_SET_VOLUME") != NULL) {
                LOGMSGIDI("AT+EAUDIO=AUDIO_LINEIN_PLAYBACK_SET_VOLUME", 0);
                linein_playback_gain_t gain;

                char* config_s = NULL;

                config_s = strchr((char *)parse_cmd->string_ptr, ',');
                config_s++;
                sscanf(config_s, "%lu", &gain.in_digital_gain);

                config_s = strchr(config_s, ',');
                config_s++;
                sscanf(config_s, "%lu", &gain.in_analog_gain);

                config_s = strchr(config_s, ',');
                config_s++;
                sscanf(config_s, "%lu", &gain.out_digital_gain);

                config_s = strchr(config_s, ',');
                config_s++;
                sscanf(config_s, "%lu", &gain.out_analog_gain);

                audio_linein_playback_set_volume(gain);
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=AUDIO_PURE_LINEIN_PLAYBACK_OPEN") != NULL) {
                LOGMSGIDI("AT+EAUDIO=AUDIO_PURE_LINEIN_PLAYBACK_OPEN", 0);
                hal_audio_sampling_rate_t fs;
                hal_audio_device_t in_device;
                hal_audio_device_t out_device;

                char* config_s = NULL;

                config_s = strchr((char *)parse_cmd->string_ptr, ',');
                config_s++;
                sscanf(config_s, "%d", (int*)&fs);

                config_s = strchr(config_s, ',');
                config_s++;
                sscanf(config_s, "%d", (int*)&in_device);

                config_s = strchr(config_s, ',');
                config_s++;
                sscanf(config_s, "%d", (int*)&out_device);
                audio_pure_linein_playback_open(fs, in_device, out_device);
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=AUDIO_PURE_LINEIN_PLAYBACK_CLOSE") != NULL) {
                LOGMSGIDI("AT+EAUDIO=AUDIO_PURE_LINEIN_PLAYBACK_CLOSE", 0);
                audio_pure_linein_playback_close();
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            #endif /* MTK_LINEIN_PLAYBACK_ENABLE */
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=DL_NM") != NULL) {
                LOGMSGIDI("AT+EAUDIO=DL_NM", 0);
                *((volatile uint32_t*)(0xA2120B04)) &= 0xFFFFFEFF;
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=DL_HP") != NULL) {
                LOGMSGIDI("AT+EAUDIO=DL_HP", 0);
                *((volatile uint32_t*)(0xA2120B04)) |= 0x100;
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=UL_NM") != NULL) {
                LOGMSGIDI("AT+EAUDIO=UL_NM", 0);
                *((volatile uint32_t*)(0xA2120B04)) &= 0xFFFFFDFF;
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=UL_HP") != NULL) {
                LOGMSGIDI("AT+EAUDIO=UL_HP", 0);
                *((volatile uint32_t*)(0xA2120B04)) |= 0x200;
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=REG_SET,") != NULL) {

                char *pch = strstr((char *)parse_cmd->string_ptr, "0x");
                if(pch == NULL){
                    LOGMSGIDE("[REG_SET]Input first parameter error.", 0);
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_ERROR;
                    presponse->response_len = strlen((const char *)presponse->response_buf);
                    atci_send_response(presponse);
                    break;
                }

                unsigned int number = (unsigned int)strtoul(pch, NULL, 0);
                LOGMSGIDI("RG result is: %x \r\n", 1, number);

                pch = pch + 3;  // to skip the first one
                char *pch2 = strstr(pch, "0x");
                if(pch2 == NULL){
                    LOGMSGIDE("[REG_SET]Input second parameter error.", 0);
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_ERROR;
                    presponse->response_len = strlen((const char *)presponse->response_buf);
                    atci_send_response(presponse);
                    break;
                }

                unsigned int number2 = (unsigned int)strtoul(pch2, NULL, 0);
                LOGMSGIDI("Value result is: %x \r\n", 1, number2);

                 *((volatile uint32_t*)(number)) = number2;

                LOGMSGIDI("Reg 0x%x is 0x%x", 2, number, *((volatile unsigned int*)(number)));

                LOGMSGIDI("Kevin here \r\n", 0);

                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
           // "AT+EAUDIO=REG_GET,0x70000749"
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=REG_GET,") != NULL) {

                char *pch = strstr((char *)parse_cmd->string_ptr, "0x");
                if(pch == NULL){
                    LOGMSGIDE("[REG_GET]Input parameter error.", 0);
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_ERROR;
                    presponse->response_len = strlen((const char *)presponse->response_buf);
                    atci_send_response(presponse);
                    break;
                }

                unsigned int number = (unsigned int)strtoul(pch, NULL, 0);
                LOGMSGIDI("Reg 0x%x is 0x%x", 2, number, *((volatile unsigned int*)(number)));
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
#if defined(HAL_AUDIO_SUPPORT_DEBUG_DUMP)
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=REG_DEBUG_DUMP") != NULL) {
                LOGMSGIDI("AT+EAUDIO=REG_DEBUG_DUMP", 0);
                hal_audio_debug_dump();
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
#endif
            #if defined(HAL_AUDIO_SUPPORT_APLL)
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=TurnOnAPLL") != NULL) {
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_ERROR;
                char* config_s = NULL;
                unsigned int samplerate;

                config_s = strchr((char *)parse_cmd->string_ptr, ',');
                config_s++;
                sscanf(config_s, "%d", &samplerate);

                LOGMSGIDI("AT+EAUDIO=TurnOnAPLL, FS:%d", 1, samplerate);
                ami_hal_audio_status_set_running_flag(AUDIO_MESSAGE_TYPE_AFE, true);
                if (HAL_AUDIO_STATUS_OK == hal_audio_apll_enable(true, samplerate)){
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                }
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=TurnOffAPLL") != NULL) {
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_ERROR;
                char* config_s = NULL;
                unsigned int samplerate;

                config_s = strchr((char *)parse_cmd->string_ptr, ',');
                config_s++;
                sscanf(config_s, "%d", &samplerate);

                LOGMSGIDI("AT+EAUDIO=TurnOffAPLL, FS:%d", 1, samplerate);
                if (HAL_AUDIO_STATUS_OK == hal_audio_apll_enable(false, samplerate)){
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                }
                ami_hal_audio_status_set_running_flag(AUDIO_MESSAGE_TYPE_AFE, false);
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=QUERY_APLL") != NULL) {
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_ERROR;
                LOGMSGIDI("AT+EAUDIO=QUERY_APLL", 0);
                if (HAL_AUDIO_STATUS_OK == hal_audio_query_apll_status()){
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                }
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=TurnOnMCLK") != NULL) {
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_ERROR;
                char* config_s = NULL;
                uint8_t mclkoutpin, apll, divider;

                config_s = strchr((char *)parse_cmd->string_ptr, ',');
                config_s++;
                sscanf(config_s, "%d", (int*)&mclkoutpin);

                config_s = strchr(config_s, ',');
                config_s++;
                sscanf(config_s, "%d", (int*)&apll);

                config_s = strchr(config_s, ',');
                config_s++;
                sscanf(config_s, "%d", (int*)&divider);

                LOGMSGIDI("AT+EAUDIO=TurnOnMCLK, I2S:%d, APLL:%d, DIVIDER:%d", 3, mclkoutpin, apll, divider);
                ami_hal_audio_status_set_running_flag(AUDIO_MESSAGE_TYPE_AFE, true);
                if (HAL_AUDIO_STATUS_OK == hal_audio_mclk_enable(true, mclkoutpin, apll, divider)){
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                }
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=TurnOffMCLK") != NULL) {
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_ERROR;
                char* config_s = NULL;
                uint8_t mclkoutpin, apll, divider;

                config_s = strchr((char *)parse_cmd->string_ptr, ',');
                config_s++;
                sscanf(config_s, "%d", (int*)&mclkoutpin);

                config_s = strchr(config_s, ',');
                config_s++;
                sscanf(config_s, "%d", (int*)&apll);

                config_s = strchr(config_s, ',');
                config_s++;
                sscanf(config_s, "%d", (int*)&divider);

                LOGMSGIDI("AT+EAUDIO=TurnOffMCLK, I2S:%d, APLL:%d, DIVIDER:%d", 3, mclkoutpin, apll, divider);
                if (HAL_AUDIO_STATUS_OK == hal_audio_mclk_enable(false, mclkoutpin, apll, divider)){
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                }
                ami_hal_audio_status_set_running_flag(AUDIO_MESSAGE_TYPE_AFE, false);
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=QUERY_MCLK") != NULL) {
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_ERROR;
                LOGMSGIDI("AT+EAUDIO=QUERY_MCLK", 0);
                if (HAL_AUDIO_STATUS_OK == hal_audio_query_mclk_status()){
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                }
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            #endif
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=DL_ANA_VOL,") != NULL) {
                char* config_s = NULL;
                unsigned int config = 0;
                config_s = strchr(parse_cmd->string_ptr, ',');
                config_s++;
                sscanf(config_s, "%x", &config);
                hal_audio_set_stream_out_volume(0x258,config);  // 600, config
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=ALC_OFF") != NULL) {
                hal_audio_dsp_controller_send_message(MSG_MCU2DSP_COMMON_ALC_SWITCH, 0, 0, false);
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=ALC_ON") != NULL) {
                hal_audio_dsp_controller_send_message(MSG_MCU2DSP_COMMON_ALC_SWITCH, 0, 1, false);
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=SW_SWAP_R_MIC") != NULL) {
                LOGMSGIDI("SW_SWAP_R_MIC",0);
                hal_audio_dsp_controller_send_message(MSG_MCU2DSP_COMMON_CHANGE_DSP_CHANEL, 1, 3, false);
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=SW_SWAP_L_MIC") != NULL) {
                LOGMSGIDI("SW_SWAP_L_MIC",0);
                hal_audio_dsp_controller_send_message(MSG_MCU2DSP_COMMON_CHANGE_DSP_CHANEL, 1, 2, false);
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=AEC_NR_EN") != NULL) {
                LOGMSGIDI("AEC_NR_EN",0);
                hal_audio_dsp_controller_send_message(MSG_MCU2DSP_COMMON_AEC_NR_EN, 1, 1, false);
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=AEC_NR_DIS") != NULL) {
                LOGMSGIDI("AEC_NR_DIS",0);
                hal_audio_dsp_controller_send_message(MSG_MCU2DSP_COMMON_AEC_NR_EN, 1, 0, false);
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            #ifdef MTK_ANC_ENABLE
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=ANC_ON,") != NULL) {
                char* config_s = NULL;
                unsigned int config = 0;
                unsigned int anc_mode;
                short runtime_gain = 0;
                config_s = strchr(parse_cmd->string_ptr, ',');
                config_s++;
                sscanf(config_s, "%d", &config);
                LOGMSGIDI("ANC on: %x",1,config);
                config_s = strchr(config_s, ',');
                if(config_s != NULL) {
                    config_s++;
                    sscanf(config_s, "%hd", &runtime_gain);
                    LOGMSGIDI("ANC on: %x %d",2,config, runtime_gain);
                    config_s = strchr(config_s, ',');
                    if(config_s != NULL) {
                        config_s++;
                        sscanf(config_s, "%d", &anc_mode);
                        if(anc_mode == 1) {
                            config |= ANC_FF_ONLY_BIT_MASK;
                        } else if(anc_mode == 2) {
                            config |= ANC_FB_ONLY_BIT_MASK;
                        }
                        LOGMSGIDI("ANC on: %x %d %d",3,config, runtime_gain, anc_mode);
                    }
                }
                audio_anc_enable(config, runtime_gain, at_anc_callback);
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=ANC_OFF") != NULL) {
                audio_anc_disable(at_anc_callback);
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=ANC_SET_VOL") != NULL) {
                char* config_s = NULL;
                anc_sw_gain_t anc_sw_gain;
                unsigned int to_role = 0;

                config_s = strchr((char *)parse_cmd->string_ptr, ',');
                config_s++;
                sscanf(config_s, "%hd", &anc_sw_gain.gain_index_l);

                config_s = strchr(config_s, ',');
                config_s++;
                sscanf(config_s, "%hd", &anc_sw_gain.gain_index_r);

                config_s = strchr(config_s, ',');
                config_s++;
                sscanf(config_s, "%u", &to_role);

                audio_anc_set_volume(anc_sw_gain, (anc_to_role_t)to_role);

                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=ANC_SET_RUNTIME_VOL") != NULL) {
                char* config_s = NULL;
                int16_t anc_runtime_gain;

                config_s = strchr((char *)parse_cmd->string_ptr, ',');
                config_s++;
                sscanf(config_s, "%hd", &anc_runtime_gain);

                audio_anc_set_runtime_volume(anc_runtime_gain);

                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=ANC_READ_VOL_NVDM") != NULL) {
                char* config_s = NULL;
                anc_sw_gain_t anc_sw_gain;
                uint16_t role = 0;
                uint32_t ret;

                config_s = strchr((char *)parse_cmd->string_ptr, ',');
                config_s++;
                sscanf(config_s, "%hd", &role);

                ret = audio_anc_read_volume_nvdm(&anc_sw_gain, (uint8_t)role);
                if(role == 0) {
                    LOGMSGIDI("ANC read vol nvdm: %d %d",2,anc_sw_gain.gain_index_l,anc_sw_gain.gain_index_r);
                }

                presponse->response_flag = (ret == 0) ? ATCI_RESPONSE_FLAG_APPEND_OK : ATCI_RESPONSE_FLAG_APPEND_ERROR;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=ANC_WRITE_VOL_NVDM") != NULL) {
                char* config_s = NULL;
                anc_sw_gain_t anc_sw_gain;
                uint16_t role = 0;
                uint32_t ret;

                config_s = strchr((char *)parse_cmd->string_ptr, ',');
                config_s++;
                sscanf(config_s, "%hd", &anc_sw_gain.gain_index_l);

                config_s = strchr(config_s, ',');
                config_s++;
                sscanf(config_s, "%hd", &anc_sw_gain.gain_index_r);

                config_s = strchr(config_s, ',');
                config_s++;
                sscanf(config_s, "%hd", &role);

                ret = audio_anc_write_volume_nvdm(&anc_sw_gain, (uint8_t)role);

                presponse->response_flag = (ret == 0) ? ATCI_RESPONSE_FLAG_APPEND_OK : ATCI_RESPONSE_FLAG_APPEND_ERROR;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=ANC_WRITE_F_VOL_NVDM") != NULL) {
                char* config_s = NULL;
                unsigned int filter_type;
                short filter_unique_gain;
                config_s = strchr((char *)parse_cmd->string_ptr, ',');
                config_s++;
                sscanf(config_s, "%u", &filter_type);
                config_s = strchr(config_s, ',');
                config_s++;
                sscanf(config_s, "%hd", &filter_unique_gain);
                anc_write_filter_volume_nvdm(filter_type, filter_unique_gain);

                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if ((strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=ANC_CONFIG,") != NULL)
                   || (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=ANC_CONFIG_THRU,") != NULL)) {
                char* config_s = NULL;
                uint32_t arg[4];
                uint32_t index = 0;
                uint32_t nvkeyid = NVKEY_DSP_PARA_ANC;
                memset(arg, 0, sizeof(arg));
                config_s = (char *)parse_cmd->string_ptr;
                if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=ANC_CONFIG_THRU,") != NULL) {
                    nvkeyid = NVKEY_DSP_PARA_PASS_THRU;
                }
                while(((config_s = strchr(config_s, ',')) != NULL) && (index < 4))
                {
                    config_s++;
                    sscanf(config_s, "%d", (int *)&arg[index]);
                    index++;
                }
                anc_config(ANC_CONFIG_RAMP_DLY, &arg[0]);
                anc_config(ANC_CONFIG_ADC_SWAP, &arg[1]);
                anc_config(ANC_CONFIG_RAMP_STEP, &arg[2]);
                anc_config(ANC_CONFIG_RAMP_CONFIG_GAIN, &arg[3]);
                anc_config(ANC_CONFIG_RESTORE_NVDM_PARAM, &nvkeyid);
                LOGMSGIDI("nvkey 0x%x config: %d %d %d %d ",5,nvkeyid,arg[0],arg[1],arg[2],arg[3]);
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if ((strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=ANC_TUNE,") != NULL)
                   || (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=ANC_TUNE_THRU,") != NULL)) {
                char* config_s = NULL;
                void *p_param_share;
                uint32_t arg[11];
                uint32_t index = 0;
                uint32_t nvkeyid = NVKEY_DSP_PARA_ANC;
                memset(arg, 0, sizeof(arg));
                config_s = (char *)parse_cmd->string_ptr;
                if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=ANC_TUNE_THRU,") != NULL) {
                    nvkeyid = NVKEY_DSP_PARA_PASS_THRU;
                }
                while(((config_s = strchr(config_s, ',')) != NULL) && (index < 11))
                {
                    config_s++;
                    sscanf(config_s, "%d", (int *)&arg[index]);
                    index++;
                }
                anc_config(ANC_CONFIG_PWR_DET_ENABLE, &arg[0]);
                anc_config(ANC_CONFIG_SMOOTH, &arg[1]);
                anc_config(ANC_CONFIG_HIGH_THRESHOLD_IN_DBFS, &arg[2]);
                anc_config(ANC_CONFIG_LOWER_BOUND, &arg[3]);
                anc_config(ANC_CONFIG_RAMP_UP_DLY, &arg[4]);
                anc_config(ANC_CONFIG_RAMP_UP_STEP, &arg[5]);
                anc_config(ANC_CONFIG_RAMP_DOWN_DLY, &arg[6]);
                anc_config(ANC_CONFIG_RAMP_DOWN_STEP, &arg[7]);
                anc_config(ANC_CONFIG_OVF_CHECK_ENABLE, &arg[8]);
                anc_config(ANC_CONFIG_OVF_THRESHOLD, &arg[9]);
                anc_config(ANC_CONFIG_REDUCE_GAIN_STEP, &arg[10]);
                anc_config(ANC_CONFIG_RESTORE_NVDM_PARAM, &nvkeyid);

                LOGMSGIDI("nvkey 0x%x tune: %d %d %d %d %d %d %d %d %d %d %d",12,nvkeyid,arg[0],arg[1],arg[2],arg[3],arg[4],arg[5],arg[6],arg[7],arg[8],arg[9],arg[10]);
                p_param_share = hal_audio_dsp_controller_put_paramter( &arg, sizeof(arg), AUDIO_MESSAGE_TYPE_ANC);
                hal_audio_dsp_controller_send_message(MSG_MCU2DSP_COMMON_ANC_SET_PARAM, 0, (uint32_t)p_param_share, false);

                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=ANC_PT") != NULL) {
                char* config_s = NULL;
                unsigned int config = 0;
                config_s = strchr(parse_cmd->string_ptr, ',');
                config_s++;
                sscanf(config_s, "%d", &config);
                hal_audio_dsp_controller_send_message(MSG_MCU2DSP_COMMON_ANC_SET_PARAM, 0xFFFF, (uint32_t)config, false);

                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=ANC_READ_COEF") != NULL) {
                hal_audio_dsp_controller_send_message(MSG_MCU2DSP_COMMON_ANC_SET_PARAM, 2, (uint32_t)0, false);
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            #ifdef MTK_DEQ_ENABLE
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=ANC_MUTE_STRM,") != NULL) {
                char* config_s = NULL;
                int config = 0;
                config_s = strchr(parse_cmd->string_ptr, ',');
                config_s++;
                sscanf(config_s, "%d", &config);
                hal_audio_dsp_controller_send_message(MSG_MCU2DSP_COMMON_PEQ_SET_PARAM, 1, (uint32_t)config, true);
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=ANC_DEQ,") != NULL) {
                char* config_s = NULL;
                unsigned int delay_samples, phase_inverse;
                short gain;
                config_s = strchr((char *)parse_cmd->string_ptr, ',');
                config_s++;
                sscanf(config_s, "%u", &delay_samples);
                config_s = strchr(config_s, ',');
                config_s++;
                sscanf(config_s, "%hd", &gain);
                config_s = strchr(config_s, ',');
                config_s++;
                sscanf(config_s, "%u", &phase_inverse);
                anc_set_deq_param((unsigned char)delay_samples, gain, (unsigned char)phase_inverse);

                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=DEQ_STEP1") != NULL) {
                uint32_t value = 0;
                uint32_t nvkeyid = NVKEY_DSP_PARA_ANC;
                anc_sw_gain_t anc_sw_gain;
                uint8_t anc_enable;

                //AT+EAUDIO=ANC_CONFIG,2,0,6,0\0d\0a
                anc_config(ANC_CONFIG_RAMP_CONFIG_GAIN, &value);
                anc_config(ANC_CONFIG_RESTORE_NVDM_PARAM, &nvkeyid);
                //AT+EAUDIO=ANC_ON,1,0,0\0d\0a
                audio_anc_enable(1, 0, at_anc_callback);
                //AT+EAUDIO=ANC_MUTE_STRM,2\0d\0a
                hal_audio_dsp_controller_send_message(MSG_MCU2DSP_COMMON_PEQ_SET_PARAM, 1, (uint32_t)2, true);
                //AT+EAUDIO=ANC_SET_VOL,-30000,0,0\0d\0a
                do {
                    anc_get_status(&anc_enable, NULL, NULL);
                } while (anc_enable == 0);
                anc_sw_gain.gain_index_l = -30000;
                anc_sw_gain.gain_index_r = 0;
                audio_anc_set_volume(anc_sw_gain, (anc_to_role_t)TO_BOTH);
                //AT+EAUDIO=REG_SET,0xA3030080,0x8060000\0d\0a
                nvkeyid = *((volatile unsigned int*)(0xA3030080));
                *((volatile unsigned int*)(0xA3030080)) = (nvkeyid & (~(1<<16)));
                printf("0xA3030080 = 0x%x ", *((volatile unsigned int*)(0xA3030080)));
                printf("DEQ calibrate step 111 finish !");

                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=DEQ_STEP2") != NULL) {
                uint32_t value = 600;
                uint32_t nvkeyid = NVKEY_DSP_PARA_ANC;
                anc_sw_gain_t anc_sw_gain;
                uint8_t anc_enable;

                //AT+EAUDIO=ANC_MUTE_STRM,0\0d\0a
                hal_audio_dsp_controller_send_message(MSG_MCU2DSP_COMMON_PEQ_SET_PARAM, 1, (uint32_t)0, true);
                //AT+EAUDIO=ANC_CONFIG,2,0,6,600\0d\0a
                anc_config(ANC_CONFIG_RAMP_CONFIG_GAIN, &value);
                anc_config(ANC_CONFIG_RESTORE_NVDM_PARAM, &nvkeyid);
                //AT+EAUDIO=ANC_OFF\0d\0a
                audio_anc_disable(at_anc_callback);
                //AT+EAUDIO=ANC_ON,1,0,0\0d\0a
                audio_anc_enable(1, 0, at_anc_callback);
                //AT+EAUDIO=ANC_SET_VOL,-30000,0,0\0d\0a
                do {
                    anc_get_status(&anc_enable, NULL, NULL);
                } while (anc_enable == 0);
                anc_sw_gain.gain_index_l = -30000;
                anc_sw_gain.gain_index_r = 0;
                audio_anc_set_volume(anc_sw_gain, (anc_to_role_t)TO_BOTH);
                printf("DEQ calibrate step 222 finish !");

                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            #endif
            #endif
            #ifdef MTK_PEQ_ENABLE
            else if ((strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=PEQ_MODE,") != NULL) ||
                     (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=PEQ_SYNC,") != NULL))
            {
                char* config_s = NULL;
                unsigned int config = 0;
                unsigned int phase_id = 0;
                unsigned int  ret;
                uint8_t setting_mode = PEQ_DIRECT;
                bt_clock_t target_bt_clk = {0};
                bt_aws_mce_role_t role = bt_connection_manager_device_local_info_get_aws_role();
                if(role == BT_AWS_MCE_ROLE_AGENT) {
                    #ifdef MTK_AWS_MCE_ENABLE
                    bt_clock_t current_bt_clk = {0};
                    int diff = 0;
                    if(strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=PEQ_SYNC,") != NULL)
                    {
                        setting_mode = PEQ_SYNC;
                        ret = bt_sink_srv_bt_clock_addition(&current_bt_clk, NULL, 0);
                        if(ret == BT_STATUS_FAIL) LOGMSGIDW("get current bt clock FAIL\n", 0);
                        ret = bt_sink_srv_bt_clock_addition(&target_bt_clk, NULL, PEQ_FW_LATENCY*1000);
                        if(ret == BT_STATUS_FAIL) LOGMSGIDW("get target bt clock FAIL with duration %d us\n", 1, PEQ_FW_LATENCY*1000);
                        diff = (((int32_t)target_bt_clk.nclk - (int32_t)current_bt_clk.nclk)*625/2 + ((int32_t)target_bt_clk.nclk_intra - (int32_t)current_bt_clk.nclk_intra));
                        if ((diff <= PEQ_FW_LATENCY*1000+10000) && (diff >= PEQ_FW_LATENCY*1000-10000)) {
                            LOGMSGIDI("get cur: %x.%x tar: %x.%x \n", 4, (unsigned int)current_bt_clk.nclk,(unsigned int)current_bt_clk.nclk_intra,(unsigned int)target_bt_clk.nclk,(unsigned int)target_bt_clk.nclk_intra);
                        } else {
                            LOGMSGIDW("cur: %x.%x tar: %x.%x  diff: %d xxxxxxxxxxxx\n",5,(unsigned int)current_bt_clk.nclk,(unsigned int)current_bt_clk.nclk_intra,(unsigned int)target_bt_clk.nclk,(unsigned int)target_bt_clk.nclk_intra,diff);
                        }
                        if(ret == BT_STATUS_FAIL) { // for agent only case
                            setting_mode = PEQ_DIRECT;
                            LOGMSGIDI("PEQ use direct mode for agent only case\n", 0);
                        }
                    }
                    #endif
                }
                config_s = strchr(parse_cmd->string_ptr, ',');
                config_s++;
                sscanf(config_s, "%d", &config);
                config_s = strchr(config_s, ',');
                if (config_s != NULL) {
                    config_s++;
                    sscanf(config_s, "%u", &phase_id);
                }
                if(config < 9) {
                    ret = race_dsprt_peq_change_mode_data((uint8_t)phase_id, setting_mode, target_bt_clk.nclk, (config!=0)?1:0, (uint8_t)config);
                } else {
                    ret = race_dsprt_peq_realtime_data((uint8_t)phase_id, setting_mode, target_bt_clk.nclk, g_peq_test_coef, sizeof(g_peq_test_coef));
                }
                if(ret != 0) {
                    LOGMSGIDE("PEQ_XXX,%d FAIL, role:0x%x, phase_id:%d", 3, config, role, phase_id);
                }
                presponse->response_flag = (ret == 0) ? ATCI_RESPONSE_FLAG_APPEND_OK : ATCI_RESPONSE_FLAG_APPEND_ERROR;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=CONFIG_DIS_PEQ") != NULL) {
                unsigned int  ret;
                ret = race_dsprt_peq_change_mode_data(0, PEQ_DIRECT, 0, 0, (uint8_t)PEQ_SOUND_MODE_FORCE_DRC);
                if(ret != 0) {
                    LOGMSGIDE("CONFIG_DIS_PEQ FAIL, phase 0", 0);
                }
                ret = race_dsprt_peq_change_mode_data(1, PEQ_DIRECT, 0, 0, (uint8_t)PEQ_SOUND_MODE_FORCE_DRC);
                if(ret != 0) {
                    LOGMSGIDE("CONFIG_DIS_PEQ FAIL, phase 1", 0);
                }
                presponse->response_flag = (ret == 0) ? ATCI_RESPONSE_FLAG_APPEND_OK : ATCI_RESPONSE_FLAG_APPEND_ERROR;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=CONFIG_ENA_PEQ") != NULL) {
                unsigned int  ret;
                #ifdef MTK_ANC_ENABLE
                uint8_t anc_enable;
                uint8_t hybrid_enable;
                #endif
                ret = race_dsprt_peq_change_mode_data(0, PEQ_DIRECT, 0, 1, PEQ_SOUND_MODE_FORCE_DRC);
                if(ret != 0) {
                    LOGMSGIDE("CONFIG_ENA_PEQ FAIL, phase 0", 0);
                }
                #ifdef MTK_ANC_ENABLE
                anc_get_status(&anc_enable, NULL, &hybrid_enable);
                if ((anc_enable > 0) && (hybrid_enable > 0)) {
                    ret = race_dsprt_peq_change_mode_data(1, PEQ_DIRECT, 0, 1, PEQ_SOUND_MODE_FORCE_DRC);
                } else {
                    ret = race_dsprt_peq_change_mode_data(1, PEQ_DIRECT, 0, 0, PEQ_SOUND_MODE_UNASSIGNED);
                }
                if(ret != 0) {
                    LOGMSGIDE("CONFIG_ENA_PEQ FAIL, phase 1", 0);
                }
                #endif
                presponse->response_flag = (ret == 0) ? ATCI_RESPONSE_FLAG_APPEND_OK : ATCI_RESPONSE_FLAG_APPEND_ERROR;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }

            #endif
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=VOLUME_PARAM_NVDM") != NULL) {
                char* config_s = NULL;
                ami_audio_volume_parameters_nvdm_t volume_param;

                config_s = strchr((char *)parse_cmd->string_ptr, ',');
                config_s++;
                sscanf(config_s, "%hd", &volume_param.gain1_compensation);

                config_s = strchr(config_s, ',');
                config_s++;
                sscanf(config_s, "%hd", &volume_param.gain2_compensation);

                config_s = strchr(config_s, ',');
                config_s++;
                sscanf(config_s, "%hhu", &volume_param.gain1_sample_per_step);

                config_s = strchr(config_s, ',');
                config_s++;
                sscanf(config_s, "%hhu", &volume_param.gain2_sample_per_step);

                am_set_audio_output_volume_parameters_to_nvdm(&volume_param);
                am_load_audio_output_volume_parameters_from_nvdm();

                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
#endif
#if defined(HAL_AUDIO_TEST_ENABLE)
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=Audio_Play_HEADSET") != NULL) {
                audio_test_set_output_device(HAL_AUDIO_DEVICE_HEADSET);
                audio_test_set_audio_tone(true);
                uint8_t result = audio_test_play_audio_1k_tone();
                if(result == 0) {
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                }
                else {
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_ERROR;
                }
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=Audio_Play_HANDSET") != NULL) {
                audio_test_set_output_device(HAL_AUDIO_DEVICE_HANDS_FREE_MONO);
                audio_test_set_audio_tone(true);
                uint8_t result = audio_test_play_audio_1k_tone();
                if(result == 0) {
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                }
                else {
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_ERROR;
                }
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=Voice_Play_HEADSET") != NULL) {
                audio_test_set_output_device(HAL_AUDIO_DEVICE_HEADSET);
                audio_test_set_audio_tone(false);
                uint8_t result = audio_test_play_voice_1k_tone();
                if(result == 0) {
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                }
                else {
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_ERROR;
                }
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=Play_Stop") != NULL) {
                uint8_t result = audio_test_stop_1k_tone();
                if(result == 0) {
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                }
                else {
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_ERROR;
                }
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=Dump_SPE_COM_Param") != NULL) {
                spe_dump_common_param();
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=Dump_SPE_MOD_Param") != NULL) {
                spe_dump_mode_param();
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                atci_send_response(presponse);
            }
#if defined(HAL_AUDIO_SDFATFS_ENABLE)
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=Record_MMIC_ACC,") != NULL) {
                uint8_t result = 0;
                char* config_s = NULL;
                unsigned int config = 0;
                config_s = strchr(parse_cmd->string_ptr, ',');
                config_s++;
                sscanf(config_s, "%x", &config);
                audio_test_set_input_device(HAL_AUDIO_DEVICE_MAIN_MIC);
                audio_test_set_audio_tone(false);
                audio_test_switch_mic_type(0); //acc
                if(config == 0) {
                    result = audio_test_pcm2way_record();
                } else {
                    result = audio_test_pcm2way_wb_record();
                }
                if(result == 0) {
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                }
                else {
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_ERROR;
                }
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=Record_MMIC_DCC,") != NULL) {
                uint8_t result = 0;
                char* config_s = NULL;
                unsigned int config = 0;
                config_s = strchr(parse_cmd->string_ptr, ',');
                config_s++;
                sscanf(config_s, "%x", &config);
                audio_test_set_input_device(HAL_AUDIO_DEVICE_MAIN_MIC);
                audio_test_set_audio_tone(false);
                audio_test_switch_mic_type(1); //dcc
                if(config == 0) {
                    result = audio_test_pcm2way_record();
                } else {
                    result = audio_test_pcm2way_wb_record();
                }
                if(result == 0) {
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                }
                else {
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_ERROR;
                }
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=Record_DMIC") != NULL) {
                audio_test_set_input_device(HAL_AUDIO_DEVICE_DUAL_DIGITAL_MIC);
                audio_test_set_audio_tone(false);
                uint8_t result = audio_test_pcm2way_record();
                if(result == 0) {
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                }
                else {
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_ERROR;
                }
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=Record_Test_Off") != NULL) {
                uint8_t result = audio_test_pcm2way_stop_1k_tone();
                if(result == 0) {
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                }
                else {
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_ERROR;
                }
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=Audio_Play_SD") != NULL) {
                uint8_t result = audio_test_play_audio_sd();
                if(result == 0) {
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                }
                else {
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_ERROR;
                }
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=Audio_Stop_SD") != NULL) {
                uint8_t result = audio_test_stop_audio_sd();
                if(result == 0) {
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                }
                else {
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_ERROR;
                }
                atci_send_response(presponse);
            }
#endif /* HAL_AUDIO_SDFATFS_ENABLE */
#ifdef MTK_BUILD_SMT_LOAD
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=Ext_LPK_HEADSET_HMIC") != NULL) {
                audio_test_set_output_device(HAL_AUDIO_DEVICE_HEADSET);
                audio_test_set_input_device(HAL_AUDIO_DEVICE_HEADSET_MIC);
                audio_test_set_audio_tone(false);
                uint8_t result = audio_test_external_loopback_test();
                if(result == 0) {
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                }
                else {
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_ERROR;
                }
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=Ext_LPK_SPK_MMIC") != NULL) {
                audio_test_set_input_device(HAL_AUDIO_DEVICE_MAIN_MIC);
                audio_test_set_output_device(HAL_AUDIO_DEVICE_HANDS_FREE_MONO);
                audio_test_set_audio_tone(false);
                uint8_t result = audio_test_external_loopback_test();
                if(result == 0) {
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                }
                else {
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_ERROR;
                }
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=Ext_LPK_SPK_DMIC") != NULL) {
                audio_test_set_input_device(HAL_AUDIO_DEVICE_DUAL_DIGITAL_MIC);
                audio_test_set_output_device(HAL_AUDIO_DEVICE_HANDS_FREE_MONO);
                audio_test_set_audio_tone(false);
                uint8_t result = audio_test_external_loopback_test();
                if(result == 0) {
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                }
                else {
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_ERROR;
                }
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=SMT_RESULT") != NULL) {
                uint8_t result = audio_test_detect_1k_tone_result();
                if(result == 0) {
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                }
                else {
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_ERROR;
                }
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
#ifdef HAL_ACCDET_MODULE_ENABLED
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=ACCDET_TEST") != NULL) {
                uint8_t result = audio_test_detect_earphone();
                snprintf((char * restrict)presponse->response_buf, sizeof(presponse->response_buf), "+EAUDIO:%d\r\n", result);
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=ACCDET_REG") != NULL) {
                //callback
                register_accdet_callback();
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
#endif /* HAL_ACCDET_MODULE_ENABLED */
#endif /* MTK_BUILD_SMT_LOAD */
#if defined(HAL_AUDIO_SLT_ENABLE)
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=SLT_START") != NULL) {
                uint8_t result = audio_slt_test();
                if(result == 0) {
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                }
                else {
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_ERROR;
                }
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=SLT_RESULT") != NULL) {
                audio_test_stop_1k_tone();
                uint8_t result = audio_test_detect_1k_tone_result();
                if(result == 0) {
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                }
                else {
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_ERROR;
                }
                atci_send_response(presponse);
            }
#endif
#ifdef MTK_EXTERNAL_DSP_ENABLE
#if defined (MTK_NDVC_ENABLE)
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=NDVC,") != NULL) {
                char* noise_ptr = NULL;
                uint16_t noise_level =0, noise_idx=0;
                noise_ptr = strchr(parse_cmd->string_ptr, ',');
                noise_ptr++;
                noise_level = atoi(noise_ptr);
                ndvc_at_test = true;
                noise_idx = spe_ndvc_uplink_noise_index_map(noise_level);
                *DSP_SPH_SWNDVC_POWER_INDEX = noise_idx;
                LOGMSGIDI("\r\n[AT]NDVC Test noise_dB=%d, index=%d \r\n",2, noise_level, noise_idx);
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=TESTATNDVCCOFF") != NULL) {
                ndvc_at_test = false;
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                atci_send_response(presponse);
            }
#endif /*MTK_NDVC_ENABLE*/
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=DSP_PWR_ON_DL") != NULL) {
                uint8_t result = external_dsp_activate(true);
                if(result == 0) {
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                }
                else {
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_ERROR;
                }
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=DSP_PWR_OFF") != NULL) {
                uint8_t result = external_dsp_activate(false);
                if(result == 0) {
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                }
                else {
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_ERROR;
                }
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=DSP_SET_MIPS,") != NULL) {
                char *token = strtok(parse_cmd->string_ptr, ","); //s
                uint8_t cnt = 0;
                uint32_t mips = 0;
                while( token != NULL) {
                    if(cnt == 1) {
                        sscanf(token, "0x%x", (unsigned int *)&mips);
                        LOGMSGIDI("mips=%x(%d)\r\n", 2, mips, mips);
                    } else {
                        //printf("parse failed:%s!\r\n", token);
                    }
                    cnt ++;
                    token = strtok(NULL, ",");
                }
                uint8_t result = external_dsp_set_clock_rate(mips);
                if(result == 0) {
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                }
                else {
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_ERROR;
                }
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=DSP_GET_MIPS") != NULL) {
                uint32_t mips = 0;
                uint8_t result = external_dsp_get_clock_rate(&mips);
                if(result == 0) {
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                }
                else {
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_ERROR;
                }
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=DSP_SPE,") != NULL) {
                char* config_s = NULL;
                unsigned int config = 0;
                config_s = strchr(parse_cmd->string_ptr, ',');
                config_s++;
                sscanf(config_s, "%x", &config);
                LOGMSGIDI("config=%x\r\n",1, config);
                external_dsp_spi_init();
                external_dsp_set_output_source(config);
                external_dsp_configure_data_path(config);
                external_dsp_spi_deinit();
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            /* SMT item for MTK automotive product */
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=UL2DL,") != NULL) {
                char *token = strtok(parse_cmd->string_ptr, ","); //s
                uint8_t cnt = 0, result = 0;
                bool    mix_tone = false;
                uint32_t delay_ms = 0;
                while( token != NULL) {
                    if(cnt == 1) {
                        if (strncmp(token, "true", strlen("true")) == 0) {
                            mix_tone = true;
                        } else {
                            mix_tone = false;
                        }
                        LOGMSGIDI("mix_tone=%x\r\n", 1, mix_tone);
                    } else if(cnt == 2) {
                        sscanf(token, "%x", (unsigned int *)&delay_ms);
                        LOGMSGIDI("delay_ms=%x\r\n", 1, delay_ms);
                    } else {
                        //printf("parse failed:%s!\r\n", token);
                    }
                    cnt ++;
                    token = strtok(NULL, ",");
                }
                result = audio_test_loopback_ul2dl_mixer(mix_tone, delay_ms);
                if(result == 0) {
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                }
                else {
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_ERROR;
                }
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=DSP_RD_DRAM,") != NULL) {
                char *token = strtok(parse_cmd->string_ptr, ",");
                uint8_t cnt = 0, result = 0;
                uint32_t address = 0;
                uint16_t value = 0;
                while( token != NULL) {
                    if(cnt == 1) {
                        sscanf(token, "0x%x", (unsigned int *)&address);
                        LOGMSGIDI("address=%x\r\n", 1, address);
                    } else {
                        //printf("parse failed:%s!\r\n", token);
                    }
                    cnt ++;
                    token = strtok(NULL, ",");
                }
                result = external_dsp_read_dram_word(address, &value);
                sprintf((char *)presponse->response_buf, "0x%x", value);
                if(result == 0) {
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                }
                else {
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_ERROR;
                }
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=DSP_WR_DRAM,") != NULL) {
                char *token = strtok(parse_cmd->string_ptr, ",");
                uint8_t cnt = 0, result = 0;
                uint32_t address = 0;
                uint32_t value = 0;
                while( token != NULL) {
                    if(cnt == 1) {
                        sscanf(token, "%x", (unsigned int *)&address);
                        LOGMSGIDI("address=%x\r\n", 1, address);
                    #if 1
                    } else if(cnt == 2) {
                        sscanf(token, "%x", (unsigned int *)&value);
                        LOGMSGIDI("value=%x\r\n", 1, value);
                    #endif
                    }
                    cnt ++;
                    token = strtok(NULL, ",");
                }
                LOGMSGIDI("address=%x, value=%x\r\n",2, address, value);
                result = external_dsp_write_dram_word(address, (uint16_t)value);
                sprintf((char *)presponse->response_buf, "0x%x", (unsigned int)value);
                if(result == 0) {
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                }
                else {
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_ERROR;
                }
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
#if defined(HAL_AUDIO_SLT_ENABLE) || defined(MTK_BUILD_SMT_LOAD)
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=DSP_SMT_1") != NULL) {
                LOGMSGIDI("SMT Test 1. Dual dmic bypass test\r\n", 0);
                uint8_t result = audio_test_dual_dmic_bypass();
                if(result == 0) {
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                }
                else {
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_ERROR;
                }
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=DSP_SMT_2") != NULL) {
                LOGMSGIDI("SMT Test 2-1. Download external dsp firmware\r\n", 0);
                uint8_t result = audio_test_download_external_dsp();
                if(result == 0) {
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                }
                else {
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_ERROR;
                }
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=DSP_SMT_3") != NULL) {
                LOGMSGIDI("SMT Test 2-2. I2S external loopback\r\n", 0);
                uint8_t result = audio_test_i2s_external_loopback();
                if(result == 0) {
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                }
                else {
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_ERROR;
                }
                atci_send_response(presponse);
            }
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=DSP_SLT") != NULL) {
                LOGMSGIDI("[Audio Ext-DSP]SLT Test\r\n", 0);
                uint8_t result = audio_external_dsp_slt_test();
                if(result == 0) {
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_OK;
                }
                else {
                    presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_ERROR;
                }
                atci_send_response(presponse);
            }
#endif /* defined(HAL_AUDIO_SLT_ENABLE) || defined(MTK_BUILD_SMT_LOAD) */
#endif /* #ifdef MTK_EXTERNAL_DSP_ENABLE */

#if defined(MTK_BT_AWS_ENABLE)
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=AWS,") != NULL) {
                char *string_pointer = (char *)parse_cmd->string_ptr;
                int32_t result;
                if (strstr(string_pointer, "AWS,STATUS") != NULL) {
                    aws_clock_skew_status_t status;
                    status = audio_service_aws_get_clock_skew_status();
                    if (status == AWS_CLOCK_SKEW_STATUS_IDLE) {
                        LOGMSGIDI("[AWS AT CMD]AWS_CLOCK_SKEW_STATUS_IDLE \n", 0);
                        result = HAL_AUDIO_AWS_NORMAL;
                    } else if (status == AWS_CLOCK_SKEW_STATUS_BUSY) {
                        LOGMSGIDI("[AWS AT CMD]AWS_CLOCK_SKEW_STATUS_BUSY \n", 0);
                        result = HAL_AUDIO_AWS_NORMAL;
                    } else {
                        result = HAL_AUDIO_AWS_ERROR;
                    }
                } else if (strstr(string_pointer, "AWS,SKEW,") != NULL) {
                    char *p_string = strstr(string_pointer, "AWS,SKEW,");
                    int32_t val = 0;
                    p_string += strlen("AWS,SKEW,");
                    sscanf(p_string, "%d", (int *)&val);
                    result = audio_service_aws_set_clock_skew_compensation_value(val);
                } else if (strstr(string_pointer, "AWS,CNT") != NULL) {
                    uint32_t value;
                    hal_gpio_init(HAL_GPIO_12);
                    hal_pinmux_set_function(HAL_GPIO_12, 0);
                    hal_gpio_set_direction(HAL_GPIO_12, HAL_GPIO_DIRECTION_OUTPUT);
                    hal_gpio_set_output(HAL_GPIO_12, HAL_GPIO_DATA_LOW);
                    hal_gpio_set_output(HAL_GPIO_12, HAL_GPIO_DATA_HIGH);
                    hal_gpio_set_output(HAL_GPIO_12, HAL_GPIO_DATA_LOW);
                    value = audio_service_aws_get_accumulated_sample_count();
                    LOGMSGIDI("[AWS AT CMD]AWS Accumulate count: %d \n",1, value);
                    result = HAL_AUDIO_AWS_NORMAL;
                } else if (strstr(string_pointer, "AWS,FILLSIL,AAC,") != NULL) {
                    char *p_string = strstr(string_pointer, "AWS,FILLSIL,AAC,");
                    uint32_t *sil_frm_cnt;
                    uint32_t temp = 100;
                    uint32_t *byte_cnt = &temp;
                    LOGMSGIDI("byte_cnt: %d",1, *byte_cnt);
                    uint32_t val;
                    p_string += strlen("AWS,FILLSIL,AAC,");
                    sscanf(p_string, "%d", (unsigned int*)&val);
                    sil_frm_cnt = &val;
                    LOGMSGIDI("sil_frame_count: %d",1, *sil_frm_cnt);
                    uint8_t temp_buffer[100];
                    memset(temp_buffer, 0, 100 * sizeof(uint8_t));
                    LOGMSGIDI("[AWS AT CMD]before fill silence: buffer space byte count: %d, sil frm user want to fill: %d",2, *byte_cnt, *sil_frm_cnt);
                    result = audio_service_aws_fill_silence_frame(temp_buffer, byte_cnt, AWS_CODEC_TYPE_AAC_FORMAT, sil_frm_cnt);
                    LOGMSGIDI("[AWS AT CMD]after fill silence: buffer space byte count: %d, sil frm user fill: %d",2, *byte_cnt, *sil_frm_cnt);
                    LOGMSGIDI("[AWS AT CMD]buffer print: ", 0);
                    for (int i = 0; i < 100; i++) {
                        LOGMSGIDI("0x%x ",1, temp_buffer[i]);
                    }
                } else {
                    LOGMSGIDE("[AWS AT CMD]Invalid AWS AT command\r\n", 0);
                    result = HAL_AUDIO_AWS_ERROR;
                }
                presponse->response_flag = result == HAL_AUDIO_AWS_NORMAL ? ATCI_RESPONSE_FLAG_APPEND_OK : ATCI_RESPONSE_FLAG_APPEND_ERROR;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
#endif  /* defined(MTK_BT_AWS_ENABLE) */
#endif /* defined(HAL_AUDIO_TEST_ENABLE) */

#if defined(MTK_BT_HFP_CODEC_DEBUG)
            else if (strstr((char *)parse_cmd->string_ptr, "AT+EAUDIO=DEBUG,") != NULL) {   // for debug use
                char *string_pointer = (char *)parse_cmd->string_ptr;
                int32_t result;
                if (strstr(string_pointer, "DEBUG,HFP,") != NULL) { // debug hfp
                    if (strstr(string_pointer, "DEBUG,HFP,DL_ESCO,") != NULL) { // debug hfp DL_ESCO
                        char *p_string = strstr(string_pointer, "DEBUG,HFP,DL_ESCO,");
                        p_string += strlen("DEBUG,HFP,DL_ESCO,");

                        uint32_t save_or_print_method = 0;
                        sscanf(p_string, "%d", (int *)&save_or_print_method);
                        if (save_or_print_method == AT_AUDIO_HFP_SAVE_OR_PRINT_METHOD_WRITE_TO_FILE) {
                            bt_hfp_codec_debug_open(BT_HFP_CODEC_DEBUG_FLAG_DOWNLINK_ESCO_RAW_DATA, BT_HFP_CODEC_DEBUG_SAVE_OR_PRINT_METHOD_SAVE_TO_SDCARD);
                            result = ATCI_RESPONSE_FLAG_APPEND_OK;
                        } else {
                            LOGMSGIDI("DEBUG,HFP,DL_ESCO: invalid save_or_print_method(%d)\r\n",1, save_or_print_method);
                            result = ATCI_RESPONSE_FLAG_APPEND_ERROR;
                        }
                    } else if (strstr(string_pointer, "DEBUG,HFP,DL_STREAM_OUT_PCM,") != NULL) {
                        char *p_string = strstr(string_pointer, "DEBUG,HFP,DL_STREAM_OUT_PCM,");
                        p_string += strlen("DEBUG,HFP,DL_STREAM_OUT_PCM,");

                        uint32_t save_or_print_method = 0;
                        sscanf(p_string, "%d", (int *)&save_or_print_method);
                        if (save_or_print_method == AT_AUDIO_HFP_SAVE_OR_PRINT_METHOD_WRITE_TO_FILE) {
                            bt_hfp_codec_debug_open(BT_HFP_CODEC_DEBUG_FLAG_DOWNLINK_STREAM_OUT_PCM, BT_HFP_CODEC_DEBUG_SAVE_OR_PRINT_METHOD_SAVE_TO_SDCARD);
                            result = ATCI_RESPONSE_FLAG_APPEND_OK;
                        } else {
                            LOGMSGIDI("DEBUG,HFP,DL_STREAM_OUT_PCM: invalid save_or_print_method(%d)\r\n",1, save_or_print_method);
                            result = ATCI_RESPONSE_FLAG_APPEND_ERROR;
                        }
                    } else if (strstr(string_pointer, "DEBUG,HFP,VM,") != NULL) {
                        char *p_string = strstr(string_pointer, "DEBUG,HFP,VM,");
                        p_string += strlen("DEBUG,HFP,VM,");

                        uint32_t save_or_print_method = 0;
                        sscanf(p_string, "%d", (int *)&save_or_print_method);
                        if (save_or_print_method == AT_AUDIO_HFP_SAVE_OR_PRINT_METHOD_WRITE_TO_FILE) {
                            bt_hfp_codec_debug_open(BT_HFP_CODEC_DEBUG_FLAG_VM_LOG, BT_HFP_CODEC_DEBUG_SAVE_OR_PRINT_METHOD_SAVE_TO_SDCARD);
                            result = ATCI_RESPONSE_FLAG_APPEND_OK;
                        } else if (save_or_print_method == AT_AUDIO_HFP_SAVE_OR_PRINT_METHOD_PRINT_TO_USB_DEBUG_PORT) {
                            bt_hfp_codec_debug_open(BT_HFP_CODEC_DEBUG_FLAG_VM_LOG, BT_HFP_CODEC_DEBUG_SAVE_OR_PRINT_METHOD_PRINT_TO_USB_DEBUG_PORT);
                            result = ATCI_RESPONSE_FLAG_APPEND_OK;
                        } else {
                            LOGMSGIDI("DEBUG,HFP,VM: invalid save_or_print_method(%d)\r\n",1, save_or_print_method);
                            result = ATCI_RESPONSE_FLAG_APPEND_ERROR;
                        }
                    } else if (strstr(string_pointer, "DEBUG,HFP,STOP") != NULL) {
                        bt_hfp_codec_debug_close();
                        result = ATCI_RESPONSE_FLAG_APPEND_OK;
                    } else {
                        LOGMSGIDE("Invalid DEBUG,HFP AT command\r\n",0);
                        result = ATCI_RESPONSE_FLAG_APPEND_ERROR;
                    }
                } else {
                    LOGMSGIDE("Invalid DEBUG AT command : \r\n",0);
                    result = ATCI_RESPONSE_FLAG_APPEND_ERROR;
                }
                presponse->response_flag = result;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
#endif  /* defined(BT_HFP_CODEC_DEBUG) */
            else
            {
                /* invalid AT command */
                LOGMSGIDI("atci_cmd_hdlr_audio: command not exist \r\n", 0);
                presponse->response_flag = ATCI_RESPONSE_FLAG_APPEND_ERROR;
                presponse->response_len = strlen((const char *)presponse->response_buf);
                atci_send_response(presponse);
            }
            break;

        default :
            /* others are invalid command format */
            strcpy((char * restrict)presponse->response_buf, "ERROR\r\n");
            presponse->response_len = strlen((const char *)presponse->response_buf);
            atci_send_response(presponse);
            break;
    }

    vPortFree(presponse);
#endif /*defined(__GNUC__)*/
    return ATCI_STATUS_OK;
}

#endif /* !MTK_AUDIO_AT_COMMAND_DISABLE && (HAL_AUDIO_MODULE_ENABLED) */
