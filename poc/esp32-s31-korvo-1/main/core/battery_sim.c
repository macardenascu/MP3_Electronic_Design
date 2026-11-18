#include "battery_sim.h"

void battery_sim_init(battery_sim_t *sim, const battery_sim_config_t *cfg)
{
    sim->cfg = *cfg;
    sim->mv = cfg->start_mv;
    sim->ticks = 0;
}

uint32_t battery_sim_tick(battery_sim_t *sim)
{
    const uint32_t reading = sim->mv;
    sim->mv = sim->mv > sim->cfg.step_mv ? sim->mv - sim->cfg.step_mv : 0;
    sim->ticks++;
    return reading;
}
