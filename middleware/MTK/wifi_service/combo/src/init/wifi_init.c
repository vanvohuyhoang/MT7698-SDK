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

#include "os.h"
#include "wifi_api.h"
#include "wifi_private_api.h"
#include "connsys_driver.h"
#include "wifi_scan.h"
#include "inband_queue.h"
#include "net_task.h"
#include "wifi_init.h"
#include "syslog.h"
#include "nvdm.h"
#include "wifi_profile.h"
#include "connsys_profile.h"
#include "get_profile_string.h"
#include "wifi_default_config.h"
#include "wifi_inband.h"
#include "wifi_channel.h"
#include "hal_efuse.h"
#include "wifi_os_api.h"
#ifdef MTK_ANT_DIV_ENABLE
#include "wifi_ant_div.h"
#endif

#ifdef MTK_MINISUPP_ENABLE
#include "wpa_supplicant_task.h"
#endif

#ifdef MTK_CM4_WIFI_TASK_ENABLE
#include "wifi_firmware_task.h"
#include "security_interface.h"
#endif

#if MTK_WIFI_STUB_CONF_ENABLE
#include "wfc_at_api.h"
#endif

#if (PRODUCT_VERSION == 7698) || (PRODUCT_VERSION == 7686) || (PRODUCT_VERSION == 7682) || (PRODUCT_VERSION == 5932)
uint32_t MACLPSState = 0, MACLPNum = 0;
#endif

int32_t wifi_get_pmk_by_ssid_psk_from_pmkinfo(uint8_t *pmk,uint8_t *ssid,uint8_t ssid_len, uint8_t *psk,uint8_t psk_len);
int32_t wifi_get_pmk_by_ssid_psk_from_pmkinfo_ap(uint8_t *pmk,uint8_t *ssid,uint8_t ssid_len, uint8_t *psk,uint8_t psk_len);

log_create_module(wifi, PRINT_LEVEL_ERROR);
//#define MTK_WIFI_SYS_CFG_SHOW_ENABLE	// For debug, to show wifi_sys_cfg context

#if (PRODUCT_VERSION == 7687) || (PRODUCT_VERSION == 7697)
static int32_t wifi_is_mac_address_valid(uint8_t *mac_addr)
{
    uint32_t byte_sum = 0;
    for (uint32_t index = 0; index < WIFI_MAC_ADDRESS_LENGTH; index++) {
        byte_sum += mac_addr[index];
    }
    return (byte_sum != 0);
}
#endif

wifi_phy_mode_t wifi_change_wireless_mode_5g_to_2g(wifi_phy_mode_t wirelessmode)
{
    if (WIFI_PHY_11A == wirelessmode) {
        return WIFI_PHY_11B;
    } else if (WIFI_PHY_11ABG_MIXED == wirelessmode) {
        return WIFI_PHY_11BG_MIXED;
    } else if (WIFI_PHY_11ABGN_MIXED == wirelessmode) {
        return WIFI_PHY_11BGN_MIXED;
    } else if (WIFI_PHY_11AN_MIXED == wirelessmode) {
        return WIFI_PHY_11N_2_4G;
    } else if (WIFI_PHY_11AGN_MIXED == wirelessmode) {
        return WIFI_PHY_11GN_MIXED;
    } else if (WIFI_PHY_11N_5G == wirelessmode) {
        return WIFI_PHY_11N_2_4G;
    } else {
        return wirelessmode;
    }
}

/**
* @brief get sta scan_round from NVDM or default
*/
static int32_t wifi_get_sta_scan_round(void)
{
#ifdef MTK_WIFI_PROFILE_ENABLE
    uint8_t buff[PROFILE_BUF_LEN];
    uint32_t len = sizeof(buff);
    nvdm_read_data_item("STA", "ScanRound", buff, &len);
    return (int32_t)atoi((char *)buff);
#else
    return (int32_t)atoi(WIFI_DEFAULT_STA_SCAN_ROUND);
#endif
}

/*Add the wifi_init_callback,  to avoid the modules dependency,
   some of callback will be invoked in the wifi driver which located in the src_protected path */
wifi_init_callback g_wifi_init_callback;

static void wifi_init_register_callback()
{
    g_wifi_init_callback.wifi_get_pmk_info = wifi_get_pmk_by_ssid_psk_from_pmkinfo;
#ifdef MTK_ANT_DIV_ENABLE
    g_wifi_init_callback.wifi_record_ant_status = wifi_profile_record_antenna_status;
#endif
    g_wifi_init_callback.wifi_get_scan_round = wifi_get_sta_scan_round;
    g_wifi_init_callback.wifi_get_pmk_info_ap = wifi_get_pmk_by_ssid_psk_from_pmkinfo_ap;
}

/**
* @brief get mac address from efuse
*/
static int32_t wifi_get_mac_addr_from_efuse(uint8_t port, uint8_t *mac_addr)
{
#if (PRODUCT_VERSION == 7698) || (PRODUCT_VERSION == 7686) || (PRODUCT_VERSION == 7682) || (PRODUCT_VERSION == 5932)
    if (wifi_config_get_mac_from_register(port, mac_addr) < 0) {
        return -1;
    }
#elif (PRODUCT_VERSION == 7687) || (PRODUCT_VERSION == 7697)
    uint8_t buf[16] = {0};      //efuse is 16 byte aligned
    uint16_t mac_offset = 0x00; //mac addr offset in efuse
    if (HAL_EFUSE_OK != hal_efuse_read(mac_offset, buf, sizeof(buf))) {
        return -1;
    }
    if (!wifi_is_mac_address_valid(buf+4)) {
        LOG_HEXDUMP_W(wifi, "data in efuse is invalid", buf, sizeof(buf));
        return -1;
    }
    if (WIFI_PORT_STA == port) {
        /* original efuse MAC address for STA */
        os_memcpy(mac_addr, buf+4, WIFI_MAC_ADDRESS_LENGTH);
    } else {
        /* original efuse MAC address with byte[5]+1 for AP */
        os_memcpy(mac_addr, buf+4, WIFI_MAC_ADDRESS_LENGTH);
        mac_addr[WIFI_MAC_ADDRESS_LENGTH-1] += 1;
    }
#endif
    return 0;
}


#ifdef MTK_NVDM_ENABLE
/**
* @brief get mac address from nvdm
*/
static int32_t wifi_get_mac_addr_from_nvdm(uint8_t port, uint8_t *mac_addr)
{
    uint8_t buff[PROFILE_BUF_LEN] = {0};
    uint32_t len = sizeof(buff);
    char *group_name = (WIFI_PORT_STA == port) ? "STA" : "AP";

    if (NVDM_STATUS_OK != nvdm_read_data_item(group_name, "MacAddr", buff, &len)) {
        return -1;
    }

    wifi_conf_get_mac_from_str((char *)mac_addr, (char *)buff);
    return 0;
}
#endif

/**
* @brief Get WiFi Interface MAC Address.
*
*/
int32_t wifi_config_get_mac_address(uint8_t port, uint8_t *address)
{
    if (NULL == address) {
        LOG_E(wifi, "address is null.");
        return WIFI_ERR_PARA_INVALID;
    }

    if (!wifi_is_port_valid(port)) {
        LOG_E(wifi, "port is invalid: %d", port);
        return WIFI_ERR_PARA_INVALID;
    }

#ifdef MTK_WIFI_STUB_CONF_ENABLE
    /*in MT5932, consider to use WFC Host MAC first,  then consider the Efuse MAC*/
    if (0 == wfc_get_wifi_host_mac(port, address)){
        return 0;
    }

    LOG_W(wifi, "wfc_get_wifi_host_mac fail.");
#endif

    if (0 == wifi_get_mac_addr_from_efuse(port, address)) {
        return 0;
    }

    LOG_W(wifi, "wifi_get_mac_addr_from_efuse fail.");
#ifdef MTK_NVDM_ENABLE
    if (0 == wifi_get_mac_addr_from_nvdm(port, address)) {
        return 0;
    }
    LOG_E(wifi, "wifi_get_mac_addr_from_nvdm fail.");
#endif
    return -1;

}


