AMR codec module usage guide

Brief:          This module is a amr codec for encoding the pcm format data and decoding the amr format data.

Usage:          GCC:  For the amr codec, make sure to include the following:
                      1) Add the following module.mk for libs and source file:
                         include $(SOURCE_DIR)/middleware/MTK/audio/module.mk
                      2) Module.mk provides different options to enable or disable according to the profiles.
                         Please configure the related options on the specified GCC/feature.mk.
                         MTK_AUDIO_AMR_ENABLED
                      3) Add the header file path:
                         CFLAGS += -I$(SOURCE_DIR)/middleware/MTK/audio/amr_codec/inc

Dependency:     None.

Notice:         None.

Relative doc:   None.

Example project:None.
