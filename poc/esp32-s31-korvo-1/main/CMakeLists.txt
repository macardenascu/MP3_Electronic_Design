idf_component_register(
    SRCS "app_main.c" "audio_korvo.c" "audio_bsp_tone.c" "audio_bsp_wav.c" "audio_bsp_mp3.c" "korvo_audio_caps.c" "music_catalog.c"
         "storage_korvo.c" "core/tone_gen.c" "core/wav_parser.c" "decoder/mp3_decoder.c"
         "audio_engine.c" "player_app.c" "korvo_volume.c"
         "core/player_core.c" "core/volume.c" "core/power_policy.c" "core/battery_sim.c"
    INCLUDE_DIRS "." "core" "decoder" "third_party/minimp3"
    REQUIRES esp32_s31_korvo_1 esp_codec_dev esp_driver_i2s
    PRIV_REQUIRES esp_timer
)
