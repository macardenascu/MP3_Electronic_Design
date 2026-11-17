#include "audio_bsp_tone.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* BSP espressif/esp32_s31_korvo_1 1.0.1:
 *   bsp_audio_codec_speaker_init() -> esp_codec_dev_handle_t (NULL on failure);
 *   it calls bsp_audio_init(NULL) internally when I2S is not yet initialised.
 * espressif/esp_codec_dev 1.5.x (esp_codec_dev.h):
 *   esp_codec_dev_open(), esp_codec_dev_set_out_vol(), esp_codec_dev_write(),
 *   esp_codec_dev_close(); all return ESP_CODEC_DEV_OK (0) on success. */
#include "bsp/esp32_s31_korvo_1.h"
#include "esp_codec_dev.h"
#include "esp_log.h"
#include "tone_gen.h"

#define TONE_SAMPLE_RATE 16000
#define TONE_CHANNELS 2
#define TONE_HZ 440
#define TONE_PEAK 8192 /* 25 % of full scale */
#define TONE_SECONDS 2
#define TONE_RAMP_MS 100
#define TONE_SILENCE_MS 1000
#define TONE_CHUNK_FRAMES 160

static const char *TAG = "audio_bsp_tone";
static int16_t s_buf[TONE_CHUNK_FRAMES * TONE_CHANNELS];

static esp_err_t write_chunk(esp_codec_dev_handle_t spk, size_t frames)
{
    const int len = (int)(frames * TONE_CHANNELS * sizeof(int16_t));
    const int ret = esp_codec_dev_write(spk, s_buf, len);
    if (ret != ESP_CODEC_DEV_OK) {
        ESP_LOGE(TAG, "esp_codec_dev_write failed: %d", ret);
        return ESP_FAIL;
    }
    return ESP_OK;
}

/* Silence is written as zero samples so the I2S clock keeps running between tones. */
static esp_err_t write_silence(esp_codec_dev_handle_t spk, uint32_t ms)
{
    memset(s_buf, 0, sizeof(s_buf));
    uint32_t remaining = TONE_SAMPLE_RATE * ms / 1000;
    while (remaining > 0) {
        const size_t frames = remaining < TONE_CHUNK_FRAMES ? remaining : TONE_CHUNK_FRAMES;
        esp_err_t err = write_chunk(spk, frames);
        if (err != ESP_OK) return err;
        remaining -= (uint32_t)frames;
    }
    return ESP_OK;
}

static esp_err_t play_tone(esp_codec_dev_handle_t spk)
{
    const tone_gen_config_t cfg = {
        .sample_rate_hz = TONE_SAMPLE_RATE,
        .tone_hz = TONE_HZ,
        .peak = TONE_PEAK,
        .duration_frames = TONE_SAMPLE_RATE * TONE_SECONDS,
        .ramp_frames = TONE_SAMPLE_RATE * TONE_RAMP_MS / 1000,
        .channels = TONE_CHANNELS,
    };
    tone_gen_t gen;
    esp_err_t err = tone_gen_init(&gen, &cfg);
    if (err != ESP_OK) return err;
    size_t frames;
    while ((frames = tone_gen_fill(&gen, s_buf, TONE_CHUNK_FRAMES)) > 0) {
        err = write_chunk(spk, frames);
        if (err != ESP_OK) return err;
    }
    return ESP_OK;
}

esp_err_t audio_bsp_tone_run(void)
{
    esp_codec_dev_handle_t spk = bsp_audio_codec_speaker_init();
    if (spk == NULL) {
        ESP_LOGE(TAG, "bsp_audio_codec_speaker_init returned NULL");
        return ESP_FAIL;
    }

    esp_codec_dev_sample_info_t fs = {
        .bits_per_sample = 16,
        .channel = TONE_CHANNELS,
        .sample_rate = TONE_SAMPLE_RATE,
    };
    int ret = esp_codec_dev_open(spk, &fs);
    if (ret != ESP_CODEC_DEV_OK) {
        ESP_LOGE(TAG, "esp_codec_dev_open failed: %d", ret);
        return ESP_FAIL;
    }

    static const int volumes[] = {30, 45, 60};
    const int count = (int)(sizeof(volumes) / sizeof(volumes[0]));
    esp_err_t err = ESP_OK;
    for (int i = 0; i < count && err == ESP_OK; ++i) {
        ret = esp_codec_dev_set_out_vol(spk, volumes[i]);
        if (ret != ESP_CODEC_DEV_OK) {
            ESP_LOGE(TAG, "esp_codec_dev_set_out_vol(%d) failed: %d", volumes[i], ret);
            err = ESP_FAIL;
            break;
        }
        printf("[AUDIO] tone %d/%d: vol=%d peak=25%%\n", i + 1, count, volumes[i]);
        err = play_tone(spk);
        if (err == ESP_OK && i + 1 < count) err = write_silence(spk, TONE_SILENCE_MS);
    }

    ret = esp_codec_dev_close(spk);
    if (ret != ESP_CODEC_DEV_OK) {
        ESP_LOGE(TAG, "esp_codec_dev_close failed: %d", ret);
        if (err == ESP_OK) err = ESP_FAIL;
    }
    if (err == ESP_OK) {
        printf("[AUDIO] BSP tone test finished (software status, not audible validation)\n");
    }
    return err;
}