uint8_t wifi_get_ps_mode(void)
{
#ifdef MTK_WIFI_PROFILE_ENABLE
    uint8_t buff[PROFILE_BUF_LEN] = {0};
    uint32_t len = sizeof(buff);
    nvdm_read_data_item("STA", "PSMode", buff, &len);
    return (uint8_t)atoi((char *)buff);
#else
    return (uint8_t)atoi(WIFI_DEFAULT_STA_POWER_SAVE_MODE);
#endif
}

static void wifi_save_sta_ext_config(wifi_sys_cfg_t *syscfg, wifi_config_ext_t *config_ext)
{
    if (NULL != config_ext) {
        if (config_ext->sta_wep_key_index_present) {
            syscfg->sta_default_key_id = config_ext->sta_wep_key_index;
        }
        if (config_ext->sta_bandwidth_present) {
            syscfg->sta_bw = config_ext->sta_bandwidth;
        }
        if (config_ext->sta_wireless_mode_present) {
            syscfg->sta_wireless_mode = config_ext->sta_wireless_mode;
        }
        if (config_ext->sta_listen_interval_present) {
            syscfg->sta_listen_interval = config_ext->sta_listen_interval;
        }
        if (config_ext->sta_power_save_mode_present) {
            syscfg->sta_ps_mode = config_ext->sta_power_save_mode;
        }

    }
}

static void wifi_save_ap_ext_config(wifi_sys_cfg_t *syscfg, wifi_config_ext_t *config_ext)
{
    if (NULL != config_ext) {
        if (config_ext->ap_wep_key_index_present) {
            syscfg->ap_default_key_id = config_ext->ap_wep_key_index;
        }
        if (config_ext->ap_hidden_ssid_enable_present) {
            syscfg->ap_hide_ssid = config_ext->ap_hidden_ssid_enable;
        }
        if (config_ext->ap_wireless_mode_present) {
            syscfg->ap_wireless_mode = config_ext->ap_wireless_mode;
        }
        if (config_ext->ap_dtim_interval_present) {
            syscfg->ap_dtim_period = config_ext->ap_dtim_interval;
        }
    }
}

static int32_t wifi_apply_sta_config(wifi_sys_cfg_t *syscfg, wifi_config_t *config, wifi_config_ext_t *config_ext)
{
    if( NULL != config ){
        os_memcpy(syscfg->sta_ssid, config->sta_config.ssid, WIFI_MAX_LENGTH_OF_SSID);
        if( WIFI_MAX_LENGTH_OF_SSID < config->sta_config.ssid_length ){
            //config->sta_config.ssid_length = WIFI_MAX_LENGTH_OF_SSID;
            LOG_E(wifi, "sta_config.ssid_length is longer than %d. initial aborted!", WIFI_MAX_LENGTH_OF_SSID);
            return WIFI_FAIL;
        }
        syscfg->sta_ssid_len = config->sta_config.ssid_length;

        os_memcpy(syscfg->sta_wpa_psk, config->sta_config.password, WIFI_LENGTH_PASSPHRASE);
        if( WIFI_LENGTH_PASSPHRASE < config->sta_config.password_length ){
            //config->sta_config.password_length = WIFI_LENGTH_PASSPHRASE;
            LOG_E(wifi, "sta_config.password_length is longer than %d. initial aborted!", WIFI_LENGTH_PASSPHRASE);
            return WIFI_FAIL;
        }
        syscfg->sta_wpa_psk_len = config->sta_config.password_length;
    }
    /* save extension config */
    if( NULL != config_ext ){
        wifi_save_sta_ext_config(syscfg, config_ext);
    }
    return WIFI_SUCC;
}

static int32_t wifi_apply_ap_config(wifi_sys_cfg_t *syscfg, wifi_config_t *config, wifi_config_ext_t *config_ext)
{
    if( NULL != config ){
        os_memcpy(syscfg->ap_ssid, config->ap_config.ssid, WIFI_MAX_LENGTH_OF_SSID);
        if( WIFI_MAX_LENGTH_OF_SSID < config->ap_config.ssid_length ){
            //config->ap_config.ssid_length = WIFI_MAX_LENGTH_OF_SSID;
            LOG_E(wifi, "ap_config.ssid_length is longer than %d. initial aborted!", WIFI_MAX_LENGTH_OF_SSID);
            return WIFI_FAIL;
        }
        syscfg->ap_ssid_len = config->ap_config.ssid_length;

        os_memcpy(syscfg->ap_wpa_psk, config->ap_config.password, WIFI_LENGTH_PASSPHRASE);
        if( WIFI_MAX_LENGTH_OF_SSID < config->ap_config.password_length ){
            //config->ap_config.password_length = WIFI_MAX_LENGTH_OF_SSID;
            LOG_E(wifi, "ap_config.password_length is longer than %d. initial aborted!", WIFI_LENGTH_PASSPHRASE);
            return WIFI_FAIL;
        }
        syscfg->ap_wpa_psk_len = config->ap_config.password_length;

        syscfg->ap_auth_mode = config->ap_config.auth_mode;

        syscfg->ap_encryp_type = config->ap_config.encrypt_type;

        syscfg->ap_channel = config->ap_config.channel;

        syscfg->ap_bw = config->ap_config.bandwidth;

        syscfg->ap_ht_ext_ch = (WIFI_BANDWIDTH_EXT_40MHZ_UP == config->ap_config.bandwidth_ext) ? 1 : 3;
    }

    /* save extension config */
    if( NULL!= config_ext ){
        wifi_save_ap_ext_config(syscfg, config_ext);
    }
    return WIFI_SUCC;
}

static int32_t wifi_apply_repeater_config(wifi_sys_cfg_t *syscfg, wifi_config_t *config, wifi_config_ext_t *config_ext)
{
    if( WIFI_SUCC != wifi_apply_sta_config(syscfg, config, config_ext)){
        return WIFI_FAIL;
    }

    if( WIFI_SUCC != wifi_apply_ap_config(syscfg, config, config_ext) ){
        return WIFI_FAIL;
    }
    return WIFI_SUCC;
}

static int32_t wifi_apply_p2p_config(wifi_sys_cfg_t *syscfg, wifi_config_t *config, wifi_config_ext_t *config_ext)
{
    /* TBD */
    return WIFI_SUCC;
}
int8_t wifi_sys_cfg_from = 0; // 1 from NVDM, 2 from hard code default

#ifdef MTK_WIFI_PROFILE_ENABLE

#if 0
#ifdef MTK_SINGLE_SKU_SUPPORT
/*need improve,  if the NVDM is disable in project,  it will build error here!*/
void wifi_get_single_sku_from_nvdm(singleSKU2G_t * single, uint8_t group_number)
{
    //uint8_t buff_ext[1024];
    //uint32_t len = sizeof(buff_ext);
    uint32_t len = PROFILE_BUF_SKU_LEN;
    uint8_t  *buff_ext = NULL;
    buff_ext = wifi_os_malloc(len);

    if( NULL==buff_ext ){
        LOG_E(wifi,"allocate buffer fail.\n");
        return;
    }

    switch(group_number) {
        case 0:
            nvdm_read_data_item("common", "SingleSKU2G_0", buff_ext, &len);
            break;

        default:
            printf("deafult only support group 0\n");
            nvdm_read_data_item("common", "SingleSKU2G_0", buff_ext, &len);
            break;
    }
    wifi_conf_get_pwr_from_str(sizeof(singleSKU2G_t), single, (char *)buff_ext);
    wifi_os_free(buff_ext);
}
#endif/*MTK_SINGLE_SKU_SUPPORT*/
#endif

#define NVDM_TO_BUFF(group, item, buff, len_out) \
    (len_out) = sizeof(buff);                    \
    if (NVDM_STATUS_OK == nvdm_read_data_item((group), (item), buff, &(len_out)))

/**
 * @brief: Convert the value read from nvdm to int type and store to target
 *
 * @param[out] target, the result stores to
 * @param[in]  group, nvdm group name string
 * @param[in]  item, nvdm item name string
 * @param[in]  buff, temp buff to store values read from nvdm
 * @param[out] len_out, how many bytes read from nvdm
 *
 * @note The target value ranges from 0 to 65535
 *
 * @return
 */
#define NVDM_TO_VALUE_INTEGER(target, group, item, buff, len_out) do{ \
    NVDM_TO_BUFF(group, item, buff, len_out) {                        \
        (target) = (uint16_t)atoi((char *)(buff));                    \
    }                                                                 \
}while(0)

