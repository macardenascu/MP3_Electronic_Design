#pragma once

#include "esp_err.h"

/* Route A: plays a 3-step 440 Hz tone ladder through the official Korvo BSP speaker codec. */
esp_err_t audio_bsp_tone_run(void);
