#ifndef __WIFI_CONFIG_H__
#define __WIFI_CONFIG_H__


#ifdef __cplusplus
extern "C" {
#endif

extern void wifi_config(const char *ssi, const char *password);
extern void wifi_ap_config(void);
#ifdef __cplusplus
}
#endif

#endif 