/**
 * @brief: Get raw value from nvdm and copy len_copy bytes to target
 *
 * @param[out] target, the result stores to
 * @param[in]  group, nvdm group name string
 * @param[in]  item, nvdm item name string
 * @param[in]  buff, temp buff to store values read from nvdm
 * @param[in]  len_copy, how many bytes copied from buff to target
 * @param[out] len_out, how many bytes read from nvdm
 *
 * @return
 */
#define NVDM_TO_VALUE_RAW(target, group, item, buff, len_copy, len_out) do{ \
    NVDM_TO_BUFF(group, item, buff, len_out) {                              \
        os_memcpy((target), buff, (len_copy));                              \
    }                                                                       \
}while(0)

extern void wifi_conf_get_pwr_from_str(uint16_t ret_len, void * single, const char *pwr_src);
static void wifi_get_config_from_nvdm(wifi_sys_cfg_t *config)
{
    // init wifi profile
    uint8_t buff[PROFILE_BUF_LEN]= {0};
    uint32_t len = sizeof(buff);
    wifi_sys_cfg_from = 1; // From nvdm
    // common
    NVDM_TO_VALUE_INTEGER(config->opmode, "common", "OpMode", buff, len);
    NVDM_TO_VALUE_INTEGER(config->country_region, "common", "CountryRegion", buff, len);
    NVDM_TO_VALUE_INTEGER(config->country_region_a_band, "common", "CountryRegionABand", buff, len);

    NVDM_TO_VALUE_RAW(config->country_code, "common", "CountryCode", buff, 4, len);

    wifi_country_code_region_mapping(config->country_code, &(config->country_region), &(config->country_region_a_band));

    NVDM_TO_VALUE_INTEGER(config->radio_off, "common", "RadioOff", buff, len);
    NVDM_TO_VALUE_INTEGER(config->rts_threshold, "common", "RTSThreshold", buff, len);
    NVDM_TO_VALUE_INTEGER(config->frag_threshold, "common", "FragThreshold", buff, len);
    NVDM_TO_VALUE_INTEGER(config->dbg_level, "common", "DbgLevel", buff, len);

    len = sizeof(buff);
    nvdm_read_data_item("network", "IpAddr", buff, &len);
    wifi_conf_get_ip_from_str(config->ap_ip_addr, (char *)buff);
    wifi_conf_get_ip_from_str(config->sta_ip_addr, (char *)buff);
#ifdef MTK_ANT_DIV_ENABLE
    NVDM_TO_VALUE_INTEGER(config->antenna_number, "common", "AntennaNumber", buff, len);
    NVDM_TO_VALUE_INTEGER(config->antenna_diversity_enable, "common", "AntDiversityEnable", buff, len);
    NVDM_TO_VALUE_INTEGER(config->antenna_probe_req_count, "common", "AntProbeReqCount", buff, len);
    NVDM_TO_VALUE_INTEGER(config->antenna_threshold_level, "common", "AntThresholdRssi", buff, len);
    NVDM_TO_VALUE_INTEGER(config->antenna_delta_rssi, "common", "AntDeltaRssi", buff, len);
    NVDM_TO_VALUE_INTEGER(config->antenna_rssi_switch, "common", "AntRssiSwitch", buff, len);
    NVDM_TO_VALUE_INTEGER(config->antenna_pre_selected_rssi, "common", "AntPreSelectedRssi", buff, len);
#endif
    NVDM_TO_VALUE_INTEGER(config->sta_local_admin_mac, "STA", "LocalAdminMAC", buff, len);
    if (wifi_config_get_mac_address(WIFI_PORT_STA, config->sta_mac_addr) < 0) {
        LOG_E(wifi, "Wi-Fi get MAC address fail.");
        return;
    }

    NVDM_TO_VALUE_INTEGER(config->sta_ssid_len, "STA", "SsidLen", buff, len);
    if(config->sta_ssid_len > sizeof(config->sta_ssid)) {
        LOG_E(wifi, "Invalid length of STA SSID: %d", config->sta_ssid_len);
    }

    NVDM_TO_VALUE_RAW(config->sta_ssid, "STA", "Ssid", buff, config->sta_ssid_len, len);

    NVDM_TO_VALUE_INTEGER(config->sta_bss_type, "STA", "BssType", buff, len);
    NVDM_TO_VALUE_INTEGER(config->sta_channel, "STA", "Channel", buff, len);
    NVDM_TO_VALUE_INTEGER(config->sta_bw, "STA", "BW", buff, len);

    len = sizeof(buff);
    nvdm_read_data_item("STA", "WirelessMode", buff, &len);
    if (wifi_5g_support() < 0) {
        config->sta_wireless_mode = (uint8_t)wifi_change_wireless_mode_5g_to_2g((wifi_phy_mode_t)atoi((char *)buff));
    }else {
        config->sta_wireless_mode = (uint8_t)atoi((char *)buff);
    }

    NVDM_TO_VALUE_INTEGER(config->sta_ba_decline, "STA", "BADecline", buff, len);
    NVDM_TO_VALUE_INTEGER(config->sta_auto_ba, "STA", "AutoBA", buff, len);
    NVDM_TO_VALUE_INTEGER(config->sta_ht_mcs, "STA", "HT_MCS", buff, len);
    NVDM_TO_VALUE_INTEGER(config->sta_ht_ba_win_size, "STA", "HT_BAWinSize", buff, len);
    NVDM_TO_VALUE_INTEGER(config->sta_ht_gi, "STA", "HT_GI", buff, len);
    NVDM_TO_VALUE_INTEGER(config->sta_ht_protect, "STA", "HT_PROTECT", buff, len);
    NVDM_TO_VALUE_INTEGER(config->sta_ht_ext_ch, "STA", "HT_EXTCHA", buff, len);
    NVDM_TO_VALUE_INTEGER(config->sta_wmm_capable, "STA", "WmmCapable", buff, len);
    NVDM_TO_VALUE_INTEGER(config->sta_listen_interval, "STA", "ListenInterval", buff, len);
    NVDM_TO_VALUE_INTEGER(config->sta_auth_mode, "STA", "AuthMode", buff, len);
    NVDM_TO_VALUE_INTEGER(config->sta_encryp_type, "STA", "EncrypType", buff, len);
    NVDM_TO_VALUE_INTEGER(config->sta_wpa_psk_len, "STA", "WpaPskLen", buff, len);

    NVDM_TO_VALUE_RAW(config->sta_wpa_psk, "STA", "WpaPsk", buff, config->sta_wpa_psk_len, len);

    // TODO: How to save binary PMK value not ending by ' ; ' ?
    NVDM_TO_VALUE_RAW(config->sta_pmk, "STA", "PMK", buff, 32, len);

    NVDM_TO_VALUE_INTEGER(config->sta_pair_cipher, "STA", "PairCipher", buff, len);
    NVDM_TO_VALUE_INTEGER(config->sta_group_cipher, "STA", "GroupCipher", buff, len);
    NVDM_TO_VALUE_INTEGER(config->sta_default_key_id, "STA", "DefaultKeyId", buff, len);
    NVDM_TO_VALUE_INTEGER(config->sta_ps_mode, "STA", "PSMode", buff, len);
    NVDM_TO_VALUE_INTEGER(config->sta_keep_alive_period, "STA", "KeepAlivePeriod", buff, len);
    NVDM_TO_VALUE_INTEGER(config->beacon_lost_time, "STA", "BeaconLostTime", buff, len);
    NVDM_TO_VALUE_INTEGER(config->apcli_40mhz_auto_upbelow, "STA", "ApcliBWAutoUpBelow", buff, len);

    // AP
#ifdef MTK_WIFI_REPEATER_ENABLE
    if (config->opmode == WIFI_MODE_REPEATER) {
        NVDM_TO_VALUE_INTEGER(config->ap_channel, "STA", "Channel", buff, len);
        NVDM_TO_VALUE_INTEGER(config->ap_bw, "STA", "BW", buff, len);
        NVDM_TO_VALUE_INTEGER(config->ap_wireless_mode, "STA", "WirelessMode", buff, len);
    } else {
#endif
        /* Use STA MAC/IP as AP MAC/IP for the time being, due to N9 dual interface not ready yet */
        NVDM_TO_VALUE_INTEGER(config->ap_channel, "AP", "Channel", buff, len);
        NVDM_TO_VALUE_INTEGER(config->ap_bw, "AP", "BW", buff, len);

        len = sizeof(buff);
        nvdm_read_data_item("AP", "WirelessMode", buff, &len);
        if (wifi_5g_support() < 0) {
            config->ap_wireless_mode = (uint8_t)wifi_change_wireless_mode_5g_to_2g((wifi_phy_mode_t)atoi((char *)buff));
        }else {
            config->ap_wireless_mode = (uint8_t)atoi((char *)buff);
        }
#ifdef MTK_WIFI_REPEATER_ENABLE
    }
#endif /* MTK_WIFI_REPEATER_ENABLE */
    NVDM_TO_VALUE_INTEGER(config->ap_local_admin_mac, "AP", "LocalAdminMAC", buff, len);
    if (wifi_config_get_mac_address(WIFI_PORT_AP, config->ap_mac_addr) < 0) {
        LOG_E(wifi, "Wi-Fi get MAC address fail.");
        return;
    }
    NVDM_TO_VALUE_INTEGER(config->ap_ssid_len, "AP", "SsidLen", buff, len);
    if(config->ap_ssid_len > sizeof(config->ap_ssid)) {
        LOG_E(wifi, "Invalid length of AP SSID: %d", config->ap_ssid_len);
    }

    NVDM_TO_VALUE_RAW(config->ap_ssid, "AP", "Ssid", buff, config->ap_ssid_len, len);

    NVDM_TO_VALUE_INTEGER(config->ap_auto_ba, "AP", "AutoBA", buff, len);
    NVDM_TO_VALUE_INTEGER(config->ap_ht_mcs, "AP", "HT_MCS", buff, len);
    NVDM_TO_VALUE_INTEGER(config->ap_ht_ba_win_size, "AP", "HT_BAWinSize", buff, len);
    NVDM_TO_VALUE_INTEGER(config->ap_ht_gi, "AP", "HT_GI", buff, len);
    NVDM_TO_VALUE_INTEGER(config->ap_ht_protect, "AP", "HT_PROTECT", buff, len);
    NVDM_TO_VALUE_INTEGER(config->ap_ht_ext_ch, "AP", "HT_EXTCHA", buff, len);
    NVDM_TO_VALUE_INTEGER(config->ap_wmm_capable, "AP", "WmmCapable", buff, len);
    NVDM_TO_VALUE_INTEGER(config->ap_dtim_period, "AP", "DtimPeriod", buff, len);
    NVDM_TO_VALUE_INTEGER(config->ap_hide_ssid, "AP", "HideSSID", buff, len);
    NVDM_TO_VALUE_INTEGER(config->ap_auto_channel_select, "AP", "AutoChannelSelect", buff, len);
    NVDM_TO_VALUE_INTEGER(config->ap_auth_mode, "AP", "AuthMode", buff, len);
    NVDM_TO_VALUE_INTEGER(config->ap_encryp_type, "AP", "EncrypType", buff, len);
    NVDM_TO_VALUE_INTEGER(config->ap_wpa_psk_len, "AP", "WpaPskLen", buff, len);

    NVDM_TO_VALUE_RAW(config->ap_wpa_psk, "AP", "WpaPsk", buff, config->ap_wpa_psk_len, len);

    // TODO: How to save binary PMK value not ending by ' ; ' ?
    NVDM_TO_VALUE_RAW(config->ap_pmk, "AP", "PMK", buff, 32, len);

    NVDM_TO_VALUE_INTEGER(config->ap_pair_cipher, "AP", "PairCipher", buff, len);
    NVDM_TO_VALUE_INTEGER(config->ap_group_cipher, "AP", "GroupCipher", buff, len);
    NVDM_TO_VALUE_INTEGER(config->ap_default_key_id, "AP", "DefaultKeyId", buff, len);
    NVDM_TO_VALUE_INTEGER(config->ap_beacon_disable, "AP", "BcnDisEn", buff, len);

    // scan channel table and regulatory table
    len = sizeof(buff);
    nvdm_read_data_item("common", "BGChannelTable", buff, &len);
    config->bg_band_entry_num = wifi_conf_get_ch_table_from_str(config->bg_band_triple, 10, (char *)buff, os_strlen((char *)buff));

    len = sizeof(buff);
    nvdm_read_data_item("common", "AChannelTable", buff, &len);
    config->a_band_entry_num = wifi_conf_get_ch_table_from_str(config->a_band_triple, 10, (char *)buff, os_strlen((char *)buff));

    config->forwarding_zero_copy = 1;

#ifdef MTK_WIFI_CONFIGURE_FREE_ENABLE
    /* These are for MBSS support, but not exist trunk (it's customer feature), however,
            we have to add them here due to N9 FW has them (only one version of N9 FW)
         */
    // TODO: How to solve it in the future...Michael
    config->mbss_enable = 0;
    os_memset(config->mbss_ssid1, 0x0, sizeof(config->mbss_ssid1));;
    config->mbss_ssid_len1 = 0;
    os_memset(config->mbss_ssid2, 0x0, sizeof(config->mbss_ssid2));;
    config->mbss_ssid_len2 = 0;

    NVDM_TO_VALUE_INTEGER(config->config_free_ready, "config_free", "ConfigFree_Ready", buff, len);
    NVDM_TO_VALUE_INTEGER(config->config_free_enable, "config_free", "ConfigFree_Enable", buff, len);

#endif /* MTK_WIFI_CONFIGURE_FREE_ENABLE */
    NVDM_TO_VALUE_INTEGER(config->sta_fast_link, "common", "StaFastLink", buff, len);

#ifdef MTK_WIFI_PRIVILEGE_ENABLE
    NVDM_TO_VALUE_INTEGER(config->wifi_privilege_enable, "common", "WiFiPrivilegeEnable", buff, len);
#else
    config->wifi_privilege_enable = 0;
#endif
    NVDM_TO_VALUE_INTEGER(config->sta_keep_alive_packet, "STA", "StaKeepAlivePacket", buff, len);

#ifdef MTK_SINGLE_SKU_SUPPORT
    uint32_t len_sku = PROFILE_BUF_SKU_LEN;
    uint8_t  *buff_sku = NULL;
    buff_sku = (uint8_t *)wifi_os_malloc(len_sku);
    if( NULL==buff_sku ){
        LOG_E(wifi,"allocate buffer fail.\n");
        return;
    }

    len = len_sku;
    nvdm_read_data_item("common", "SupportSingleSKU", buff_sku, &len);
    config->support_single_sku= (uint8_t)atoi((char *)buff_sku);
    os_memset(buff_sku,0x00,len_sku);
#if 0
    len = len_sku;
    nvdm_read_data_item("common", "SingleSKU_index", buff_sku, &len);

    if((uint8_t)atoi((char *)buff_sku) == 0) {
        wifi_get_single_sku_from_nvdm(&config->single_sku_2g, 0);
    } else {
        LOG_E(wifi, "only support one group default");
    }
#else
    len = len_sku;
    nvdm_read_data_item("common", "SingleSKU2G_0", buff_sku, &len);
    wifi_conf_get_pwr_from_str(sizeof(singleSKU2G_t), &(config->single_sku_2g), (char *)buff_sku);
#endif
    os_memset(buff_sku,0x00,len_sku);

    len = len_sku;
    nvdm_read_data_item("common", "SingleSKU5G_L0", buff_sku, &len);
    wifi_conf_get_pwr_from_str(WIFI_5G_LOW_CHANNEL_SKU_LEN, &(config->single_sku_5g), (char *)buff_sku);
    os_memset(buff_sku,0x00,len_sku);

    len = len_sku;
    nvdm_read_data_item("common", "SingleSKU5G_M0", buff_sku, &len);
    wifi_conf_get_pwr_from_str(WIFI_5G_MIDDLE_CHANNEL_SKU_LEN, (char *)&(config->single_sku_5g) + WIFI_5G_LOW_CHANNEL_SKU_LEN, (char *)buff_sku);
    os_memset(buff_sku,0x00,len_sku);

    len = len_sku;
    nvdm_read_data_item("common", "SingleSKU5G_H0", buff_sku, &len);
    wifi_conf_get_pwr_from_str(WIFI_5G_HIGH_CHANNEL_SKU_LEN, (char *)&(config->single_sku_5g) + (WIFI_5G_LOW_CHANNEL_SKU_LEN + WIFI_5G_MIDDLE_CHANNEL_SKU_LEN), (char *)buff_sku);
    wifi_os_free(buff_sku);
#endif

}
#endif
static int32_t wifi_apply_user_config(wifi_sys_cfg_t *syscfg, wifi_config_t *config, wifi_config_ext_t *config_ext)
{
    int32_t ret = WIFI_SUCC;

    if(NULL!=config){
        if ( wifi_is_opmode_valid(config->opmode) ) {
            syscfg->opmode = config->opmode;
        }else{
            LOG_E(wifi, "User config opmode not right, default use STA");
            syscfg->opmode = WIFI_MODE_STA_ONLY;
        }
    }
    if ( config_ext!= NULL && config_ext->country_code_present) {
        os_memcpy(syscfg->country_code, config_ext->country_code, 4);
        if( WIFI_SUCC != wifi_country_code_region_mapping(syscfg->country_code, &(syscfg->country_region), &(syscfg->country_region_a_band)) ){
            LOG_E(wifi, "wifi_country_code_region_mapping fail. initial aborted!");
            return WIFI_FAIL;
        }
    }
    if (WIFI_MODE_STA_ONLY == syscfg->opmode) {
        ret = wifi_apply_sta_config(syscfg, config, config_ext);
    } else if (WIFI_MODE_AP_ONLY == syscfg->opmode) {
        ret = wifi_apply_ap_config(syscfg, config, config_ext);
    } else if (WIFI_MODE_REPEATER == syscfg->opmode) {
        ret = wifi_apply_repeater_config(syscfg, config, config_ext);
    } else if (WIFI_MODE_P2P_ONLY == syscfg->opmode) {
        ret = wifi_apply_p2p_config(syscfg, config, config_ext);
    } else {
        /* no configuration is required for Monitor Mode */
    }
    return ret;
}

