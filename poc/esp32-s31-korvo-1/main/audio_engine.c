#include "audio_engine.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/* BSP espressif/esp32_s31_korvo_1 1.0.1: bsp_audio_codec_speaker_init() -> handle or NULL.
 * espressif/esp_codec_dev 1.5.11: open/close/write/set_out_vol/set_out_mute return ESP_CODEC_DEV_OK.
 * ESP-IDF 6.1 FreeRTOS: StackType_t is uint8_t, so the task stack size is in bytes. */
#include "bsp/esp32_s31_korvo_1.h"
#include "esp_codec_dev.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "korvo_audio_caps.h"
#include "korvo_volume.h"
#include "mp3_decoder.h"

#define ENGINE_STACK_BYTES 32768 /* minimp3 keeps ~16 KB of scratch on the stack per frame */
#define ENGINE_PRIORITY 6
#define ENGINE_QUEUE_LEN 8
#define ENGINE_OUT_CHANNELS 2
#define ENGINE_ZERO_FRAMES 1152 /* one MP3 frame of silence */

typedef enum { CMD_OPEN, CMD_PAUSE, CMD_RESUME, CMD_CLOSE, CMD_RESTART, CMD_SET_VOLUME, CMD_SHUTDOWN } cmd_kind_t;

typedef struct {
    cmd_kind_t kind;
    int value;
    char path[AUDIO_ENGINE_PATH_MAX];
} engine_cmd_t;

static const char *TAG = "audio_engine";

static QueueHandle_t s_queue;
static SemaphoreHandle_t s_shutdown_done;
static audio_engine_cb_t s_cb;
static void *s_cb_user;

/* Engine-task state. */
static esp_codec_dev_handle_t s_spk;
static bool s_codec_open;
static uint32_t s_codec_rate;
static int s_volume_pct = 0;
static bool s_paused = true;
static FILE *s_file;
static uint32_t s_gen;
static uint32_t s_rate;
static uint32_t s_channels;
static volatile uint32_t s_samples_played; /* 32-bit: atomic read on RISC-V; ~24 h at 48 kHz */

/* Large buffers are static (not on the task stack). */
static mp3_decoder_t s_dec;
static int16_t s_pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];
static int16_t s_stereo[MINIMP3_MAX_SAMPLES_PER_FRAME];
static int16_t s_zeros[ENGINE_ZERO_FRAMES * ENGINE_OUT_CHANNELS];
static size_t s_pending_spc; /* first frame decoded during OPEN, not yet written */
static engine_cmd_t s_cmd;   /* receive buffer, keeps 300+ bytes off the stack */

static void notify(audio_engine_event_t evt, esp_err_t err)
{
    if (s_cb) s_cb(evt, s_gen, err, s_cb_user);
}

static int file_read(void *ctx, void *buf, size_t len)
{
    FILE *f = ctx;
    const size_t n = fread(buf, 1, len, f);
    if (n < len && ferror(f)) return -1;
    return (int)n;
}

static void write_zeros(void)
{
    if (!s_codec_open) return;
    const int ret = esp_codec_dev_write(s_spk, s_zeros, (int)sizeof(s_zeros));
    if (ret != ESP_CODEC_DEV_OK) ESP_LOGW(TAG, "zero block write failed: %d", ret);
}

static void set_mute(bool mute)
{
    if (!s_codec_open) return;
    const int ret = esp_codec_dev_set_out_mute(s_spk, mute);
    if (ret != ESP_CODEC_DEV_OK) ESP_LOGW(TAG, "esp_codec_dev_set_out_mute(%d) failed: %d", mute, ret);
}

static void close_file(void)
{
    if (s_file) {
        fclose(s_file);
        s_file = NULL;
    }
    s_pending_spc = 0;
    s_samples_played = 0;
}

static void close_codec(void)
{
    if (!s_codec_open) return;
    const int ret = esp_codec_dev_close(s_spk);
    if (ret != ESP_CODEC_DEV_OK) ESP_LOGW(TAG, "esp_codec_dev_close failed: %d", ret);
    s_codec_open = false;
    s_codec_rate = 0;
}

