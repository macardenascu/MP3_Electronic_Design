idf_component_register(
    SRCS "app_main.c" "audio_korvo.c" "audio_bsp_tone.c" "music_catalog.c" "storage_korvo.c"
         "core/tone_gen.c"
    INCLUDE_DIRS "." "core"
    REQUIRES esp32_s31_korvo_1 esp_codec_dev esp_driver_i2s
)
