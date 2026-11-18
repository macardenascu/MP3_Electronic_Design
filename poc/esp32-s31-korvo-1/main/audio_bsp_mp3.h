#pragma once

#include "esp_err.h"

/* E4: starts the "mp3_play" task, which decodes an MP3 file from the SD card with minimp3 and
 * plays it through the official Korvo BSP speaker codec. Returns once the task is created. */
esp_err_t audio_bsp_mp3_start(const char *path);
