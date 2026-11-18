#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

/* True if the Korvo BSP speaker path (ES8389 slave, no MCLK, 16-bit) can clock this sample rate. */
bool korvo_audio_rate_supported(uint32_t hz);
