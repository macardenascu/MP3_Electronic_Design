#include "mute_sequencer.h"

static esp_err_t emit(const mute_step_t *steps, size_t count, mute_step_t out[], size_t cap,
                      size_t *n)
{
    *n = 0;
    if (!out || cap < count) return ESP_ERR_INVALID_SIZE;
    for (size_t i = 0; i < count; ++i) out[i] = steps[i];
    *n = count;
    return ESP_OK;
}

esp_err_t mute_seq_unmute(const mute_seq_config_t *cfg, mute_step_t out[], size_t cap, size_t *n)
{
    if (!n) return ESP_ERR_INVALID_ARG;
    *n = 0;
    if (!cfg) return ESP_ERR_INVALID_ARG;
    const mute_step_t steps[MUTE_SEQ_UNMUTE_STEPS] = {
        {STEP_WAIT_MS, cfg->settle_ms},
        {STEP_DAC_UNMUTE, 0},
        {STEP_WAIT_MS, cfg->stage_ms},
        {STEP_AMP_UNMUTE, 0},
    };
    return emit(steps, MUTE_SEQ_UNMUTE_STEPS, out, cap, n);
}

esp_err_t mute_seq_mute(const mute_seq_config_t *cfg, mute_step_t out[], size_t cap, size_t *n)
{
    if (!n) return ESP_ERR_INVALID_ARG;
    *n = 0;
    if (!cfg) return ESP_ERR_INVALID_ARG;
    const mute_step_t steps[MUTE_SEQ_MUTE_STEPS] = {
        {STEP_AMP_MUTE, 0},
        {STEP_WAIT_MS, cfg->stage_ms},
        {STEP_DAC_MUTE, 0},
    };
    return emit(steps, MUTE_SEQ_MUTE_STEPS, out, cap, n);
}
