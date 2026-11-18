#include "audio_bsp_mp3.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* BSP espressif/esp32_s31_korvo_1 1.0.1: bsp_audio_codec_speaker_init() -> handle or NULL.
 * espressif/esp_codec_dev 1.5.11: open/set_out_vol/write/close return ESP_CODEC_DEV_OK (0).
 * ESP-IDF 6.1 FreeRTOS: StackType_t is uint8_t, so xTaskCreate() stack depth and
 * uxTaskGetStackHighWaterMark() are in bytes. esp_timer_get_time(): microseconds. */
#include "bsp/esp32_s31_korvo_1.h"
#include "esp_codec_dev.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "korvo_audio_caps.h"
#include "mp3_decoder.h"
#include "storage_korvo.h"

#define MP3_TASK_STACK_BYTES 32768 /* minimp3 keeps ~16 KB of scratch on the stack per frame */
#define MP3_TASK_PRIORITY 5
#define MP3_OUT_CHANNELS 2
#define MP3_VOLUME 45
#define MP3_PATH_MAX 128

static const char *TAG = "audio_bsp_mp3";
static mp3_decoder_t s_dec;
static int16_t s_pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];
static int16_t s_stereo[MINIMP3_MAX_SAMPLES_PER_FRAME]; /* mono frame (<=1152) duplicated to L/R */
static char s_path[MP3_PATH_MAX];

typedef struct {
    uint64_t samples_per_ch;
    int64_t decode_total_us;
    int64_t decode_max_us;
    uint32_t decode_calls;
    int64_t t_start;
    int64_t t_end;
    uint32_t stack_free_min;
} mp3_stats_t;

static int file_read(void *ctx, void *buf, size_t len)
{
    FILE *f = ctx;
    const size_t n = fread(buf, 1, len, f);
    if (n < len && ferror(f)) return -1;
    return (int)n;
}

static void track_stack(mp3_stats_t *st)
{
    const uint32_t free_bytes = (uint32_t)uxTaskGetStackHighWaterMark(NULL);
    if (free_bytes < st->stack_free_min) st->stack_free_min = free_bytes;
}

static esp_err_t decode_timed(mp3_stats_t *st, mp3_frame_info_t *info, size_t *spc)
{
    const int64_t t0 = esp_timer_get_time();
    const esp_err_t err = mp3_decoder_next(&s_dec, s_pcm, info, spc);
    const int64_t dt = esp_timer_get_time() - t0;
    st->decode_total_us += dt;
    st->decode_calls++;
    if (dt > st->decode_max_us) st->decode_max_us = dt;
    track_stack(st);
    return err;
}

static esp_err_t write_frame(esp_codec_dev_handle_t spk, const mp3_frame_info_t *info, size_t spc,
                             mp3_stats_t *st)
{
    int16_t *out = s_pcm;
    if (info->channels == 1) {
        for (size_t i = 0; i < spc; ++i) {
            s_stereo[2 * i] = s_pcm[i];
            s_stereo[2 * i + 1] = s_pcm[i];
        }
        out = s_stereo;
    }
    const int len = (int)(spc * MP3_OUT_CHANNELS * sizeof(int16_t));
    if (st->t_start == 0) st->t_start = esp_timer_get_time();
    const int ret = esp_codec_dev_write(spk, out, len);
    if (ret != ESP_CODEC_DEV_OK) {
        ESP_LOGE(TAG, "esp_codec_dev_write failed: %d", ret);
        return ESP_FAIL;
    }
    st->t_end = esp_timer_get_time();
    st->samples_per_ch += spc;
    return ESP_OK;
}

