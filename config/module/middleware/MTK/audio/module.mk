
###################################################

AUDIO_SRC = middleware/MTK/audio

ifeq ($(MTK_TEMP_REMOVE), y)
else
  ifeq ($(MTK_NVDM_ENABLE), y)
    AUDIO_FILES = $(AUDIO_SRC)/src/audio_middleware_api.c
    AUDIO_FILES += $(AUDIO_SRC)/src/audio_dsp_fd216_db_to_gain_value_mapping_table.c

    ifeq ($(IC_CONFIG),ab155x)
      AUDIO_FILES += $(AUDIO_SRC)/port/ab155x/src/audio_nvdm.c
    else ifeq ($(IC_CONFIG),am255x)
      AUDIO_FILES += $(AUDIO_SRC)/port/am255x/src/audio_nvdm.c
    else ifeq ($(IC_CONFIG),mt2533)
      AUDIO_FILES += $(AUDIO_SRC)/port/mt2533/src/audio_nvdm.c
    else
      AUDIO_FILES += $(AUDIO_SRC)/port/mt2523/src/audio_nvdm.c  
    endif

  endif
endif

# temp for build pass
ifeq ($(MTK_WAV_DECODER_ENABLE), y)
    AUDIO_FILES += $(AUDIO_SRC)/src/audio_codec.c
endif

ifeq ($(MTK_AUDIO_MP3_ENABLED), y)
  ifeq ($(IC_CONFIG),mt7687)
    AUDIO_FILES += $(AUDIO_SRC)/mp3_codec/src/mp3_codec_7687.c
    LIBS += $(SOURCE_DIR)/prebuilt/middleware/MTK/audio/mp3_codec/lib/arm_cm4/libmp3dec.a
  else
    ifeq ($(IC_CONFIG),mt7682)
      AUDIO_FILES += $(AUDIO_SRC)/mp3_codec/src/mp3_codec_7682.c
      LIBS += $(SOURCE_DIR)/prebuilt/middleware/MTK/audio/mp3_codec/lib/arm_cm4/libmp3dec.a
    else ifeq ($(IC_CONFIG),mt7686)
      AUDIO_FILES += $(AUDIO_SRC)/mp3_codec/src/mp3_codec_7682.c
      LIBS += $(SOURCE_DIR)/prebuilt/middleware/MTK/audio/mp3_codec/lib/arm_cm4/libmp3dec.a
	else ifeq ($(IC_CONFIG),aw7698)
      AUDIO_FILES += $(AUDIO_SRC)/mp3_codec/src/mp3_codec_7698.c
      LIBS += $(SOURCE_DIR)/prebuilt/middleware/MTK/audio/mp3_codec/lib/arm_cm4/libmp3dec.a
    else
      AUDIO_FILES += $(AUDIO_SRC)/mp3_codec/src/mp3_codec.c
      LIBS += $(SOURCE_DIR)/prebuilt/middleware/MTK/audio/mp3_codec/lib/arm_cm4/libmp3dec.a
    endif
  endif
  
  ifeq ($(MTK_AVM_DIRECT), y)
     AUDIO_FILES += $(AUDIO_SRC)/mp3_codec/src/mp3_codec_155x.c
  endif
  
  # temp for build pass
  AUDIO_FILES += $(AUDIO_SRC)/src/audio_codec.c
else ifeq ($(MTK_MP3_DECODER_ENABLED), y)
  ifeq ($(IC_CONFIG),ab155x)
    AUDIO_FILES += $(AUDIO_SRC)/mp3_codec/src/mp3_codec.c
    AUDIO_FILES += $(AUDIO_SRC)/mp3_codec/src/mp3_codec_155x.c
    LIBS += $(SOURCE_DIR)/prebuilt/middleware/MTK/audio/mp3_codec/lib/arm_cm4/libmp3dec.a
  else ifeq ($(IC_CONFIG),am255x)
    AUDIO_FILES += $(AUDIO_SRC)/mp3_codec/src/mp3_codec.c
    AUDIO_FILES += $(AUDIO_SRC)/mp3_codec/src/mp3_codec_255x.c
    LIBS += $(SOURCE_DIR)/prebuilt/middleware/MTK/audio/mp3_codec/lib/arm_cm4/libmp3dec.a
  endif

  # temp for build pass
  AUDIO_FILES += $(AUDIO_SRC)/src/audio_codec.c
