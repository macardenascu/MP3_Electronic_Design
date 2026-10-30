idf_component_register(
    SRCS "app_main.c" "audio_korvo.c" "music_catalog.c" "storage_korvo.c"
    INCLUDE_DIRS "."
    REQUIRES esp32_s31_korvo_1 esp_codec_dev esp_driver_i2s
)
