#include "volume.h"
#include <assert.h>
#include <stdio.h>

static const volume_config_t CFG = {.max_db = -12, .min_db = -60, .step_db = 2, .startup_db = -30};

int main(void)
{
    volume_t v;
    assert(volume_init(&v, &CFG) == ESP_OK);
    assert(volume_get_db(&v) == -30 && !volume_is_silent(&v));
    assert(volume_up(&v) == -28 && volume_down(&v) == -30);

    for (int i = 0; i < 100; ++i) assert(volume_up(&v) <= -12);
    assert(volume_get_db(&v) == -12);
    for (int i = 0; i < 100; ++i) assert(volume_down(&v) >= -60);
    assert(volume_get_db(&v) == -60 && volume_is_silent(&v));

    /* Step that does not divide the range still clamps exactly. */
    volume_config_t odd = {.max_db = -12, .min_db = -60, .step_db = 7, .startup_db = -30};
    assert(volume_init(&v, &odd) == ESP_OK);
    for (int i = 0; i < 100; ++i) assert(volume_up(&v) <= -12);
    assert(volume_get_db(&v) == -12);
    for (int i = 0; i < 100; ++i) assert(volume_down(&v) >= -60);
    assert(volume_get_db(&v) == -60);

    volume_config_t bad = CFG;
    bad.step_db = 0;
    assert(volume_init(&v, &bad) == ESP_ERR_INVALID_ARG);
    bad = CFG; bad.step_db = -2;
    assert(volume_init(&v, &bad) == ESP_ERR_INVALID_ARG);
    bad = CFG; bad.startup_db = -60; /* min must be < startup */
    assert(volume_init(&v, &bad) == ESP_ERR_INVALID_ARG);
    bad = CFG; bad.startup_db = -10; /* startup above max */
    assert(volume_init(&v, &bad) == ESP_ERR_INVALID_ARG);
    bad = CFG; bad.startup_db = -12; /* startup == max is valid */
    assert(volume_init(&v, &bad) == ESP_OK);
    assert(volume_init(NULL, &CFG) == ESP_ERR_INVALID_ARG);
    assert(volume_init(&v, NULL) == ESP_ERR_INVALID_ARG);

    puts("volume_test: OK");
    return 0;
}