endif

ifeq ($(MTK_AUDIO_AMR_ENABLED), y)
ifeq ($(IC_CONFIG),mt7687)
AUDIO_FILES += $(AUDIO_SRC)/amr_codec/src/amr_encoder.c
AUDIO_FILES += $(AUDIO_SRC)/amr_codec/src/amr_decoder.c
LIBS += $(SOURCE_DIR)/prebuilt/middleware/MTK/audio/amr_codec/lib/arm_cm4/libamr.a
endif
ifeq ($(IC_CONFIG),mt7682)
AUDIO_FILES += $(AUDIO_SRC)/amr_codec/src/amr_encoder_7682.c
AUDIO_FILES += $(AUDIO_SRC)/amr_codec/src/amr_decoder_7682.c
LIBS += $(SOURCE_DIR)/prebuilt/middleware/MTK/audio/amr_codec/lib/arm_cm4/libamr.a
endif
ifeq ($(IC_CONFIG),mt7686)
AUDIO_FILES += $(AUDIO_SRC)/amr_codec/src/amr_encoder_7682.c
AUDIO_FILES += $(AUDIO_SRC)/amr_codec/src/amr_decoder_7682.c
LIBS += $(SOURCE_DIR)/prebuilt/middleware/MTK/audio/amr_codec/lib/arm_cm4/libamr.a
endif
ifeq ($(IC_CONFIG),aw7698)
AUDIO_FILES += $(AUDIO_SRC)/amr_codec/src/amr_encoder_7698.c
AUDIO_FILES += $(AUDIO_SRC)/amr_codec/src/amr_decoder_7698.c
LIBS += $(SOURCE_DIR)/prebuilt/middleware/MTK/audio/amr_codec/lib/arm_cm4/libamr.a
endif
endif


ifeq ($(MTK_AUDIO_AAC_DECODER_ENABLED),y)
    ifeq ($(IC_CONFIG),mt7682)
      AUDIO_FILES  += $(AUDIO_SRC)/aac_codec/src/aac_decoder_api_7686.c
    else ifeq ($(IC_CONFIG),mt7686)
      AUDIO_FILES  += $(AUDIO_SRC)/aac_codec/src/aac_decoder_api_7686.c
	else ifeq ($(IC_CONFIG),aw7698)
      AUDIO_FILES  += $(AUDIO_SRC)/aac_codec/src/aac_decoder_api_7698.c
    else
      AUDIO_FILES  += $(AUDIO_SRC)/aac_codec/src/aac_decoder_api.c
    endif
    LIBS += $(SOURCE_DIR)/prebuilt/middleware/MTK/audio/aac_codec/lib/libheaac_decoder.a
endif

ifeq ($(MTK_PROMPT_SOUND_ENABLE), y)
  ifeq ($(MTK_AVM_DIRECT), y)
  else
    LIBS += $(SOURCE_DIR)/prebuilt/driver/board/component/audio/lib/arm_cm4/libblisrc.a

    AUDIO_FILES += $(AUDIO_SRC)/audio_mixer/src/audio_mixer.c
  endif
  
  AUDIO_FILES += $(AUDIO_SRC)/prompt_control/src/prompt_control.c
endif

ifeq ($(MTK_SBC_ENCODER_ENABLE), y)
    LIBS += $(SOURCE_DIR)/middleware/MTK/audio/sbc_codec/lib/arm_cm4/libsbc_encoder.a
