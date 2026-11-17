#include "tone_gen.h"

#include <math.h>

#define TONE_TWO_PI 6.283185307179586

esp_err_t tone_gen_init(tone_gen_t *gen, const tone_gen_config_t *cfg)
{
    if (!gen || !cfg) return ESP_ERR_INVALID_ARG;
    if (cfg->sample_rate_hz == 0 || cfg->tone_hz == 0 || cfg->peak <= 0 ||
        cfg->duration_frames == 0 || cfg->ramp_frames == 0 || cfg->channels == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    if (cfg->ramp_frames > cfg->duration_frames / 2) return ESP_ERR_INVALID_ARG;
    gen->cfg = *cfg;
    gen->frame = 0;
    gen->phase = 0.0;
    gen->phase_step = TONE_TWO_PI * (double)cfg->tone_hz / (double)cfg->sample_rate_hz;
    return ESP_OK;
}

/* Linear envelope in [0, 1]: rises over the first ramp, falls to 0 at the last frame. */
static double envelope(const tone_gen_config_t *cfg, uint32_t k)
{
    const uint32_t last = cfg->duration_frames - 1;
    double g = 1.0;
    if (k < cfg->ramp_frames) g = (double)k / (double)cfg->ramp_frames;
    const uint32_t from_end = last - k;
    if (from_end < cfg->ramp_frames) {
        double tail = (double)from_end / (double)cfg->ramp_frames;
        if (tail < g) g = tail;
    }
    return g;
}

size_t tone_gen_fill(tone_gen_t *gen, int16_t *buf, size_t max_frames)
{
    if (!gen || !buf) return 0;
    size_t written = 0;
    while (written < max_frames && gen->frame < gen->cfg.duration_frames) {
        double v = (double)gen->cfg.peak * envelope(&gen->cfg, gen->frame) * sin(gen->phase);
        long s = lround(v);
        if (s > gen->cfg.peak) s = gen->cfg.peak;
        if (s < -gen->cfg.peak) s = -gen->cfg.peak;
        for (uint32_t c = 0; c < gen->cfg.channels; ++c) {
            buf[written * gen->cfg.channels + c] = (int16_t)s;
        }
        gen->phase += gen->phase_step;
        if (gen->phase >= TONE_TWO_PI) gen->phase -= TONE_TWO_PI;
        ++gen->frame;
        ++written;
    }
    return written;
}
