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

#ifndef __CONNSYS_PROFILE_H__
#define __CONNSYS_PROFILE_H__

#include <stdint.h>

#pragma pack(push, 1)

#define WIFI_PROFILE_PDA_HEADER_LEN     (12)

// copy from N9 wifi_uni_mac_7636/rom/include/iot/rt_bora.h
#define MAX_LEN_OF_SSID                 (32)

/**
* @brief definitation of 2G max channel number.
*/
#define WIFI_2G_CHANNEL_NUMBER            (14)    //channel 1 ~14

/**
* @brief definitation of single SKU number of each channel for 2G channel.
*/
#define WIFI_2G_SKU_POWER_PER_CHANNEL     (19)

/**
* @brief definitation of 5G Low, Middle, HIGH max channel number.
*/
#define WIFI_5G_LOW_CHANNEL_NUMBER        (14)     /**<  SingleSKU5G_ch36 ~ singleSKU5G_ch102 */
#define WIFI_5G_MIDDLE_CHANNEL_NUMBER     (14)     /**<  SingleSKU5G_ch104 ~ singleSKU5G_ch140 */
#define WIFI_5G_HIGH_CHANNEL_NUMBER       (11)     /**<  SingleSKU5G_ch142 ~ singleSKU5G_ch173 */

/**
* @brief definitation of single SKU number of each channel for 5G channel.
*/
#define WIFI_5G_SKU_POWER_PER_CHANNEL     (17)
#define WIFI_5G_LOW_CHANNEL_SKU_LEN       (WIFI_5G_LOW_CHANNEL_NUMBER * WIFI_5G_SKU_POWER_PER_CHANNEL)
#define WIFI_5G_MIDDLE_CHANNEL_SKU_LEN    (WIFI_5G_MIDDLE_CHANNEL_NUMBER * WIFI_5G_SKU_POWER_PER_CHANNEL)
#define WIFI_5G_HIGH_CHANNEL_SKU_LEN      (WIFI_5G_HIGH_CHANNEL_NUMBER * WIFI_5G_SKU_POWER_PER_CHANNEL)
#define WIFI_5G_CHANNEL_NUMBER            (WIFI_5G_LOW_CHANNEL_NUMBER + WIFI_5G_MIDDLE_CHANNEL_NUMBER + WIFI_5G_HIGH_CHANNEL_NUMBER)    /**< The 5G total channel number. */


typedef struct _CIPHER_KEY {
    uint8_t   Key[16];            // right now we implement 4 keys, 128 bits max
    uint8_t   RxMic[8];           // make alignment
    uint8_t   TxMic[8];
    uint8_t   TxTsc[6];           // 48bit TSC value
    uint8_t   RxTsc[6];           // 48bit TSC value
    uint8_t   CipherAlg;          // 0-none, 1:WEP64, 2:WEP128, 3:TKIP, 4:AES, 5:CKIP64, 6:CKIP128
    uint8_t   KeyLen;
    uint8_t   BssId[6];
    uint8_t   Type;               // Indicate Pairwise/Group when reporting MIC error
} CIPHER_KEY,*PCIPHER_KEY;

typedef struct {
    unsigned char first_channel;
    unsigned char num_of_ch;
    unsigned char channel_prop; //0: Active, 1: Passive
    unsigned char tx_power; // 4-align and could be used to extend channel_prop
} ch_desc_t;

typedef struct {
    unsigned char num_bg_band_triplet;
    unsigned char num_a_band_triplet;
    ch_desc_t triplet[]; // BGBand Triplet followed by Aband Triplet
} ch_list_t;

/** @brief Indicates the detailed single SKU information.
* For each channel power item format is 2.4G_channel  * (2+5+6+6) power per channel
* | -------|----------|----------|----------- |------------|-------- |---------|---------|---------- |---------- |-------- |-------- |-------- | --------|---------- |---------- |-------- |-------- |-------- |
* |CCK1/2M|CCK5.5/11M|OFDM_6/9M|OFDM_12/18M|OFDM_24/36M|OFDM_48M|OFDM_54M| HT20MCS0|HT20MCS1/2|HT20MCS3/4|HT20MCS5|HT20MCS6|HT20MCS7|HT40MCS0|HT40MCS1/2|HT40MCS3/4|HT40MCS5|HT40MCS6|HT40MCS7|
*/
typedef struct {
    uint8_t singleSKU2G[WIFI_2G_CHANNEL_NUMBER][WIFI_2G_SKU_POWER_PER_CHANNEL]; // 14 2G_channel  * (2+5+6+6) power per channel
} singleSKU2G_t;

