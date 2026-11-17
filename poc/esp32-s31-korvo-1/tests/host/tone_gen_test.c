#include "tone_gen.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static tone_gen_config_t base(void)
{
    tone_gen_config_t cfg = {.sample_rate_hz = 16000, .tone_hz = 440, .peak = 8192,
                             .duration_frames = 16000, .ramp_frames = 1600, .channels = 2};
    return cfg;
}

int main(void)
{
    static int16_t buf[1000 * 4];
    const tone_gen_config_t cfg = base();
    tone_gen_t g;

    /* Exact total frame count with several buffer sizes. */
    const size_t sizes[] = {1, 7, 160, 1000};
    for (size_t s = 0; s < sizeof(sizes) / sizeof(sizes[0]); ++s) {
        assert(tone_gen_init(&g, &cfg) == ESP_OK);
        size_t total = 0, n;
        while ((n = tone_gen_fill(&g, buf, sizes[s])) > 0) {
            assert(n <= sizes[s]);
            total += n;
        }
        assert(total == cfg.duration_frames);
        assert(tone_gen_fill(&g, buf, sizes[s]) == 0);
    }

    /* First sample 0, |sample| <= peak, identical channels, ~440 rising zero crossings. */
    assert(tone_gen_init(&g, &cfg) == ESP_OK);
    size_t n, frame = 0;
    int prev = 0, crossings = 0, last = 1;
    while ((n = tone_gen_fill(&g, buf, 160)) > 0) {
        for (size_t i = 0; i < n; ++i, ++frame) {
            int l = buf[2 * i], r = buf[2 * i + 1];
            assert(l == r);
            assert(abs(l) <= cfg.peak);
            if (frame == 0) assert(l == 0);
            if (frame > 0 && prev < 0 && l >= 0) ++crossings;
            prev = l;
            last = l;
        }
    }
    assert(last == 0); /* fade-out ends at silence */
    printf("tone_gen_test: rising zero crossings in 1 s = %d\n", crossings);
    assert(crossings >= 438 && crossings <= 442);

    /* Mono and 4 channels also fill every channel. */
    tone_gen_config_t quad = cfg;
    quad.channels = 4;
    assert(tone_gen_init(&g, &quad) == ESP_OK);
    for (int k = 0; k < 20; ++k) {
        n = tone_gen_fill(&g, buf, 1000);
        for (size_t i = 0; i < n; ++i)
            for (int c = 1; c < 4; ++c) assert(buf[4 * i + c] == buf[4 * i]);
    }

    /* Invalid configs. */
    tone_gen_config_t bad;
    bad = cfg; bad.sample_rate_hz = 0; assert(tone_gen_init(&g, &bad) == ESP_ERR_INVALID_ARG);
    bad = cfg; bad.tone_hz = 0; assert(tone_gen_init(&g, &bad) == ESP_ERR_INVALID_ARG);
    bad = cfg; bad.peak = 0; assert(tone_gen_init(&g, &bad) == ESP_ERR_INVALID_ARG);
    bad = cfg; bad.peak = -5; assert(tone_gen_init(&g, &bad) == ESP_ERR_INVALID_ARG);
    bad = cfg; bad.duration_frames = 0; assert(tone_gen_init(&g, &bad) == ESP_ERR_INVALID_ARG);
    bad = cfg; bad.ramp_frames = 0; assert(tone_gen_init(&g, &bad) == ESP_ERR_INVALID_ARG);
    bad = cfg; bad.channels = 0; assert(tone_gen_init(&g, &bad) == ESP_ERR_INVALID_ARG);
    bad = cfg; bad.ramp_frames = 8001; assert(tone_gen_init(&g, &bad) == ESP_ERR_INVALID_ARG);
    bad = cfg; bad.ramp_frames = 8000; assert(tone_gen_init(&g, &bad) == ESP_OK);
    bad = cfg; bad.duration_frames = 3; bad.ramp_frames = 1;
    assert(tone_gen_init(&g, &bad) == ESP_OK);
    assert(tone_gen_init(NULL, &cfg) == ESP_ERR_INVALID_ARG);
    assert(tone_gen_init(&g, NULL) == ESP_ERR_INVALID_ARG);

    puts("tone_gen_test: OK");
    return 0;
}