/**
* @brief build the whole configurations
*/
static int32_t wifi_build_whole_config(wifi_sys_cfg_t *syscfg, wifi_config_t *config, wifi_config_ext_t *config_ext)
{
    if (0 != wifi_get_default_config(syscfg)) {
        LOG_E(wifi, "wifi_get_default_config fail. initial aborted!");
        return -1;
    }

#ifdef MTK_WIFI_PROFILE_ENABLE
    wifi_get_config_from_nvdm(syscfg);
#endif

    /* both config and config_ext are NULL, it will not apply user config, */
    if( (NULL!=config) || (NULL!=config_ext) ){
        if( WIFI_SUCC != wifi_apply_user_config(syscfg, config, config_ext) ){
            LOG_E(wifi, "wifi_apply_user_config fail. initial aborted!");
            return -1;
        }
    }
    return 0;
}

static void wifi_build_config_from_sys_cfg(wifi_config_t *config, wifi_sys_cfg_t *syscfg )
{
    config->opmode = syscfg->opmode;

    /*sta config*/
    os_memcpy(config->sta_config.ssid, syscfg->sta_ssid, WIFI_MAX_LENGTH_OF_SSID);
    config->sta_config.ssid_length = syscfg->sta_ssid_len;
    os_memcpy(config->sta_config.password, syscfg->sta_wpa_psk, WIFI_LENGTH_PASSPHRASE);
    config->sta_config.password_length = syscfg->sta_wpa_psk_len;

    /*ap config*/
    os_memcpy(config->ap_config.ssid, syscfg->ap_ssid, WIFI_MAX_LENGTH_OF_SSID);
    config->ap_config.ssid_length = syscfg->ap_ssid_len;
    os_memcpy(config->ap_config.password, syscfg->ap_wpa_psk, WIFI_LENGTH_PASSPHRASE);
    config->ap_config.password_length = syscfg->ap_wpa_psk_len;
    config->ap_config.auth_mode = (wifi_auth_mode_t)syscfg->ap_auth_mode;
    config->ap_config.encrypt_type = (wifi_encrypt_type_t)syscfg->ap_encryp_type;
    config->ap_config.channel = syscfg->ap_channel;
    config->ap_config.bandwidth = syscfg->ap_bw;
    config->ap_config.bandwidth_ext = (syscfg->ap_ht_ext_ch == 1) ? WIFI_BANDWIDTH_EXT_40MHZ_UP : WIFI_BANDWIDTH_EXT_40MHZ_BELOW;
}

