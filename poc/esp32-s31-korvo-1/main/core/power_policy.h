#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool enabled;
    uint32_t cutoff_mv;        /* e.g. 3500 */
    uint32_t hysteresis_mv;    /* e.g. 100 */
    uint32_t samples_required; /* e.g. 5 */
} power_policy_config_t;

typedef struct {
    power_policy_config_t cfg;
    uint32_t low_count;
    bool latched;
} power_policy_t;

void power_policy_init(power_policy_t *policy, const power_policy_config_t *cfg);
/* True only on the first sample that meets the cutoff; afterwards it stays latched (false). */
bool power_policy_feed(power_policy_t *policy, uint32_t mv);
