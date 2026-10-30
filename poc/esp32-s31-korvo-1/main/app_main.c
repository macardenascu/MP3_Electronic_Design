#include <stdio.h>

#include "esp_err.h"
#include "esp_log.h"
#include "audio_korvo.h"
#if !CONFIG_POC_MODE_PCM
#include "music_catalog.h"
#include "storage_korvo.h"
#endif

static const char *TAG = "player";
/* Keep the fixed-capacity catalogue out of app_main's task stack. */
#if !CONFIG_POC_MODE_PCM
static music_catalog_t s_catalog;
#endif

void app_main(void)
{
#if CONFIG_POC_MODE_PCM
    const esp_err_t audio_err = audio_korvo_play_test_tone();
    if (audio_err != ESP_OK) {
        ESP_LOGE(TAG, "PCM test failed: %s", esp_err_to_name(audio_err));
    }
#else
    printf("[PLAYER] Starting...\n");
    printf("[PLATFORM] ESP32-S31-Korvo-1\n");

    esp_err_t err = storage_korvo_mount();
    if (err != ESP_OK) {
        printf("[SD] Mount failed: %s\n", esp_err_to_name(err));
        printf("[PLAYER] Diagnostic mode; firmware remains running\n");
        ESP_LOGE(TAG, "No filesystem available");
        return;
    }
    printf("[SD] Mounted successfully at %s\n", storage_korvo_mount_point());

    err = music_catalog_scan(storage_korvo_mount_point(), &s_catalog);
    if (err == ESP_ERR_NOT_FOUND) {
        printf("[MUSIC] Directory not found: MUSIC\n");
        printf("[PLAYER] Diagnostic mode; create MUSIC on the FAT32 card\n");
        return;
    }
    if (err != ESP_OK) {
        printf("[MUSIC] Scan failed: %s\n", esp_err_to_name(err));
        printf("[PLAYER] Diagnostic mode\n");
        return;
    }

    printf("[MUSIC] Directory found\n[MUSIC] Tracks: %u\n", (unsigned)s_catalog.count);
    for (size_t i = 0; i < s_catalog.count; ++i) {
        printf("[TRACK %03u] %s\n", (unsigned)(i + 1), s_catalog.tracks[i].name);
    }
    printf("[PLAYER] Ready\n");
#endif
}