static void wifi_build_config_ext_from_sys_cfg(wifi_config_ext_t *config_ext, wifi_sys_cfg_t *syscfg)
{
    os_memcpy(config_ext->country_code, syscfg->country_code, 4);
    config_ext->country_code_present = 1;
    wifi_country_code_region_mapping(syscfg->country_code, &(syscfg->country_region), &(syscfg->country_region_a_band));

    /*sta config_ext*/
    config_ext->sta_wep_key_index = syscfg->sta_default_key_id;
    config_ext->sta_wep_key_index_present = 1;
    config_ext->sta_bandwidth = syscfg->sta_bw;
    config_ext->sta_bandwidth_present = 1;
    config_ext->sta_wireless_mode = (wifi_phy_mode_t)syscfg->sta_wireless_mode;
    config_ext->sta_wireless_mode_present = 1;
    config_ext->sta_listen_interval = syscfg->sta_listen_interval;
    config_ext->sta_listen_interval_present = 1;
    config_ext->sta_power_save_mode = (wifi_power_saving_mode_t)syscfg->sta_ps_mode;
    config_ext->sta_power_save_mode_present = 1;

    /*ap config_ext*/
    config_ext->ap_wep_key_index = syscfg->ap_default_key_id;
    config_ext->ap_wep_key_index_present = 1;
    config_ext->ap_hidden_ssid_enable = syscfg->ap_hide_ssid;
    config_ext->ap_hidden_ssid_enable_present = 1;
    config_ext->ap_wireless_mode = (wifi_phy_mode_t)syscfg->ap_wireless_mode;
    config_ext->ap_wireless_mode_present = 1;
    config_ext->ap_dtim_interval = syscfg->ap_dtim_period;
    config_ext->ap_dtim_interval_present = 1;
}

void wifi_sys_cfg_init(wifi_sys_cfg_t *config)
{
     return;
}

