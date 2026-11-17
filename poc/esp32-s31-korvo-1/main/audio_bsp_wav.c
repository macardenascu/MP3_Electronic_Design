#include "audio_bsp_wav.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

/* BSP espressif/esp32_s31_korvo_1 1.0.1: bsp_audio_codec_speaker_init() -> handle or NULL.
 * espressif/esp_codec_dev 1.5.11: open/set_out_vol/write/close return ESP_CODEC_DEV_OK (0).
 * ESP-IDF 6.1 esp_timer.h: int64_t esp_timer_get_time(void), microseconds since boot. */
#include "bsp/esp32_s31_korvo_1.h"
#include "esp_codec_dev.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "storage_korvo.h"
#include "wav_parser.h"

#define WAV_READ_BYTES 4096
#define WAV_OUT_CHANNELS 2
#define WAV_VOLUME 45

static const char *TAG = "audio_bsp_wav";
static int16_t s_read_buf[WAV_READ_BYTES / sizeof(int16_t)];
static int16_t s_stereo_buf[WAV_READ_BYTES / sizeof(int16_t) * 2]; /* mono -> L/R duplicate */

static int file_read(void *ctx, void *buf, size_t len)
{
    FILE *f = ctx;
    const size_t n = fread(buf, 1, len, f);
    if (n < len && ferror(f)) return -1;
    return (int)n;
}

/* Streams the data chunk; the file must already be positioned at the first sample. */
static esp_err_t stream_data(FILE *f, esp_codec_dev_handle_t spk, const wav_info_t *info)
{
    const uint32_t frame_bytes = (uint32_t)info->channels * sizeof(int16_t);
    const uint64_t expected = info->data_size / frame_bytes;
    const uint64_t duration_ms = expected * 1000u / info->sample_rate;
    uint32_t remaining = info->data_size - info->data_size % frame_bytes;
    uint64_t frames_written = 0;
    int64_t max_read_us = 0;
    int64_t t_start = 0;
    int64_t t_end = 0;
    esp_err_t err = ESP_OK;

    while (remaining > 0) {
        const size_t want = remaining < WAV_READ_BYTES ? remaining : WAV_READ_BYTES;
        const int64_t r0 = esp_timer_get_time();
        const size_t got = fread(s_read_buf, 1, want, f);
        const int64_t read_us = esp_timer_get_time() - r0;
        if (read_us > max_read_us) max_read_us = read_us;
        if (got < want && ferror(f)) {
            ESP_LOGE(TAG, "fread failed after %" PRIu64 " frames", frames_written);
            err = ESP_FAIL;
            break;
        }
        /* Only whole frames are written; a trailing partial frame is dropped. */
        const size_t frames = got / frame_bytes;
        if (frames == 0) {
            ESP_LOGW(TAG, "file ended early: %" PRIu32 " data bytes missing", remaining);
            break;
        }

        int16_t *out = s_read_buf;
        if (info->channels == 1) {
            for (size_t i = 0; i < frames; ++i) {
                s_stereo_buf[2 * i] = s_read_buf[i];
                s_stereo_buf[2 * i + 1] = s_read_buf[i];
            }
            out = s_stereo_buf;
        }
        const int len = (int)(frames * WAV_OUT_CHANNELS * sizeof(int16_t));
        if (t_start == 0) t_start = esp_timer_get_time();
        const int ret = esp_codec_dev_write(spk, out, len);
        if (ret != ESP_CODEC_DEV_OK) {
            ESP_LOGE(TAG, "esp_codec_dev_write failed: %d", ret);
            err = ESP_FAIL;
            break;
        }
        t_end = esp_timer_get_time();
        frames_written += frames;
        remaining -= (uint32_t)(frames * frame_bytes);
        if (got < want) {
            ESP_LOGW(TAG, "file ended early: %" PRIu32 " data bytes missing", remaining);
            break;
        }
    }

    const int64_t elapsed_ms = (t_end - t_start) / 1000;
    printf("[WAV] done: frames=%" PRIu64 " expected=%" PRIu64 " elapsed=%" PRId64
           " ms duration=%" PRIu64 " ms ratio=%.3f max_read=%.1f ms\n",
           frames_written, expected, elapsed_ms, duration_ms,
           duration_ms ? (double)elapsed_ms / (double)duration_ms : 0.0, (double)max_read_us / 1000.0);
    return err;
}

esp_err_t audio_bsp_wav_run(const char *path)
{
    esp_err_t err = storage_korvo_mount();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "SD mount failed: %s", esp_err_to_name(err));
        return err;
    }

    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        ESP_LOGE(TAG, "cannot open %s", path);
        return ESP_ERR_NOT_FOUND;
    }

    wav_info_t info;
    err = wav_parse_header(file_read, f, &info);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "WAV header rejected: %s", esp_err_to_name(err));
        fclose(f);
        return err;
    }
    if (fseek(f, (long)info.data_offset, SEEK_SET) != 0) {
        ESP_LOGE(TAG, "fseek to data offset %" PRIu32 " failed", info.data_offset);
        fclose(f);
        return ESP_FAIL;
    }
    const uint32_t frame_bytes = (uint32_t)info.channels * sizeof(int16_t);
    printf("[WAV] file=%s, rate=%" PRIu32 ", ch=%u, bits=%u, data=%" PRIu32 " bytes, duration=%" PRIu64 " ms\n",
           path, info.sample_rate, (unsigned)info.channels, (unsigned)info.bits_per_sample, info.data_size,
           (uint64_t)(info.data_size / frame_bytes) * 1000u / info.sample_rate);

    esp_codec_dev_handle_t spk = bsp_audio_codec_speaker_init();
    if (spk == NULL) {
        ESP_LOGE(TAG, "bsp_audio_codec_speaker_init returned NULL");
        fclose(f);
        return ESP_FAIL;
    }
    esp_codec_dev_sample_info_t fs = {
        .bits_per_sample = 16,
        .channel = WAV_OUT_CHANNELS,
        .sample_rate = info.sample_rate,
    };
    int ret = esp_codec_dev_open(spk, &fs);
    if (ret != ESP_CODEC_DEV_OK) {
        ESP_LOGE(TAG, "esp_codec_dev_open failed: %d", ret);
        fclose(f);
        return ESP_FAIL;
    }

    ret = esp_codec_dev_set_out_vol(spk, WAV_VOLUME);
    if (ret != ESP_CODEC_DEV_OK) {
        ESP_LOGE(TAG, "esp_codec_dev_set_out_vol(%d) failed: %d", WAV_VOLUME, ret);
        err = ESP_FAIL;
    } else {
        printf("[WAV] playing at vol=%d\n", WAV_VOLUME);
        err = stream_data(f, spk, &info);
    }

    ret = esp_codec_dev_close(spk);
    if (ret != ESP_CODEC_DEV_OK) {
        ESP_LOGE(TAG, "esp_codec_dev_close failed: %d", ret);
        if (err == ESP_OK) err = ESP_FAIL;
    }
    fclose(f);
    printf("[WAV] finished: %s (software status, not audible validation)\n", esp_err_to_name(err));
    return err;
}