static esp_err_t play(FILE *f, mp3_stats_t *st, uint32_t *rate_out)
{
    esp_err_t err = mp3_decoder_init(&s_dec, file_read, f);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "mp3_decoder_init failed: %s", esp_err_to_name(err));
        return err;
    }

    /* The first frame fixes the output format before the codec is opened. */
    mp3_frame_info_t info;
    size_t spc = 0;
    err = decode_timed(st, &info, &spc);
    if (err == ESP_OK && spc == 0) err = ESP_ERR_NOT_FOUND; /* no audio frames at all */
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "first frame: %s", esp_err_to_name(err));
        return err;
    }
    *rate_out = info.sample_rate;
    printf("[MP3] file=%s, rate=%" PRIu32 ", ch=%" PRIu32 ", bitrate=%" PRIu32 " kbps\n",
           s_path, info.sample_rate, info.channels, info.bitrate_kbps);

    if (!korvo_audio_rate_supported(info.sample_rate)) {
        ESP_LOGE(TAG, "sample rate %u Hz not supported by ES8389 without MCLK on Korvo (no coeff entry)",
                 (unsigned)info.sample_rate);
        return ESP_ERR_NOT_SUPPORTED; /* mp3_task closes the file */
    }
    esp_codec_dev_handle_t spk = bsp_audio_codec_speaker_init();
    if (spk == NULL) {
        ESP_LOGE(TAG, "bsp_audio_codec_speaker_init returned NULL");
        return ESP_FAIL;
    }
    esp_codec_dev_sample_info_t fs = {
        .bits_per_sample = 16,
        .channel = MP3_OUT_CHANNELS,
        .sample_rate = info.sample_rate,
    };
    int ret = esp_codec_dev_open(spk, &fs);
    if (ret != ESP_CODEC_DEV_OK) {
        ESP_LOGE(TAG, "esp_codec_dev_open failed: %d", ret);
        return ESP_FAIL;
    }

    ret = esp_codec_dev_set_out_vol(spk, MP3_VOLUME);
    if (ret != ESP_CODEC_DEV_OK) {
        ESP_LOGE(TAG, "esp_codec_dev_set_out_vol(%d) failed: %d", MP3_VOLUME, ret);
        err = ESP_FAIL;
    } else {
        printf("[MP3] playing at vol=%d\n", MP3_VOLUME);
        err = write_frame(spk, &info, spc, st);
        while (err == ESP_OK) {
            err = decode_timed(st, &info, &spc);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "decode failed after %" PRIu32 " frames: %s",
                         mp3_decoder_frames(&s_dec), esp_err_to_name(err));
                break;
            }
            if (spc == 0) break; /* end of file */
            err = write_frame(spk, &info, spc, st);
        }
    }

    ret = esp_codec_dev_close(spk);
    if (ret != ESP_CODEC_DEV_OK) {
        ESP_LOGE(TAG, "esp_codec_dev_close failed: %d", ret);
        if (err == ESP_OK) err = ESP_FAIL;
    }
    return err;
}

static void mp3_task(void *arg)
{
    (void)arg;
    mp3_stats_t st = {.stack_free_min = UINT32_MAX};
    uint32_t rate = 0;
    esp_err_t err = storage_korvo_mount();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "SD mount failed: %s", esp_err_to_name(err));
    } else {
        FILE *f = fopen(s_path, "rb");
        if (f == NULL) {
            ESP_LOGE(TAG, "cannot open %s", s_path);
            err = ESP_ERR_NOT_FOUND;
        } else {
            err = play(f, &st, &rate);
            fclose(f);
        }
    }
    track_stack(&st);

    const int64_t elapsed_ms = (st.t_end - st.t_start) / 1000;
    const uint64_t duration_ms = rate ? st.samples_per_ch * 1000u / rate : 0;
    const double avg_ms = st.decode_calls ? (double)st.decode_total_us / st.decode_calls / 1000.0 : 0.0;
    printf("[MP3] done: frames=%" PRIu32 " samples=%" PRIu64 " elapsed=%" PRId64 " ms duration=%" PRIu64
           " ms ratio=%.3f decode_avg=%.2f ms decode_max=%.2f ms skipped=%" PRIu64
           " bytes stack_free_min=%" PRIu32 " bytes\n",
           mp3_decoder_frames(&s_dec), st.samples_per_ch, elapsed_ms, duration_ms,
           duration_ms ? (double)elapsed_ms / (double)duration_ms : 0.0, avg_ms,
           (double)st.decode_max_us / 1000.0, mp3_decoder_skipped_bytes(&s_dec), st.stack_free_min);
    printf("[MP3] finished: %s (software status, not audible validation)\n", esp_err_to_name(err));
    vTaskDelete(NULL);
}

esp_err_t audio_bsp_mp3_start(const char *path)
{
    if (path == NULL || strlen(path) >= sizeof(s_path)) return ESP_ERR_INVALID_ARG;
    strcpy(s_path, path);
    if (xTaskCreate(mp3_task, "mp3_play", MP3_TASK_STACK_BYTES, NULL, MP3_TASK_PRIORITY, NULL) != pdPASS) {
        ESP_LOGE(TAG, "xTaskCreate(mp3_play) failed");
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}