extern uint8_t ch_table_5g[42];
extern void hex_dump(char *str, unsigned char *pSrcBufVA, unsigned int SrcBufLen);
/**
* @brief Example of wifi_sys_cfg struct show, it is also the profile download to N9(all config afetr wifi_init)
*
* @note Call this function should be after wifi_build_whole_config() in wifi_init()
*/
static int8_t wifi_sys_cfg_show(wifi_sys_cfg_t *psyscfg)
{
#ifdef MTK_WIFI_SYS_CFG_SHOW_ENABLE
    wifi_sys_cfg_t *wifi_profile = NULL;
	if( NULL==psyscfg ){
		LOG_E(wifi,"a");
		return WIFI_FAIL;
	}	
    wifi_profile = psyscfg;
	if(1==wifi_sys_cfg_from){
		printf("From: NVDM\n");
	}else if(2==wifi_sys_cfg_from){
    	printf("From: DEFAULT\n");
	}else{
    	printf("From: Unknown\n");
	}
	// COMMON
    printf("[COMMON]\n");
    printf("\tOpMode = %d\n", wifi_profile->opmode);
    printf("\tCountryRegion = %d\n", wifi_profile->country_region);
    printf("\tCountryRegionABand = %d\n", wifi_profile->country_region_a_band);
    printf("\tCountryCode = %s\n", wifi_profile->country_code);
    printf("\tRadioOff = %d\n", wifi_profile->radio_off);
    printf("\tRTSThreshold = %d\n", wifi_profile->rts_threshold);
    printf("\tFragThreshold = %d\n", wifi_profile->frag_threshold);
    printf("\tDbgLevel = %d\n", wifi_profile->dbg_level);
    printf("\tAp_Beacon_Disable = %d\n", wifi_profile->ap_beacon_disable);
    printf("\tForwardingZeroCopy = %d\n", wifi_profile->forwarding_zero_copy);
    // STA
    printf("[STA]\n");
    printf("\tLocalAdminMAC = %d\n", wifi_profile->sta_local_admin_mac);
    printf("\tIpAddr = %d.%d.%d.%d\n",
           wifi_profile->sta_ip_addr[0],
           wifi_profile->sta_ip_addr[1],
           wifi_profile->sta_ip_addr[2],
           wifi_profile->sta_ip_addr[3]);
    printf("\tMacAddr = %02x:%02x:%02x:%02x:%02x:%02x\n",
           wifi_profile->sta_mac_addr[0],
           wifi_profile->sta_mac_addr[1],
           wifi_profile->sta_mac_addr[2],
           wifi_profile->sta_mac_addr[3],
           wifi_profile->sta_mac_addr[4],
           wifi_profile->sta_mac_addr[5]);
    printf("\tSsidLen = %d\n", wifi_profile->sta_ssid_len);
    printf("\tSsid = %s\n", wifi_profile->sta_ssid);
    printf("\tBssType = %d\n", wifi_profile->sta_bss_type);
    printf("\tChannel = %d\n", wifi_profile->sta_channel);
    printf("\tBW = %d\n", wifi_profile->sta_bw);
    printf("\tWirelessMode = %d\n", wifi_profile->sta_wireless_mode);
    printf("\tBADecline = %d\n", wifi_profile->sta_ba_decline);
    printf("\tAutoBA = %d\n", wifi_profile->sta_auto_ba);
    printf("\tHT_MCS = %d\n", wifi_profile->sta_ht_mcs);
    printf("\tHT_BAWinSize = %d\n", wifi_profile->sta_ht_ba_win_size);
    printf("\tHT_GI = %d\n", wifi_profile->sta_ht_gi);
    printf("\tHT_PROTECT = %d\n", wifi_profile->sta_ht_protect);
    printf("\tHT_EXTCHA = %d\n", wifi_profile->sta_ht_ext_ch);
    printf("\tWmmCapable = %d\n", wifi_profile->sta_wmm_capable);
    printf("\tListenInterval = %d\n", wifi_profile->sta_listen_interval);
    printf("\tAuthMode = %d\n", wifi_profile->sta_auth_mode);
    printf("\tEncrypType = %d\n", wifi_profile->sta_encryp_type);
    printf("\tWpaPskLen = %d\n", wifi_profile->sta_wpa_psk_len);
    printf("\tPairCipher = %d\n", wifi_profile->sta_pair_cipher);
    printf("\tGroupCipher = %d\n", wifi_profile->sta_group_cipher);
    printf("\tDefaultKeyId = %d\n", wifi_profile->sta_default_key_id);
    printf("\tPSMode = %d\n", wifi_profile->sta_ps_mode);
    printf("\tKeepAlivePeriod = %d\n", wifi_profile->sta_keep_alive_period);

    hex_dump("WpaPsk", wifi_profile->sta_wpa_psk, sizeof(wifi_profile->sta_wpa_psk));
    hex_dump("PMK", wifi_profile->sta_pmk, sizeof(wifi_profile->sta_pmk));

    // AP
    printf("[AP]\n");
    printf("\tLocalAdminMAC = %d\n", wifi_profile->ap_local_admin_mac);
    printf("\tIpAddr = %d.%d.%d.%d\n",
           wifi_profile->ap_ip_addr[0],
           wifi_profile->ap_ip_addr[1],
           wifi_profile->ap_ip_addr[2],
           wifi_profile->ap_ip_addr[3]);
    printf("\tMacAddr = %02x:%02x:%02x:%02x:%02x:%02x\n",
           wifi_profile->ap_mac_addr[0],
           wifi_profile->ap_mac_addr[1],
           wifi_profile->ap_mac_addr[2],
           wifi_profile->ap_mac_addr[3],
           wifi_profile->ap_mac_addr[4],
           wifi_profile->ap_mac_addr[5]);
    printf("\tSsidLen = %d\n", wifi_profile->ap_ssid_len);
    printf("\tSsid = %s\n", wifi_profile->ap_ssid);
    printf("\tChannel = %d\n", wifi_profile->ap_channel);
    printf("\tBW = %d\n", wifi_profile->ap_bw);
    printf("\tWirelessMode = %d\n", wifi_profile->ap_wireless_mode);
    printf("\tAutoBA = %d\n", wifi_profile->ap_auto_ba);
    printf("\tHT_MCS = %d\n", wifi_profile->ap_ht_mcs);
    printf("\tHT_BAWinSize = %d\n", wifi_profile->ap_ht_ba_win_size);
    printf("\tHT_GI = %d\n", wifi_profile->ap_ht_gi);
    printf("\tHT_PROTECT = %d\n", wifi_profile->ap_ht_protect);
    printf("\tHT_EXTCHA = %d\n", wifi_profile->ap_ht_ext_ch);
    printf("\tWmmCapable = %d\n", wifi_profile->ap_wmm_capable);
    printf("\tDtimPeriod = %d\n", wifi_profile->ap_dtim_period);
    printf("\tHideSSID = %d\n", wifi_profile->ap_hide_ssid);
    printf("\tAutoChannelSelect = %d\n", wifi_profile->ap_auto_channel_select);
    printf("\tAuthMode = %d\n", wifi_profile->ap_auth_mode);
    printf("\tEncrypType = %d\n", wifi_profile->ap_encryp_type);
    printf("\tWpaPskLen = %d\n", wifi_profile->ap_wpa_psk_len);
    printf("\tPairCipher = %d\n", wifi_profile->ap_pair_cipher);
    printf("\tGroupCipher = %d\n", wifi_profile->ap_group_cipher);
    printf("\tDefaultKeyId = %d\n", wifi_profile->ap_default_key_id);

    hex_dump("WpaPsk", wifi_profile->ap_wpa_psk, sizeof(wifi_profile->ap_wpa_psk));
    hex_dump("PMK", wifi_profile->ap_pmk, sizeof(wifi_profile->ap_pmk));

    // Others	 
    printf("[Others]\n");
    // scan channel table and regulatory table
    printf("\tBG_Band_Entry_Num:%d\n",wifi_profile->bg_band_entry_num);
    for (uint8_t i = 0; (i < wifi_profile->bg_band_entry_num)&&(i < 10); i++)
        printf("\t\t{%d,%d,%d,%d}\n",
               wifi_profile->bg_band_triple[i].first_channel,
               wifi_profile->bg_band_triple[i].num_of_ch,
               wifi_profile->bg_band_triple[i].channel_prop,
               wifi_profile->bg_band_triple[i].tx_power);
    printf("\tA_Band_Entry_Num:%d\n",wifi_profile->a_band_entry_num);
    for (uint8_t i = 0; (i < wifi_profile->a_band_entry_num)&&(i < 10); i++)
        printf("\t\t{%d,%d,%d,%d}\n",
               wifi_profile->a_band_triple[i].first_channel,
               wifi_profile->a_band_triple[i].num_of_ch,
               wifi_profile->a_band_triple[i].channel_prop,
               wifi_profile->a_band_triple[i].tx_power);
    // MBSS 

    printf("\tconfig_free_ready = %d\n", wifi_profile->config_free_ready);
    printf("\tconfig_free_enable = %d\n", wifi_profile->config_free_enable);
    printf("\tsta_fast_link = %d\n", wifi_profile->sta_fast_link);
    printf("\tbeacon_lost_time = %d\n", wifi_profile->beacon_lost_time);
    printf("\tapcli_40mhz_auto_upbelow = %d\n", wifi_profile->apcli_40mhz_auto_upbelow);
    printf("\twifi_privilege_enable = %d\n", wifi_profile->wifi_privilege_enable);
    printf("\tsta_keep_alive_packet = %d\n", wifi_profile->sta_keep_alive_packet);
	printf("\tsupport_single_sku = %d\n", wifi_profile->support_single_sku);
#ifdef MTK_ANT_DIV_ENABLE
    //antenna
    printf("\tantenna\n");
    printf("\tnumber = %d\n", wifi_profile->antenna_number);
    printf("\tdiversity_enable = %d\n", wifi_profile->antenna_diversity_enable);
    printf("\tprobe_req_count = %d\n", wifi_profile->antenna_probe_req_count);
    printf("\tthreshold_level = %d\n", wifi_profile->antenna_threshold_level);
    printf("\tdelta_rssi = %d\n", wifi_profile->antenna_delta_rssi);
    printf("\trssi_switch = %d\n", wifi_profile->antenna_rssi_switch);
    printf("\tpre_selected_rssi = %d\n", wifi_profile->antenna_pre_selected_rssi);
#endif

    // singleSKU
#ifdef MTK_SINGLE_SKU_SUPPORT
	singleSKU2G_t  *sku_table_2g = NULL;
	singleSKU5G_t  *sku_table_5g = NULL;
	sku_table_2g = &wifi_profile->single_sku_2g;
	sku_table_5g = &wifi_profile->single_sku_5g;
	printf("singleSKU 2G\n");
	for(uint8_t i = 0; i < WIFI_2G_CHANNEL_NUMBER; i++) {
		printf("CH%2d:", i+1);
		for(uint8_t j = 0; j < WIFI_2G_SKU_POWER_PER_CHANNEL; j++){
			printf("%3d", *((char *)sku_table_2g + (i*WIFI_2G_SKU_POWER_PER_CHANNEL+j)));
		}
		//UART will miss some char, need delay between each line.
		vTaskDelay(8);
		printf("\n");
	}
	printf("singleSKU 5G\n");
	//uint8_t ch_table_5g[42] = {36,38,40,42,44,46,48,52,54,56,58,60,62,64,100,102,104,106,108,110,112,116,118,120,122,124,126,128,132,134,136,140,149,151,153,155,157,159,161,165,169,173};
	for(uint8_t i = 0; i < WIFI_5G_CHANNEL_NUMBER; i++) {
		printf("CH%3d:",ch_table_5g[i]);//skip CCK module
		for(uint8_t j = 0; j < WIFI_5G_SKU_POWER_PER_CHANNEL; j++){
			printf("%3d", *((char *)sku_table_5g + (i*WIFI_5G_SKU_POWER_PER_CHANNEL+j)));
		}
		//UART will miss some char, need delay between each line.
		vTaskDelay(8);
		printf("\n");
	}
#endif
	printf("End\n");
#endif
	return 0;
}

