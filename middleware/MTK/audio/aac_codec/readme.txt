AAC codec module usage guide

Brief:          This module is a aac codec for decoding the aac format data.

Usage:          GCC:  For the aac codec, make sure to include the following:
                      1) Add the following module.mk for libs and source file:
                         include $(SOURCE_DIR)/middleware/MTK/audio/module.mk
                      2) Module.mk provides different options to enable or disable according to the profiles.
                         Please configure the related options on the specified GCC/feature.mk.
                         MTK_AUDIO_AAC_DECODER_ENABLED
                      3) Add the header file path:
                         CFLAGS += -I$(SOURCE_DIR)/middleware/MTK/audio/aac_codec/inc

Dependency:     None.

Notice:         None.

Relative doc:   None.

Example project:None.
