idf_component_register(
    SRCS "app_main.c" "audio_korvo.c" "audio_bsp_tone.c" "audio_bsp_wav.c" "audio_bsp_mp3.c" "music_catalog.c"
         "storage_korvo.c" "core/tone_gen.c" "core/wav_parser.c" "decoder/mp3_decoder.c"
    INCLUDE_DIRS "." "core" "decoder" "third_party/minimp3"
    REQUIRES esp32_s31_korvo_1 esp_codec_dev esp_driver_i2s
    PRIV_REQUIRES esp_timer
)
