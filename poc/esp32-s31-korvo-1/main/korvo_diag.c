#include "korvo_diag.h"

#include <inttypes.h>
#include <stdio.h>

/* ESP-IDF 6.1: esp_get_free_heap_size() (esp_system.h), heap_caps_get_free_size() (esp_heap_caps.h),
 * esp_timer_get_time() in microseconds since boot. */
#include "esp_heap_caps.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "korvo_display.h"

#define DIAG_COLS 46 /* 46 * 16 px = 736 px wide */
#define DIAG_TITLE_COLOR 0xFFE0u /* yellow */
#define DIAG_TEXT_COLOR 0x07E0u  /* green */

esp_err_t korvo_diag_update(const korvo_diag_info_t *info)
{
    char line[80]; /* longer than DIAG_COLS: text2x clips to DIAG_COLS columns */
    int y = KORVO_DIAG_Y;
    esp_err_t err = korvo_display_text2x(KORVO_DIAG_X, y, "Korvo diagnostics (PoC only)", DIAG_COLS,
                                         DIAG_TITLE_COLOR, KORVO_SCREEN_BG_COLOR);

    if (info->sample_rate) {
        snprintf(line, sizeof(line), "Audio %" PRIu32 " Hz %" PRIu32 " ch %" PRIu32 " kbps", info->sample_rate,
                 info->channels, info->bitrate_kbps);
    } else {
        snprintf(line, sizeof(line), "Audio --");
    }
    y += KORVO_DIAG_LINE_H;
    if (err == ESP_OK) err = korvo_display_text2x(KORVO_DIAG_X, y, line, DIAG_COLS, DIAG_TEXT_COLOR, KORVO_SCREEN_BG_COLOR);

    snprintf(line, sizeof(line), "Decode avg %" PRIu32 ".%02" PRIu32 " max %" PRIu32 ".%02" PRIu32 " ms",
             info->decode_avg_us / 1000, (info->decode_avg_us % 1000) / 10, info->decode_max_us / 1000,
             (info->decode_max_us % 1000) / 10);
    y += KORVO_DIAG_LINE_H;
    if (err == ESP_OK) err = korvo_display_text2x(KORVO_DIAG_X, y, line, DIAG_COLS, DIAG_TEXT_COLOR, KORVO_SCREEN_BG_COLOR);

    if (info->battery_valid) {
        snprintf(line, sizeof(line), "Battery (sim) %" PRIu32 " mV", info->battery_mv);
    } else {
        snprintf(line, sizeof(line), "Battery (sim) --");
    }
    y += KORVO_DIAG_LINE_H;
    if (err == ESP_OK) err = korvo_display_text2x(KORVO_DIAG_X, y, line, DIAG_COLS, DIAG_TEXT_COLOR, KORVO_SCREEN_BG_COLOR);

    snprintf(line, sizeof(line), "Heap free %" PRIu32 " KB (internal %u KB)", esp_get_free_heap_size() / 1024,
             (unsigned)(heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024));
    y += KORVO_DIAG_LINE_H;
    if (err == ESP_OK) err = korvo_display_text2x(KORVO_DIAG_X, y, line, DIAG_COLS, DIAG_TEXT_COLOR, KORVO_SCREEN_BG_COLOR);

    const uint32_t up_s = (uint32_t)(esp_timer_get_time() / 1000000);
    snprintf(line, sizeof(line), "Uptime %02" PRIu32 ":%02" PRIu32 ":%02" PRIu32, up_s / 3600, (up_s / 60) % 60,
             up_s % 60);
    y += KORVO_DIAG_LINE_H;
    if (err == ESP_OK) err = korvo_display_text2x(KORVO_DIAG_X, y, line, DIAG_COLS, DIAG_TEXT_COLOR, KORVO_SCREEN_BG_COLOR);
    return err;
}
