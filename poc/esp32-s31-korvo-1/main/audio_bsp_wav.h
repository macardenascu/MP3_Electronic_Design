#pragma once

#include "esp_err.h"

/* E3: plays a 16-bit PCM WAV file from the SD card through the official Korvo BSP speaker codec. */
esp_err_t audio_bsp_wav_run(const char *path);
