IC_CONFIG                             = aw7698
BOARD_CONFIG                          = aw7698_evk
#This option indicate AW7698 package with PSRAM or not
MTK_NO_PSRAM_ENABLE                 = n
# debug level: none, error, warning, and info
MTK_DEBUG_LEVEL                     = info
# System service debug feature for internal use
MTK_SUPPORT_HEAP_DEBUG              = n
MTK_HEAP_SIZE_GUARD_ENABLE          = n
MTK_OS_CPU_UTILIZATION_ENABLE       = y
#SWLA
MTK_SWLA_ENABLE                     = n

#NVDM
MTK_NVDM_ENABLE                     = y

#WIFI features
#If this feature is off, than other Wi-Fi and LWIP feature options won't take effect
MTK_WIFI_ENABLE                     = y
#hoang edit
#MTK_WIFI_TGN_VERIFY_ENABLE          = y 
MTK_WIFI_TGN_VERIFY_ENABLE          = n

MTK_WIFI_DIRECT_ENABLE              = n
MTK_WIFI_PROFILE_ENABLE             = y
MTK_SMTCN_V5_ENABLE                 = y
MTK_CM4_WIFI_TASK_ENABLE            = y
MTK_WIFI_ROM_ENABLE                 = y
MTK_WIFI_CLI_ENABLE                 = y
#Wi-Fi&BT coex
MTK_BWCS_ENABLE                     = n

# system hang debug: none, o1 and o2
MTK_SYSTEM_HANG_TRACER_ENABLE       = n
#LWIP features
MTK_IPERF_ENABLE                    = y
MTK_PING_OUT_ENABLE                 = y
MTK_USER_FAST_TX_ENABLE             = n

#CLI features
MTK_MINICLI_ENABLE                  = y
MTK_CLI_TEST_MODE_ENABLE            = y

MTK_HAL_LOWPOWER_ENABLE             = y
MTK_HIF_GDMA_ENABLE                 = y
MTK_OS_CLI_ENABLE                   = y

#secure boot
ifneq ($(wildcard $(strip $(SOURCE_DIR))/tools/secure_boot/),)
MTK_SECURE_BOOT_ENABLE      = y
else
MTK_SECURE_BOOT_ENABLE      = n
endif


#single rate test tool
MTK_WIFI_SINGLE_RATE_TEST_ENABLE    = n
