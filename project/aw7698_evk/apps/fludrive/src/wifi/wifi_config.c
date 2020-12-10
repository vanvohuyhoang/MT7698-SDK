#include "wifi_api.h"
#include "wifi_lwip_helper.h"
#include "wifi_config.h"
#include "connsys_profile.h"
#include "wifi_nvdm_config.h"

void wifi_config(const char* ssid, const char *password){
    wifi_config_t config = {0};
    config.opmode = WIFI_MODE_STA_ONLY;
    strcpy((char *)config.sta_config.ssid, ssid);
    strcpy((char *)config.sta_config.password, password);
    config.sta_config.ssid_length = strlen((const char *)config.sta_config.ssid);
    config.sta_config.password_length = strlen((const char *)config.sta_config.password);
    /* Initialize wifi stack and register wifi init complete event handler,
    * notes:  the wifi initial process will be implemented and finished while system task scheduler is running.
    */
    wifi_init(&config, NULL);

    /* Tcpip stack and net interface initialization,  dhcp client, dhcp server process initialization. */
    lwip_network_init(config.opmode);
    lwip_net_start(config.opmode);
}
static int32_t wifi_init_done_handler(wifi_event_t event,
                                      uint8_t *payload,
                                      uint32_t length)
{
    LOG_I(common, "WiFi Init Done: port = %d", payload[6]);
    return 1;
}
void wifi_ap_config(void)
{
    wifi_cfg_t wifi_config = {0};
    if (0 != wifi_config_init(&wifi_config)) {
    LOG_E(common, "wifi config init fail");
    return -1;
    }

    wifi_config_t config = {0};
    wifi_config_ext_t config_ext = {0};

    config.opmode = wifi_config.opmode;

    memcpy(config.ap_config.ssid, wifi_config.ap_ssid, 32);
    config.ap_config.ssid_length = wifi_config.ap_ssid_len;
    memcpy(config.ap_config.password, wifi_config.ap_wpa_psk, 64);
    config.ap_config.password_length = wifi_config.ap_wpa_psk_len;
    config.ap_config.auth_mode = (wifi_auth_mode_t)wifi_config.ap_auth_mode;
    config.ap_config.encrypt_type = (wifi_encrypt_type_t)wifi_config.ap_encryp_type;
    config.ap_config.channel = wifi_config.ap_channel;
    config.ap_config.bandwidth = wifi_config.ap_bw;
    config.ap_config.bandwidth_ext = WIFI_BANDWIDTH_EXT_40MHZ_UP;
    config_ext.ap_wep_key_index_present = 1;
    config_ext.ap_wep_key_index = wifi_config.ap_default_key_id;
    config_ext.ap_hidden_ssid_enable_present = 1;
    config_ext.ap_hidden_ssid_enable = wifi_config.ap_hide_ssid;
    config_ext.sta_power_save_mode = (wifi_power_saving_mode_t)wifi_config.sta_power_save_mode;


	config.opmode = WIFI_MODE_AP_ONLY;
	strcpy((char *)config.ap_config.ssid, "Fludrive_AP");
	config.ap_config.ssid_length = strlen("Fludrive_AP");
	config.ap_config.auth_mode = WIFI_AUTH_MODE_WPA2_PSK;
	config.ap_config.encrypt_type = WIFI_ENCRYPT_TYPE_AES_ENABLED;
	strcpy((char *)config.ap_config.password, "77777777");
	config.ap_config.password_length = strlen("77777777");
	config.ap_config.channel = 6;

    wifi_init(&config, NULL);
    //wifi_init(&config, &config_ext);

    //wifi_connection_register_event_handler(WIFI_EVENT_IOT_INIT_COMPLETE, wifi_init_done_handler);

    /* Tcpip stack and net interface initialization,  dhcp client, dhcp server process initialization*/
    lwip_network_init(config.opmode);
    lwip_net_start(config.opmode);

}