/** @brief Indicates the detailed single SKU 5G information.
* For each channel power item format is 5G_channel  * (5+6+6) power per channel
* | ----------|----------- |------------|-------- |---------|---------|---------- |---------- |-------- |-------- |-------- | --------|---------- |---------- |-------- |-------- |-------- |
* |OFDM_6/9M|OFDM_12/18M|OFDM_24/36M|OFDM_48M|OFDM_54M| HT20MCS0|HT20MCS1/2|HT20MCS3/4|HT20MCS5|HT20MCS6|HT20MCS7|HT40MCS0|HT40MCS1/2|HT40MCS3/4|HT40MCS5|HT40MCS6|HT40MCS7|
*/
typedef struct {
#if 0
    uint8_t singleSKU5G[42][17]; // 42 5G_channel  * (5+6+6) power per channel
#else
    uint8_t singleSKU5G_ch36[WIFI_5G_SKU_POWER_PER_CHANNEL];
    uint8_t singleSKU5G_ch38[WIFI_5G_SKU_POWER_PER_CHANNEL];
    uint8_t singleSKU5G_ch40[WIFI_5G_SKU_POWER_PER_CHANNEL];
    uint8_t singleSKU5G_ch44[WIFI_5G_SKU_POWER_PER_CHANNEL];
    uint8_t singleSKU5G_ch46[WIFI_5G_SKU_POWER_PER_CHANNEL];
    uint8_t singleSKU5G_ch48[WIFI_5G_SKU_POWER_PER_CHANNEL];
    uint8_t singleSKU5G_ch52[WIFI_5G_SKU_POWER_PER_CHANNEL];
    uint8_t singleSKU5G_ch54[WIFI_5G_SKU_POWER_PER_CHANNEL];
    uint8_t singleSKU5G_ch56[WIFI_5G_SKU_POWER_PER_CHANNEL];
    uint8_t singleSKU5G_ch60[WIFI_5G_SKU_POWER_PER_CHANNEL];
    uint8_t singleSKU5G_ch62[WIFI_5G_SKU_POWER_PER_CHANNEL];
    uint8_t singleSKU5G_ch64[WIFI_5G_SKU_POWER_PER_CHANNEL];
    uint8_t singleSKU5G_ch100[WIFI_5G_SKU_POWER_PER_CHANNEL];
    uint8_t singleSKU5G_ch102[WIFI_5G_SKU_POWER_PER_CHANNEL];

    uint8_t singleSKU5G_ch104[WIFI_5G_SKU_POWER_PER_CHANNEL];
    uint8_t singleSKU5G_ch108[WIFI_5G_SKU_POWER_PER_CHANNEL];
    uint8_t singleSKU5G_ch110[WIFI_5G_SKU_POWER_PER_CHANNEL];
    uint8_t singleSKU5G_ch112[WIFI_5G_SKU_POWER_PER_CHANNEL];
    uint8_t singleSKU5G_ch116[WIFI_5G_SKU_POWER_PER_CHANNEL];
    uint8_t singleSKU5G_ch118[WIFI_5G_SKU_POWER_PER_CHANNEL];
    uint8_t singleSKU5G_ch120[WIFI_5G_SKU_POWER_PER_CHANNEL];
    uint8_t singleSKU5G_ch124[WIFI_5G_SKU_POWER_PER_CHANNEL];
    uint8_t singleSKU5G_ch126[WIFI_5G_SKU_POWER_PER_CHANNEL];
    uint8_t singleSKU5G_ch128[WIFI_5G_SKU_POWER_PER_CHANNEL];
    uint8_t singleSKU5G_ch132[WIFI_5G_SKU_POWER_PER_CHANNEL];
    uint8_t singleSKU5G_ch134[WIFI_5G_SKU_POWER_PER_CHANNEL];
	uint8_t singleSKU5G_ch136[WIFI_5G_SKU_POWER_PER_CHANNEL];
    uint8_t singleSKU5G_ch140[WIFI_5G_SKU_POWER_PER_CHANNEL];

	uint8_t singleSKU5G_ch142[WIFI_5G_SKU_POWER_PER_CHANNEL];
    uint8_t singleSKU5G_ch144[WIFI_5G_SKU_POWER_PER_CHANNEL];
    uint8_t singleSKU5G_ch149[WIFI_5G_SKU_POWER_PER_CHANNEL];
    uint8_t singleSKU5G_ch151[WIFI_5G_SKU_POWER_PER_CHANNEL];
    uint8_t singleSKU5G_ch153[WIFI_5G_SKU_POWER_PER_CHANNEL];
    uint8_t singleSKU5G_ch157[WIFI_5G_SKU_POWER_PER_CHANNEL];
    uint8_t singleSKU5G_ch159[WIFI_5G_SKU_POWER_PER_CHANNEL];
    uint8_t singleSKU5G_ch161[WIFI_5G_SKU_POWER_PER_CHANNEL];
    uint8_t singleSKU5G_ch165[WIFI_5G_SKU_POWER_PER_CHANNEL];
    uint8_t singleSKU5G_ch169[WIFI_5G_SKU_POWER_PER_CHANNEL];
    uint8_t singleSKU5G_ch173[WIFI_5G_SKU_POWER_PER_CHANNEL];
#endif
} singleSKU5G_t;