endif
ifeq ($(MTK_WAV_DECODER_ENABLE), y)
    AUDIO_FILES += $(AUDIO_SRC)/wav_codec/src/wav_codec.c
    LIBS += $(SOURCE_DIR)/middleware/MTK/audio/wav_codec/lib/arm_cm4/libwav_codec.a
endif

ifeq ($(MTK_BT_A2DP_SOURCE_ENABLE), y)
    AUDIO_FILES += $(AUDIO_SRC)/sbc_codec/src/sbc_codec.c
    LIBS += $(SOURCE_DIR)/middleware/MTK/audio/sbc_codec/lib/arm_cm4/libsbc_encoder.a
    LIBS += $(SOURCE_DIR)/prebuilt/driver/board/component/audio/lib/arm_cm4/libblisrc.a
endif

#bt codec driver source
ifeq ($(MTK_AVM_DIRECT), y)
    ifeq ($(MTK_BT_CODEC_ENABLED),y)
        AUDIO_FILES  += $(AUDIO_SRC)/bt_codec/src/bt_a2dp_codec.c
        AUDIO_FILES  += $(AUDIO_SRC)/bt_codec/src/bt_hfp_codec.c
    ifeq ($(MTK_BT_CODEC_BLE_ENABLED),y)
        CFLAGS += -DMTK_BT_CODEC_BLE_ENABLED
        AUDIO_FILES  += $(AUDIO_SRC)/bt_codec/src/bt_ble_codec.c
    endif
    endif
endif

ifeq ($(MTK_ANC_ENABLE), y)
    AUDIO_FILES += $(AUDIO_SRC)/anc_control/src/anc_control.c
endif

# Audio sink line-in related
ifeq ($(MTK_LINEIN_PLAYBACK_ENABLE), y)
    AUDIO_FILES += $(AUDIO_SRC)/linein_playback/src/linein_playback.c                       \
            $(AUDIO_SRC)/linein_playback/src/linein_control/audio_sink_srv_line_in_state_machine.c \
            $(AUDIO_SRC)/linein_playback/src/linein_control/audio_sink_srv_line_in_callback.c      \
            $(AUDIO_SRC)/linein_playback/src/linein_control/audio_sink_srv_line_in_playback.c      \
            $(AUDIO_SRC)/linein_playback/src/linein_control/audio_sink_srv_line_in_control.c
endif

# pure linein playback check
ifeq ($(MTK_PURE_LINEIN_PLAYBACK_ENABLE),y)
    ifeq ($(MTK_LINEIN_PLAYBACK_ENABLE),n)
    $(error IF you select MTK_PURE_LINEIN_PLAYBACK_ENABLE, you also need select MTK_LINEIN_PLAYBACK_ENABLE)
    endif
endif

ifeq ($(MTK_RECORD_ENABLE), y)
    AUDIO_FILES += $(AUDIO_SRC)/record_control/src/record_control.c
endif

ifeq ($(MTK_AUDIO_DUMP_ENABLE), y)
    AUDIO_FILES += $(AUDIO_SRC)/record_control/audio_dump/src/audio_dump.c
endif

AUDIO_FILES += $(AUDIO_SRC)/src/audio_log.c

C_FILES += $(AUDIO_FILES)

###################################################
# include path
CFLAGS += -I$(SOURCE_DIR)/middleware/MTK/audio/inc

ifeq ($(IC_CONFIG),ab155x)
  CFLAGS += -I$(SOURCE_DIR)/middleware/MTK/audio/port/ab155x/inc
else ifeq ($(IC_CONFIG),am255x)
  CFLAGS += -I$(SOURCE_DIR)/middleware/MTK/audio/port/am255x/inc
else ifeq ($(IC_CONFIG),mt2533)
  CFLAGS += -I$(SOURCE_DIR)/middleware/MTK/audio/port/mt2533/inc
  CFLAGS += -I$(SOURCE_DIR)/middleware/MTK/audio/port/mt2533/inc/mt2533_external_dsp_profile