#if (PRODUCT_VERSION == 5932)
#ifdef MTK_WIFI_STUB_CONF_ENABLE
static uint32_t *tx_power_bin_addr_from_host = NULL;
void wifi_service_set_tx_power_bin(uint32_t *tx_power_bin)
{
    tx_power_bin_addr_from_host = tx_power_bin;
}
#endif
#endif

void wifi_init(wifi_config_t *config, wifi_config_ext_t *config_ext)
{
    //wifi_sys_cfg_t syscfg = {0};
    wifi_sys_cfg_t *psyscfg = NULL;
    wifi_config_t running_config = {0};
    wifi_config_ext_t running_config_ext = {0};
    wifi_config_t *pconfig = NULL;
    wifi_config_ext_t *pconfig_ext = NULL;
    uint32_t buf_size = sizeof(wifi_sys_cfg_t);
    
    psyscfg = wifi_os_malloc(buf_size);
    if( NULL==psyscfg ){
        LOG_E(wifi, "malloc wifi_sys_cfg_t buf fail.buf_size=%d ", buf_size);
        return;
    }
    os_memset(psyscfg, 0x00, buf_size);

    //os_memset(&syscfg, 0, sizeof(syscfg));
    pconfig = config;
    pconfig_ext = config_ext;
#ifdef MTK_WIFI_STUB_CONF_ENABLE /* For master-slave architecture */
    if(0 == wfc_get_wifi_sys_config(psyscfg)) {
        if( (NULL!=pconfig) || (NULL!=pconfig_ext) ){
            if( WIFI_SUCC != wifi_apply_user_config(psyscfg, pconfig, pconfig_ext) ){
                LOG_E(wifi, "wifi_apply_user_config fail. initial aborted!");
                wifi_os_free(psyscfg);
                return;
            }
        }        
    }else {
        LOG_W(wifi, "wifi build whole config from host fail. initial default value!");
        if (0 != wifi_build_whole_config(psyscfg, pconfig, pconfig_ext)) {
            LOG_E(wifi, "wifi_build_whole_config fail. initial aborted!");
            wifi_os_free(psyscfg);
            return;
        }
    }
#else
    if (0 != wifi_build_whole_config(psyscfg, pconfig, pconfig_ext)) {
        LOG_E(wifi, "wifi_build_whole_config fail. initial aborted!");
        wifi_os_free(psyscfg);
        return;
    }
#endif    
	wifi_sys_cfg_show(psyscfg);
    /*If user have setting params, use user setting, else use default.*/
    /*config is NULL, all config using default syscfg configurations. */
    /*config is not NULL, user not setting params items use default configurations.*/
    if (NULL == pconfig) {
        LOG_I(wifi, "config is null. Will use default config configuration");
    }else{
        os_memcpy(&running_config, config, sizeof(wifi_config_t));
    }
    wifi_build_config_from_sys_cfg(&running_config, psyscfg);
    pconfig = &running_config;

    if (NULL == pconfig_ext) {
        LOG_I(wifi, "config_ext is null. Will use default config_ext configuration");
    }else{
        os_memcpy(&running_config_ext, config_ext, sizeof(wifi_config_ext_t));
    }
    wifi_build_config_ext_from_sys_cfg(&running_config_ext, psyscfg);
    pconfig_ext = &running_config_ext;

#if (PRODUCT_VERSION == 7698 || PRODUCT_VERSION == 7682 || PRODUCT_VERSION == 7686 || PRODUCT_VERSION == 5932)
uint32_t *tx_power_bin_addr = NULL;
#ifdef MTK_WIFI_STUB_CONF_ENABLE
    tx_power_bin_addr = tx_power_bin_addr_from_host;
#else
    tx_power_bin_addr = (uint32_t *)ROM_WIFI_TX_POWER_BASE;
#endif
    wifi_stack_set_tx_power_bin(tx_power_bin_addr);
#endif

#ifdef MTK_ANT_DIV_ENABLE
    /* After HW/patch download, to overwrite the patch configuration */
    //tx_switch_for_antenna,rx_switch_for_antenna is controled in NVDM and include in MTK_WIFI_PROFILE_ENABLE
    if(psyscfg->antenna_diversity_enable != 0) {
        ant_diversity_init(psyscfg->antenna_number);
        //printf("tx_switch_for_antenna=%d,rx_switch_for_antenna=%d",syscfg.tx_switch_for_antenna,syscfg.rx_switch_for_antenna);
    }
    //printf_ant_diversity(10);
#endif

    wifi_channel_list_init(psyscfg);
#if defined(MTK_WIFI_ROM_ENABLE) && !defined(MTK_WIFI_SLIM_ENABLE)
    connsys_set_wifi_profile(psyscfg);
    // connsys_init(); /* moved to  system_init() for early N9 FW download */
#else
    connsys_init(psyscfg);
#endif
    wifi_init_register_callback();

    wifi_scan_init(pconfig);
    inband_queue_init();
    NetTaskInit();

   /*Fix if SSID length = 0, switch to repeater mode from station mode, N9 will assert*/
    if(pconfig->ap_config.ssid_length == 0) {
        pconfig->ap_config.ssid_length = psyscfg->ap_ssid_len;
        os_memcpy(pconfig->ap_config.ssid, psyscfg->ap_ssid, WIFI_MAX_LENGTH_OF_SSID);
    } else if(pconfig->sta_config.ssid_length == 0) {
        pconfig->sta_config.ssid_length = psyscfg->sta_ssid_len;
        os_memcpy(pconfig->sta_config.ssid, psyscfg->sta_ssid, WIFI_MAX_LENGTH_OF_SSID);
    }
#ifdef MTK_MINISUPP_ENABLE
    wpa_supplicant_task_init(pconfig, pconfig_ext);
#endif

#ifdef MTK_CM4_WIFI_TASK_ENABLE
    wifi_firmware_init(pconfig, pconfig_ext,psyscfg);
    if (wifi_firmware_task_start(pconfig) != 0) {
        LOG_E(wifi, "failed to start firmware Wi-Fi task");
    }
#endif
    wifi_os_free(psyscfg);
}


