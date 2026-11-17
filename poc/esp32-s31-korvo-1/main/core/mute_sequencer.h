#pragma once

#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

typedef enum {
    STEP_DAC_MUTE,
    STEP_DAC_UNMUTE,
    STEP_AMP_MUTE,
    STEP_AMP_UNMUTE,
    STEP_WAIT_MS
} mute_step_kind_t;

typedef struct {
    mute_step_kind_t kind;
    uint32_t wait_ms; /* only meaningful for STEP_WAIT_MS */
} mute_step_t;

typedef struct {
    uint32_t settle_ms; /* DAC settling after I2S start */
    uint32_t stage_ms;  /* gap between DAC and amplifier, e.g. 50 */
} mute_seq_config_t;

#define MUTE_SEQ_UNMUTE_STEPS 4
#define MUTE_SEQ_MUTE_STEPS 3

/* WAIT settle, DAC_UNMUTE, WAIT stage, AMP_UNMUTE. Short cap: ESP_ERR_INVALID_SIZE, *n = 0. */
esp_err_t mute_seq_unmute(const mute_seq_config_t *cfg, mute_step_t out[], size_t cap, size_t *n);
/* AMP_MUTE, WAIT stage, DAC_MUTE. Short cap: ESP_ERR_INVALID_SIZE, *n = 0. */
esp_err_t mute_seq_mute(const mute_seq_config_t *cfg, mute_step_t out[], size_t cap, size_t *n);
