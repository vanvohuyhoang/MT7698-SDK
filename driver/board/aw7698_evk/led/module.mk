LED_BSP_SRC = driver/board/aw7698_evk/led

C_FILES  += $(LED_BSP_SRC)/bsp_led_internal.c
C_FILES  += $(LED_BSP_SRC)/bsp_led.c

#################################################################################
#include path
CFLAGS  += -I$(SOURCE_DIR)/driver/board/aw7698_evk/led

#################################################################################
#Enable the feature by configuring
COM_CFLAGS         += -DMTK_LED_ENABLE