/* Opens the codec at rate, or reopens it if the rate changed between tracks. */
static esp_err_t ensure_codec(uint32_t rate)
{
    if (s_codec_open && s_codec_rate == rate) return ESP_OK;
    close_codec();
    esp_codec_dev_sample_info_t fs = {
        .bits_per_sample = 16,
        .channel = ENGINE_OUT_CHANNELS,
        .sample_rate = rate,
    };
    int ret = esp_codec_dev_open(s_spk, &fs);
    if (ret != ESP_CODEC_DEV_OK) {
        ESP_LOGE(TAG, "esp_codec_dev_open(%" PRIu32 " Hz) failed: %d", rate, ret);
        return ESP_FAIL;
    }
    s_codec_open = true;
    s_codec_rate = rate;
    ret = esp_codec_dev_set_out_vol(s_spk, s_volume_pct);
    if (ret != ESP_CODEC_DEV_OK) ESP_LOGW(TAG, "esp_codec_dev_set_out_vol(%d) failed: %d", s_volume_pct, ret);
    set_mute(s_paused);
    printf("[ENGINE] codec open at %" PRIu32 " Hz\n", rate);
    return ESP_OK;
}

static void do_open(const char *path)
{
    close_file();
    s_gen++;
    s_file = fopen(path, "rb");
    if (s_file == NULL) {
        ESP_LOGE(TAG, "cannot open %s", path);
        notify(AUDIO_ENGINE_EVT_TRACK_ERROR, ESP_ERR_NOT_FOUND);
        return;
    }
    esp_err_t err = mp3_decoder_init(&s_dec, file_read, s_file);
    mp3_frame_info_t info = {0};
    size_t spc = 0;
    if (err == ESP_OK) err = mp3_decoder_next(&s_dec, s_pcm, &info, &spc);
    if (err == ESP_OK && spc == 0) err = ESP_ERR_NOT_FOUND; /* no audio frame */
    if (err == ESP_OK && !korvo_audio_rate_supported(info.sample_rate)) {
        ESP_LOGE(TAG, "sample rate %u Hz not supported by ES8389 without MCLK on Korvo (no coeff entry)",
                 (unsigned)info.sample_rate);
        err = ESP_ERR_NOT_SUPPORTED;
    }
    if (err == ESP_OK) err = ensure_codec(info.sample_rate);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "open %s failed: %s", path, esp_err_to_name(err));
        close_file();
        notify(AUDIO_ENGINE_EVT_TRACK_ERROR, err);
        return;
    }
    s_rate = info.sample_rate;
    s_channels = info.channels;
    s_pending_spc = spc;
    printf("[ENGINE] open %s: %" PRIu32 " Hz, %" PRIu32 " ch, %" PRIu32 " kbps\n", path, info.sample_rate,
           info.channels, info.bitrate_kbps);
    notify(AUDIO_ENGINE_EVT_OPENED, ESP_OK);
}

static void do_restart(void)
{
    if (s_file == NULL) return;
    s_pending_spc = 0;
    s_samples_played = 0;
    esp_err_t err = fseek(s_file, 0, SEEK_SET) == 0 ? ESP_OK : ESP_FAIL;
    if (err == ESP_OK) err = mp3_decoder_init(&s_dec, file_read, s_file);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "restart failed: %s", esp_err_to_name(err));
        close_file();
        notify(AUDIO_ENGINE_EVT_TRACK_ERROR, err);
    }
}

/* Returns false when the task must exit. */
static bool handle_cmd(const engine_cmd_t *cmd)
{
    switch (cmd->kind) {
    case CMD_OPEN:
        do_open(cmd->path);
        break;
    case CMD_PAUSE:
        /* DAC mute stage. The amplifier (AMP) stage is a no-op on the Korvo: the ES8389 driver owns
         * the PA pin (BSP_POWER_AMP_IO) and this PoC does not sequence it per pause. */
        set_mute(true);
        write_zeros();
        s_paused = true;
        break;
    case CMD_RESUME:
        write_zeros();
        set_mute(false); /* AMP stage: no-op on the Korvo, see CMD_PAUSE */
        s_paused = false;
        break;
    case CMD_CLOSE:
        close_file();
        break;
    case CMD_RESTART:
        do_restart();
        break;
    case CMD_SET_VOLUME:
        s_volume_pct = korvo_volume_db_to_percent(cmd->value);
        if (s_codec_open) {
            const int ret = esp_codec_dev_set_out_vol(s_spk, s_volume_pct);
            if (ret != ESP_CODEC_DEV_OK) ESP_LOGW(TAG, "esp_codec_dev_set_out_vol failed: %d", ret);
        }
        break;
    case CMD_SHUTDOWN:
        close_file();
        set_mute(true);
        close_codec();
        return false;
    }
    return true;
}