typedef struct {
    uint8_t opmode;
    uint8_t country_region;
    uint8_t country_region_a_band;
    uint8_t country_code[4];
    uint8_t radio_off;
    uint8_t dbg_level;
    uint16_t rts_threshold;
    uint16_t frag_threshold;

    uint8_t sta_local_admin_mac;
    uint8_t sta_ip_addr[4];
    uint8_t sta_mac_addr[6];
    uint8_t sta_ssid[32];
    uint8_t sta_ssid_len;
    uint8_t sta_bss_type;
    uint8_t sta_channel;
    uint8_t sta_bw;
    uint8_t sta_wireless_mode;
    uint8_t sta_ba_decline;
    uint8_t sta_auto_ba;
    uint8_t sta_ht_mcs;
    uint8_t sta_ht_ba_win_size;
    uint8_t sta_ht_gi;
    uint8_t sta_ht_protect;
    uint8_t sta_ht_ext_ch;
    uint8_t sta_wmm_capable;
    uint8_t sta_listen_interval;
    uint8_t sta_auth_mode;
    uint8_t sta_encryp_type;
    uint8_t sta_wpa_psk[64];
    uint8_t sta_wpa_psk_len;
    uint8_t sta_pmk[32];
    uint8_t sta_pair_cipher;
    uint8_t sta_group_cipher;
    uint8_t sta_default_key_id;
    uint8_t sta_ps_mode;
    uint8_t sta_keep_alive_period;

    uint8_t ap_local_admin_mac;
    uint8_t ap_ip_addr[4];
    uint8_t ap_mac_addr[6];
    uint8_t ap_ssid[32];
    uint8_t ap_ssid_len;
    uint8_t ap_channel;
    uint8_t ap_bw;
    uint8_t ap_wireless_mode;
    uint8_t ap_auto_ba;
    uint8_t ap_ht_mcs;
    uint8_t ap_ht_ba_win_size;
    uint8_t ap_ht_gi;
    uint8_t ap_ht_protect;
    uint8_t ap_ht_ext_ch;
    uint8_t ap_wmm_capable;
    uint8_t ap_dtim_period;
    uint8_t ap_hide_ssid;
    uint8_t ap_auto_channel_select;
    uint8_t ap_auth_mode;
    uint8_t ap_encryp_type;
    uint8_t ap_wpa_psk[64];
    uint8_t ap_wpa_psk_len;
    uint8_t ap_pmk[32];
    uint8_t ap_pair_cipher;
    uint8_t ap_group_cipher;
    uint8_t ap_default_key_id;

    // "scan channel table" and "regulatory table"
    unsigned char bg_band_entry_num;
    ch_desc_t bg_band_triple[10];
    unsigned char a_band_entry_num;
    ch_desc_t a_band_triple[10];
    uint8_t ap_beacon_disable;
    uint8_t forwarding_zero_copy;

    /* These are for MBSS support, but not exist trunk (it's customer feature), however,
            we have to add them here due to N9 FW has them (only one version of N9 FW)
         */
    // TODO: How to solve it in the future...Michael
    uint8_t mbss_enable;
    uint8_t mbss_ssid1[32];
    uint8_t mbss_ssid_len1;
    uint8_t mbss_ssid2[32];
    uint8_t mbss_ssid_len2;

    uint8_t config_free_ready;
    uint8_t config_free_enable;
    uint8_t sta_fast_link;
    uint8_t beacon_lost_time;
    uint8_t apcli_40mhz_auto_upbelow;
    uint8_t wifi_privilege_enable;
    uint8_t sta_keep_alive_packet;
    uint8_t support_single_sku;//0: not support SKU, 1:support 2G,  2:support 5G, 3:support both 2G and 5G
#if MTK_ANT_DIV_ENABLE
    //TODO, need N9 update the profile together
    uint8_t antenna_number;
    uint8_t antenna_diversity_enable;
    uint8_t antenna_probe_req_count;  //nunber of probe requests to send
    int8_t  antenna_threshold_level;  //rssi threshold level
    uint8_t antenna_delta_rssi;       //delta rssi
    uint8_t antenna_rssi_switch;      //rssi switch value
    int8_t  antenna_pre_selected_rssi;
    uint8_t reserved[141];
#else
    //Fixed profile should put in the place of reserved 
    uint8_t reserved[148];
#endif

    //SKU shoud fix form offset 0x280
#if 1 //def MTK_SINGLE_SKU_SUPPORT
    singleSKU2G_t single_sku_2g;
    singleSKU5G_t single_sku_5g;
#endif
} wifi_sys_cfg_t;


/*move below funtions from connsys_driver.h to connsys_profile.h  beacuse the arg is defined in this header file*/
int32_t connsys_init(wifi_sys_cfg_t *sys_config);
void connsys_set_wifi_profile(wifi_sys_cfg_t *config);

#pragma pack(pop)

#endif // #ifndef __CONNSYS_PROFILE_H__

