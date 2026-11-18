idf_component_register(
    SRCS "app_main.c" "audio_korvo.c" "audio_bsp_tone.c" "audio_bsp_wav.c" "audio_bsp_mp3.c" "korvo_audio_caps.c" "music_catalog.c"
         "storage_korvo.c" "core/tone_gen.c" "core/wav_parser.c" "decoder/mp3_decoder.c"
         "audio_engine.c" "player_app.c" "korvo_volume.c"
         "core/player_core.c" "core/volume.c" "core/power_policy.c" "core/battery_sim.c"
         "korvo_display.c" "korvo_diag.c" "core/ui_render.c" "core/ui_model.c"
    INCLUDE_DIRS "." "core" "decoder" "third_party/minimp3"
    REQUIRES esp32_s31_korvo_1 esp_codec_dev esp_driver_i2s
    # esp_timer: esp_timer_get_time() and the battery timer.
    # esp_lcd: korvo_display.c calls esp_lcd_panel_draw_bitmap()/disp_on_off() directly; the BSP only
    # lists esp_lcd in its own PRIV_REQUIRES, so it does not reach main.
    PRIV_REQUIRES esp_timer esp_lcd
)
