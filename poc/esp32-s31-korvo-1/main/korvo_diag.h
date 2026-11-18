#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

/* E6 (Korvo only, PoC): color diagnostics panel drawn outside the virtual OLED, 8x8 font scaled x2.
 * Stays visible while the virtual OLED is "off". Call from the same task as korvo_display. */

typedef struct {
    uint32_t sample_rate;
    uint32_t channels;
    uint32_t bitrate_kbps;
    uint32_t decode_avg_us;
    uint32_t decode_max_us;
    bool battery_valid; /* false before the simulated battery starts: shows "--" */
    uint32_t battery_mv;
} korvo_diag_info_t;

/* Draws the panel (title, audio, decode, battery, free heap, uptime). Heap and uptime are read here. */
esp_err_t korvo_diag_update(const korvo_diag_info_t *info);
