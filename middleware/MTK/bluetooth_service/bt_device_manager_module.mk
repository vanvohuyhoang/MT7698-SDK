###################################################
# Sources
###################################################

BT_DEVICE_MGR_SOURCE = middleware/MTK/bluetooth_service/src
BT_DEVICE_MGR_FILES =  $(BT_DEVICE_MGR_SOURCE)/bt_device_manager.c \
					   $(BT_DEVICE_MGR_SOURCE)/bt_device_manager_db.c \
					   $(BT_DEVICE_MGR_SOURCE)/bt_device_manager_remote_info.c

ifeq ($(MTK_AWS_MCE_ENABLE), y)            
BT_DEVICE_MGR_FILES  += 	$(BT_DEVICE_MGR_SOURCE)/bt_device_manager_aws_local_info.c
endif

C_FILES += $(BT_DEVICE_MGR_FILES)
###################################################
# include path
###################################################

CFLAGS  += -I$(SOURCE_DIR)/middleware/MTK/bluetooth_service/inc
