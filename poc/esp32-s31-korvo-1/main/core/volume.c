#include "volume.h"

#include <stddef.h>

esp_err_t volume_init(volume_t *vol, const volume_config_t *cfg)
{
    if (!vol || !cfg) return ESP_ERR_INVALID_ARG;
    if (cfg->step_db <= 0 || cfg->min_db >= cfg->startup_db || cfg->startup_db > cfg->max_db) {
        return ESP_ERR_INVALID_ARG;
    }
    vol->cfg = *cfg;
    vol->db = cfg->startup_db;
    return ESP_OK;
}

int volume_up(volume_t *vol)
{
    /* Compare before adding so the limit is never overshot. */
    if (vol->db >= vol->cfg.max_db - vol->cfg.step_db) {
        vol->db = vol->cfg.max_db;
    } else {
        vol->db += vol->cfg.step_db;
    }
    return vol->db;
}

int volume_down(volume_t *vol)
{
    if (vol->db <= vol->cfg.min_db + vol->cfg.step_db) {
        vol->db = vol->cfg.min_db;
    } else {
        vol->db -= vol->cfg.step_db;
    }
    return vol->db;
}

int volume_get_db(const volume_t *vol)
{
    return vol->db;
}

bool volume_is_silent(const volume_t *vol)
{
    return vol->db <= vol->cfg.min_db;
}
