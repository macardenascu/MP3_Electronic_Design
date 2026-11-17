#pragma once

#include <stdbool.h>
#include "esp_err.h"

typedef struct {
    int max_db;     /* upper limit, e.g. -12 */
    int min_db;     /* silence, e.g. -60 */
    int step_db;    /* e.g. 2 */
    int startup_db; /* e.g. -30 */
} volume_config_t;

typedef struct {
    volume_config_t cfg;
    int db;
} volume_t;

/* Requires min_db < startup_db <= max_db and step_db > 0; otherwise ESP_ERR_INVALID_ARG. */
esp_err_t volume_init(volume_t *vol, const volume_config_t *cfg);
int volume_up(volume_t *vol);
int volume_down(volume_t *vol);
int volume_get_db(const volume_t *vol);
bool volume_is_silent(const volume_t *vol);