static bool wifi_security_valid = false;
bool wifi_get_security_valid(void)
{
    return wifi_security_valid;
}

void wifi_set_security_valid(bool value)
{
    wifi_security_valid = value;
    return;
}

/***************** Just for internal use **********************/
#include "hal_sleep_manager.h"
#ifdef HAL_SLEEP_MANAGER_ENABLED
#define WIFI_MAX_SLEEP_HANDLE  32   // This define value should equal to MAX_SLEEP_HANDLE in hal_sleep_driver.h
uint8_t locks[WIFI_MAX_SLEEP_HANDLE];
#endif
uint8_t wifi_set_sleep_handle(const char *handle_name)
{
#ifdef HAL_SLEEP_MANAGER_ENABLED
    return hal_sleep_manager_set_sleep_handle(handle_name);
#else
    return 0xff;
#endif
}

int32_t wifi_lock_sleep(uint8_t handle_index)
{
#ifdef HAL_SLEEP_MANAGER_ENABLED
    locks[handle_index] = 1;
    return hal_sleep_manager_lock_sleep(handle_index);
#else
    return -1;
#endif
}

int32_t wifi_unlock_sleep(uint8_t handle_index)
{
#ifdef HAL_SLEEP_MANAGER_ENABLED
    locks[handle_index] = 0;
    return hal_sleep_manager_unlock_sleep(handle_index);
#else
    return -1;
#endif
}

int32_t wifi_unlock_sleep_all(void)
{
#ifdef HAL_SLEEP_MANAGER_ENABLED
    int i = 0;
    for (i = 0; i < WIFI_MAX_SLEEP_HANDLE; i++) {
        if (locks[i] == 1) {
            hal_sleep_manager_unlock_sleep(i);
        }
    }
    return 0;
#else
    return -1;
#endif
}

int32_t wifi_sleep_manager_get_lock_status(void)
{
#ifdef HAL_SLEEP_MANAGER_ENABLED
    return hal_sleep_manager_get_lock_status();
#else
    return -1;
#endif
}

int32_t wifi_release_sleep_handle(uint8_t handle)
{
#ifdef HAL_SLEEP_MANAGER_ENABLED
    return hal_sleep_manager_release_sleep_handle(handle);
#else
    return -1;
#endif
}

int32_t wifi_get_pmk_by_ssid_psk_from_pmkinfo(uint8_t *pmk,uint8_t *ssid,uint8_t ssid_len, uint8_t *psk,uint8_t psk_len)
{
#ifdef MTK_NVDM_ENABLE
//if ((0 != ssid->auth_mode) && ((0 != ssid->encr_type) || (1 != ssid->encr_type))) {
    uint8_t pmk_info[WIFI_MAX_LENGTH_OF_SSID+WIFI_LENGTH_PASSPHRASE+32] = {0};
    uint8_t pmk_info_zero[WIFI_MAX_LENGTH_OF_SSID+WIFI_LENGTH_PASSPHRASE+32] = {0};
    uint32_t pmk_info_len = sizeof(pmk_info);
    nvdm_read_data_item(WIFI_PROFILE_BUFFER_STA, "PMK_INFO",
                       (uint8_t *)pmk_info, &pmk_info_len);
    //hex_dump(" STA pmk_info", pmk_info, 128);
	if ((0 == os_memcmp(pmk_info, ssid, ssid_len)) &&
        (0 == os_memcmp(pmk_info+WIFI_MAX_LENGTH_OF_SSID, psk, psk_len)) &&
        (0 == os_memcmp(pmk_info+ssid_len, pmk_info_zero, WIFI_MAX_LENGTH_OF_SSID - ssid_len)) &&
        (0 == os_memcmp(pmk_info+WIFI_MAX_LENGTH_OF_SSID+psk_len, pmk_info_zero, WIFI_LENGTH_PASSPHRASE - psk_len))) {
            os_memcpy(pmk, pmk_info+WIFI_MAX_LENGTH_OF_SSID+WIFI_LENGTH_PASSPHRASE, 32);
			LOG_I(wifi, "pmkinfo STA same \r\n");
            return 0;
    } else
#endif
    {
        return -1;
    }
}


int32_t wifi_get_pmk_by_ssid_psk_from_pmkinfo_ap(uint8_t *pmk,uint8_t *ssid,uint8_t ssid_len, uint8_t *psk,uint8_t psk_len)
{
#ifdef MTK_NVDM_ENABLE
//if ((0 != ssid->auth_mode) && ((0 != ssid->encr_type) || (1 != ssid->encr_type))) {
    uint8_t pmk_info[WIFI_MAX_LENGTH_OF_SSID+WIFI_LENGTH_PASSPHRASE+32] = {0};
    uint8_t pmk_info_zero[WIFI_MAX_LENGTH_OF_SSID+WIFI_LENGTH_PASSPHRASE+32] = {0};
    uint32_t pmk_info_len = sizeof(pmk_info);
    nvdm_read_data_item(WIFI_PROFILE_BUFFER_AP, "PMK_INFO",
                       (uint8_t *)pmk_info, &pmk_info_len);
    //hex_dump("ap pmk_info", pmk_info, 128);
    if ((0 == os_memcmp(pmk_info, ssid, ssid_len)) &&
        (0 == os_memcmp(pmk_info+WIFI_MAX_LENGTH_OF_SSID, psk, psk_len)) &&
        (0 == os_memcmp(pmk_info+ssid_len, pmk_info_zero, WIFI_MAX_LENGTH_OF_SSID - ssid_len)) &&
        (0 == os_memcmp(pmk_info+WIFI_MAX_LENGTH_OF_SSID+psk_len, pmk_info_zero, WIFI_LENGTH_PASSPHRASE - psk_len))) {
            os_memcpy(pmk, pmk_info+WIFI_MAX_LENGTH_OF_SSID+WIFI_LENGTH_PASSPHRASE, 32);
			LOG_I(wifi, "pmkinfo_ap same \r\n");
            return 0;
    } else
#endif
    {
        return -1;
    }
}
//return >= 0 means got value succeed
//PreventAutoOpenConnect = 1 means feature on, 
//will stop connect to openAP when user set wpapsk or wep_key.
int32_t wifi_get_prevent_to_open_flag(uint8_t* prevent_flag)
{
#ifdef MTK_NVDM_ENABLE
    uint8_t buff[2] = {0};
    uint32_t len = sizeof(buff);
    nvdm_status_t ret = NVDM_STATUS_OK;
    ret = nvdm_read_data_item(WIFI_PROFILE_BUFFER_STA, "PreventAutoOpenConnect", buff, &len);
    if(ret == NVDM_STATUS_OK) {
        *prevent_flag = (uint8_t)atoi((char *)buff);
    }
    LOG_I(wifi, "prevent_flag=%d, ret=%d", *prevent_flag, ret);
    return ret;
#else
    LOG_E(wifi,"not support");
    //need init from wifi_defualt_config.h or other flash region
    return -1;
#endif

}

#ifdef MTK_CM4_WIFI_TASK_ENABLE
void RegisterSecurityInterface(SECURITY_INTERFACE_T * ptInterface)
{
    os_memset(ptInterface,0,sizeof(SECURITY_INTERFACE_T));
    #ifdef WIFI_STACK_HW_CRYPTO
        ptInterface->pfHmacSha1     = WIFI_HAL_HMAC_SHA1;
        ptInterface->pfHmacMd5      = WIFI_HAL_HMAC_MD5;
        ptInterface->pfAesKeyWrap   = WIFI_HAL_AES_Key_Wrap;
        ptInterface->pfAesKeyUnwrap = WIFI_HAL_AES_Key_Unwrap;
        ptInterface->pfAesCmac      = WIFI_HAL_AES_CMAC;
    #else
        ptInterface->pfHmacSha1     = RT_HMAC_SHA1;
        ptInterface->pfHmacMd5      = RT_HMAC_MD5;
        ptInterface->pfAesKeyWrap   = AES_Key_Wrap;
        ptInterface->pfAesKeyUnwrap = AES_Key_Unwrap;
    #endif
}
#endif
/***************** Just for internal use **********************/

