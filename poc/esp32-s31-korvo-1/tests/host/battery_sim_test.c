#include "battery_sim.h"
#include "power_policy.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
    const battery_sim_config_t bcfg = {.start_mv = 3700, .step_mv = 20};
    battery_sim_t sim;
    battery_sim_init(&sim, &bcfg);
    assert(battery_sim_tick(&sim) == 3700);
    assert(battery_sim_tick(&sim) == 3680);

    /* Saturates at 0. */
    const battery_sim_config_t tiny = {.start_mv = 30, .step_mv = 20};
    battery_sim_init(&sim, &tiny);
    assert(battery_sim_tick(&sim) == 30);
    assert(battery_sim_tick(&sim) == 10);
    assert(battery_sim_tick(&sim) == 0);
    assert(battery_sim_tick(&sim) == 0);

    /* E7 chain: 3700 mV, -20 mV/tick, cutoff 3500, hysteresis 100, 5 readings. */
    const power_policy_config_t pcfg = {.enabled = true, .cutoff_mv = 3500, .hysteresis_mv = 100,
                                        .samples_required = 5};
    power_policy_t policy;
    power_policy_init(&policy, &pcfg);
    battery_sim_init(&sim, &bcfg);
    int fired_tick = -1;
    int fired_count = 0;
    uint32_t fired_mv = 0;
    for (int tick = 0; tick < 200; ++tick) {
        const uint32_t mv = battery_sim_tick(&sim);
        if (power_policy_feed(&policy, mv)) {
            fired_count++;
            if (fired_tick < 0) {
                fired_tick = tick;
                fired_mv = mv;
            }
        }
    }
    printf("battery_sim: LOW_BATTERY at tick %d (%u mV), fired %d time(s)\n", fired_tick,
           (unsigned)fired_mv, fired_count);
    /* Below 3500 from tick 11 (3480 mV); fifth low reading at tick 15 (3400 mV). */
    assert(fired_tick == 15 && fired_mv == 3400);
    assert(fired_count == 1);
    puts("battery_sim tests passed");
    return 0;
}