else
  CFLAGS += -I$(SOURCE_DIR)/middleware/MTK/audio/port/mt2523/inc
endif



ifeq ($(MTK_AUDIO_MP3_ENABLED), y)
CFLAGS += -I$(SOURCE_DIR)/middleware/MTK/audio/mp3_codec/inc
else ifeq ($(MTK_MP3_DECODER_ENABLED), y)
CFLAGS += -I$(SOURCE_DIR)/middleware/MTK/audio/mp3_codec/inc
endif

ifeq ($(MTK_AUDIO_AMR_ENABLED), y)
CFLAGS += -I$(SOURCE_DIR)/middleware/MTK/audio/amr_codec/inc
endif

ifeq ($(MTK_AUDIO_AAC_DECODER_ENABLED), y)
CFLAGS += -I$(SOURCE_DIR)/middleware/MTK/audio/aac_codec/inc
endif

ifeq ($(MTK_PROMPT_SOUND_ENABLE), y)
  CFLAGS += -I$(SOURCE_DIR)/middleware/MTK/audio/audio_mixer/inc
  CFLAGS += -I$(SOURCE_DIR)/middleware/MTK/audio/prompt_control/inc
  CFLAGS += -I$(SOURCE_DIR)/prebuilt/driver/board/component/audio/inc
endif

ifeq ($(MTK_SBC_ENCODER_ENABLE), y)
  CFLAGS += -I$(SOURCE_DIR)/middleware/MTK/audio/sbc_codec/inc
endif
ifeq ($(MTK_WAV_DECODER_ENABLE), y)
  CFLAGS += -I$(SOURCE_DIR)/middleware/MTK/audio/wav_codec/inc
endif

ifeq ($(MTK_BT_A2DP_SOURCE_ENABLE), y)
  CFLAGS += -I$(SOURCE_DIR)/middleware/MTK/audio/sbc_codec/inc
  CFLAGS += -I$(SOURCE_DIR)/prebuilt/driver/board/component/audio/inc
endif

#bt codec driver source
ifeq ($(MTK_AVM_DIRECT), y)
    ifeq ($(MTK_BT_CODEC_ENABLED),y)
        CFLAGS  += -I$(SOURCE_DIR)/middleware/MTK/audio/bt_codec/inc
        CFLAGS  += -I$(SOURCE_DIR)/middleware/MTK/bluetooth/inc
        CFLAGS  += -I$(SOURCE_DIR)/prebuilt/middleware/MTK/bluetooth/inc
        ifeq ($(MTK_BT_A2DP_SOURCE_ENABLE), y)
            CFLAGS  += -I$(SOURCE_DIR)/middleware/MTK/audio/sbc_codec/inc
        endif
    endif

    ifeq ($(MTK_BT_A2DP_SOURCE_ENABLE), y)
        CFLAGS += -DMTK_BT_A2DP_SOURCE_SUPPORT
    endif
endif

ifeq ($(MTK_ANC_ENABLE), y)
  CFLAGS  += -I$(SOURCE_DIR)/middleware/MTK/audio/anc_control/inc
endif

# Include audio sink path
ifeq ($(MTK_LINEIN_PLAYBACK_ENABLE), y)
    CFLAGS  += -I$(SOURCE_DIR)/middleware/MTK/audio/linein_playback/inc
    CFLAGS  += -I$(SOURCE_DIR)/middleware/MTK/audio/linein_playback/inc/linein_control
    CFLAGS  += -DMTK_LINE_IN_ENABLE
endif

ifeq ($(MTK_RECORD_ENABLE), y)
  CFLAGS  += -I$(SOURCE_DIR)/middleware/MTK/audio/record_control/inc
endif

ifeq ($(MTK_AUDIO_DUMP_ENABLE), y)
  CFLAGS  += -DMTK_AUDIO_DUMP_ENABLE
  CFLAGS  += -I$(SOURCE_DIR)/middleware/MTK/audio/record_control/audio_dump/inc
endif
