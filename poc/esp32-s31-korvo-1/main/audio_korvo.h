#pragma once

#include "esp_err.h"

/* Runs one bounded 440 Hz PCM tone test through the Korvo ES8389 output. */
esp_err_t audio_korvo_play_test_tone(void);
