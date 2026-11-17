#include "mute_sequencer.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
    const mute_seq_config_t cfg = {.settle_ms = 20, .stage_ms = 50};
    mute_step_t out[8];
    size_t n = 99;

    assert(mute_seq_unmute(&cfg, out, 8, &n) == ESP_OK && n == 4);
    assert(out[0].kind == STEP_WAIT_MS && out[0].wait_ms == 20);
    assert(out[1].kind == STEP_DAC_UNMUTE);
    assert(out[2].kind == STEP_WAIT_MS && out[2].wait_ms == 50);
    assert(out[3].kind == STEP_AMP_UNMUTE);

    n = 99;
    assert(mute_seq_mute(&cfg, out, 8, &n) == ESP_OK && n == 3);
    assert(out[0].kind == STEP_AMP_MUTE);
    assert(out[1].kind == STEP_WAIT_MS && out[1].wait_ms == 50);
    assert(out[2].kind == STEP_DAC_MUTE);

    /* Exact capacity works; one less fails with *n = 0. */
    assert(mute_seq_unmute(&cfg, out, 4, &n) == ESP_OK && n == 4);
    n = 99;
    assert(mute_seq_unmute(&cfg, out, 3, &n) == ESP_ERR_INVALID_SIZE && n == 0);
    assert(mute_seq_mute(&cfg, out, 3, &n) == ESP_OK && n == 3);
    n = 99;
    assert(mute_seq_mute(&cfg, out, 2, &n) == ESP_ERR_INVALID_SIZE && n == 0);
    n = 99;
    assert(mute_seq_mute(&cfg, NULL, 0, &n) == ESP_ERR_INVALID_SIZE && n == 0);
    n = 99;
    assert(mute_seq_mute(NULL, out, 8, &n) == ESP_ERR_INVALID_ARG && n == 0);

    puts("mute_sequencer_test: OK");
    return 0;
}
