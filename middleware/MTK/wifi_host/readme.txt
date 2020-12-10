wifi_host module usage guide

Brief:          This module is the wifi_host implementation. It supports the Wi-Fi host on MT2523 and MT2625.

Usage:          GCC:  Include the module with
					  1) Add the following module.mk for libs and source file:
					     include $(SOURCE_DIR)/middleware/MTK/wifi_host/wfcm/module.mk
					     include $(SOURCE_DIR)/middleware/MTK/wifi_host/xboot/module.mk
					     include $(SOURCE_DIR)/middleware/MTK/wifi_host/platform/freertos/hif/sdio/module.mk
					     include $(SOURCE_DIR)/middleware/MTK/wifi_host/common/module.mk
					     include $(SOURCE_DIR)/middleware/MTK/wifi_host/platform/freertos/kal/module.mk
					  2) Module.mk provides different options to enable or disable according profiles. Please configure these options in the specified GCC/feature.mk:
					     MTK_WIFI_CHIP_USE_MT5932
					     MTK_WIFI_STUB_CONF_ENABLE
					     MTK_WIFI_STUB_CONF_SPI_ENABLE
					     MTK_WIFI_STUB_CONF_SPIM_ENABLE
					     MTK_WIFI_STUB_CONF_SDIO_MSDC_ENABLE
					  3) Add the header file path:
					     CFLAGS  += -I$(SOURCE_DIR)/middleware/MTK/wifi_host/wfcm/inc
					     CFLAGS  += -I$(SOURCE_DIR)/middleware/MTK/wifi_host/common/inc
					     CFLAGS  += -I$(SOURCE_DIR)/middleware/MTK/wifi_host/platform/freertos/hif/sdio/inc
					  4) Copy lwip_network.h/ept_gpio_var.h/ept_eint_var.h to the [project]/inc folder and lwip_network.c/ept_gpio_var.c/ept_eint_var.c to the [project]/src folder from the project
					     under project folder wifi5932_ref_design. Add the following source files to your GCC project makefile.
					     APP_FILES += $(APP_PATH_SRC)/main.c
					     APP_FILES += $(APP_PATH_SRC)/regions_init.c
					     APP_FILES += $(APP_PATH_SRC)/lwip_network.c
					     APP_FILES += $(APP_PATH)/GCC/syscalls.c
					     APP_FILES += $(APP_PATH_SRC)/ept_gpio_var.c
					     APP_FILES += $(APP_PATH_SRC)/ept_eint_var.c
					     APP_FILES += $(APP_PATH_SRC)/sys_init.c
					     APP_FILES += $(APP_PATH_SRC)/system_mt2523.c

Dependency:     Please define HAL_SDIO_MODULE_ENABLED and HAL_EINT_MODULE_ENABLED and HAL_GDMA_MODULE_ENABLED in the hal_feature_config.h under the project inc folder.

Notice:         1) Please disable macro configUSE_TICKLESS_IDLE in project if you want to use the AT command.

Relative doc:   Please refer to the Airoha_IoT_SDK_for_WiFi_Migration_Developers_Guide under the doc folder for more detail.

Example project: Please find the wifi5932_ref_design project under the project folder.

