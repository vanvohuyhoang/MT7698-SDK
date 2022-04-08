sct_key module usage guide

Brief:          sct_key is a common upper layer for different types of keys, include eint_key and keypad.
                This module have a common interface and common event type.

Usage:          For specific information about the sct_key event type and API interfaces, please refer to middleware\MTK\key_event\common\inc\sct_key_event.h


Dependency:     sct_key_EVENT_ENABLE: This option is for enabling sct_key. Defined in mcu\middleware\MTK\key_event\module.mk, the user can include this module.mk on Makefile to enable this feature.
                Sub options:
                    MTK_EINT_KEY_ENABLE: This option configures whether sct_key can support eint_key. Defined in mcu\driver\board\BOARD_CONFIG\eint_key\module.mk, the user can include this module.mk on Makefile to enable this feature.
                    HAL_KEYPAD_ENABLE: This option configures whether sct_key can support keypad. Defined in the hal_feature_config.h under the project inc folder to enable this feature.
Notice:         None

Relative doc:   None

Example project:None
