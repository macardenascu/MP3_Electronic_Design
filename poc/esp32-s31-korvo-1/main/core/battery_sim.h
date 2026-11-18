#pragma once

#include <stdint.h>

/* Simulated battery for E7: each tick returns the current voltage, then lowers it by step_mv
 * (saturating at 0). Pure logic; the board decides when to tick. */
typedef struct {
    uint32_t start_mv; /* e.g. 3700 */
    uint32_t step_mv;  /* e.g. 20 */
} battery_sim_config_t;

typedef struct {
    battery_sim_config_t cfg;
    uint32_t mv;
    uint32_t ticks;
} battery_sim_t;

void battery_sim_init(battery_sim_t *sim, const battery_sim_config_t *cfg);
/* Returns the reading for this tick (start_mv on the first call). */
uint32_t battery_sim_tick(battery_sim_t *sim);
