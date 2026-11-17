#pragma once

#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

typedef struct {
    uint32_t sample_rate_hz;
    uint32_t tone_hz;
    int16_t peak;             /* maximum absolute amplitude */
    uint32_t duration_frames; /* total frames to generate */
    uint32_t ramp_frames;     /* linear fade-in and fade-out length */
    uint32_t channels;        /* same sample written to every channel */
} tone_gen_config_t;

typedef struct {
    tone_gen_config_t cfg;
    uint32_t frame;     /* frames already produced */
    double phase;       /* radians, kept in [0, 2*pi) across calls */
    double phase_step;
} tone_gen_t;

/* All fields > 0 and 2 * ramp_frames <= duration_frames; otherwise ESP_ERR_INVALID_ARG. */
esp_err_t tone_gen_init(tone_gen_t *gen, const tone_gen_config_t *cfg);
/* Writes up to max_frames interleaved frames into buf; returns frames written (0 when finished). */
size_t tone_gen_fill(tone_gen_t *gen, int16_t *buf, size_t max_frames);
