wifi_host module usage guide

Brief:          This module is the BWCS implementation. It supports the Wi-Fi and BLE coexistence.

Usage:          GCC:  Include the following items with the module:
					  1) Add the following module.mk for libs and source file:
					     include $(SOURCE_DIR)/middleware/MTK/bwcs/module.mk
					  2) Module.mk provides different options to enable or disable related profiles. Please configure these options in the specified GCC/feature.mk:
					     MTK_BWCS_ENABLE
					  3) Add the header file path:
					     CFLAGS  += -I$(SOURCE_DIR)/middleware/MTK/bwcs/inc
					     CFLAGS  += -I$(SOURCE_DIR)/middleware/MTK/wifi_service/combo/inc

Dependency:     Please define MTK_WIFI_ENABLE in the feature.mk under the project gcc folder.

Notice:         1) BWCS works in situations with BT and Wi-Fi coexistence.

Relative doc:   Please refer to the Airoha_IoT_SDK_for_WiFi_Developers_Guide under the doc folder for more detail.

Example project: Please find the iot_sdk_demo project under the aw7698_evk project folder.