/* Writes one decoded frame (pending first frame or a new one). */
static void play_one_frame(void)
{
    size_t spc = s_pending_spc;
    s_pending_spc = 0;
    if (spc == 0) {
        mp3_frame_info_t info;
        const esp_err_t err = mp3_decoder_next(&s_dec, s_pcm, &info, &spc);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "decode failed after %" PRIu32 " frames: %s", mp3_decoder_frames(&s_dec),
                     esp_err_to_name(err));
            close_file();
            notify(AUDIO_ENGINE_EVT_TRACK_ERROR, err);
            return;
        }
        if (spc == 0) {
            close_file();
            notify(AUDIO_ENGINE_EVT_TRACK_END, ESP_OK);
            return;
        }
    }
    int16_t *out = s_pcm;
    if (s_channels == 1) {
        for (size_t i = 0; i < spc; ++i) {
            s_stereo[2 * i] = s_pcm[i];
            s_stereo[2 * i + 1] = s_pcm[i];
        }
        out = s_stereo;
    }
    const int ret = esp_codec_dev_write(s_spk, out, (int)(spc * ENGINE_OUT_CHANNELS * sizeof(int16_t)));
    if (ret != ESP_CODEC_DEV_OK) {
        ESP_LOGE(TAG, "esp_codec_dev_write failed: %d", ret);
        close_file();
        notify(AUDIO_ENGINE_EVT_TRACK_ERROR, ESP_FAIL);
        return;
    }
    s_samples_played += (uint32_t)spc;
}

static void engine_task(void *arg)
{
    (void)arg;
    for (;;) {
        const bool feeding = s_file != NULL && !s_paused;
        /* Between frames: poll without blocking. Paused or closed: block until a command. */
        if (xQueueReceive(s_queue, &s_cmd, feeding ? 0 : portMAX_DELAY) == pdTRUE) {
            if (!handle_cmd(&s_cmd)) break;
            continue; /* drain all pending commands before the next frame */
        }
        if (feeding) play_one_frame();
    }
    printf("[ENGINE] shutdown: file and codec closed\n");
    xSemaphoreGive(s_shutdown_done);
    vTaskDelete(NULL);
}

static esp_err_t send(cmd_kind_t kind, int value, const char *path)
{
    if (s_queue == NULL) return ESP_ERR_INVALID_STATE;
    static engine_cmd_t cmd; /* only the player task sends; keeps the caller's stack small */
    cmd.kind = kind;
    cmd.value = value;
    cmd.path[0] = '\0';
    if (path) {
        if (strlen(path) >= sizeof(cmd.path)) return ESP_ERR_INVALID_ARG;
        strcpy(cmd.path, path);
    }
    return xQueueSend(s_queue, &cmd, pdMS_TO_TICKS(100)) == pdTRUE ? ESP_OK : ESP_ERR_TIMEOUT;
}

esp_err_t audio_engine_init(audio_engine_cb_t cb, void *user)
{
    if (s_queue != NULL) return ESP_ERR_INVALID_STATE;
    s_cb = cb;
    s_cb_user = user;
    s_spk = bsp_audio_codec_speaker_init();
    if (s_spk == NULL) {
        ESP_LOGE(TAG, "bsp_audio_codec_speaker_init returned NULL");
        return ESP_FAIL;
    }
    s_queue = xQueueCreate(ENGINE_QUEUE_LEN, sizeof(engine_cmd_t));
    s_shutdown_done = xSemaphoreCreateBinary();
    if (s_queue == NULL || s_shutdown_done == NULL) return ESP_ERR_NO_MEM;
    if (xTaskCreate(engine_task, "audio_engine", ENGINE_STACK_BYTES, NULL, ENGINE_PRIORITY, NULL) != pdPASS) {
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

esp_err_t audio_engine_open(const char *path) { return send(CMD_OPEN, 0, path); }
esp_err_t audio_engine_pause(void) { return send(CMD_PAUSE, 0, NULL); }
esp_err_t audio_engine_resume(void) { return send(CMD_RESUME, 0, NULL); }
esp_err_t audio_engine_close(void) { return send(CMD_CLOSE, 0, NULL); }
esp_err_t audio_engine_restart(void) { return send(CMD_RESTART, 0, NULL); }
esp_err_t audio_engine_set_volume(int db) { return send(CMD_SET_VOLUME, db, NULL); }

esp_err_t audio_engine_shutdown(uint32_t timeout_ms)
{
    const esp_err_t err = send(CMD_SHUTDOWN, 0, NULL);
    if (err != ESP_OK) return err;
    return xSemaphoreTake(s_shutdown_done, pdMS_TO_TICKS(timeout_ms)) == pdTRUE ? ESP_OK : ESP_ERR_TIMEOUT;
}

uint32_t audio_engine_position_ms(void)
{
    const uint32_t rate = s_rate;
    const uint64_t samples = s_samples_played;
    return rate ? (uint32_t)(samples * 1000u / rate) : 0; /* 64-bit product avoids overflow */
}
