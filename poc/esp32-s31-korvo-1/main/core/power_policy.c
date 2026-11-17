#include "power_policy.h"

void power_policy_init(power_policy_t *policy, const power_policy_config_t *cfg)
{
    policy->cfg = *cfg;
    policy->low_count = 0;
    policy->latched = false;
}

bool power_policy_feed(power_policy_t *policy, uint32_t mv)
{
    if (!policy->cfg.enabled || policy->latched) return false;
    if (mv < policy->cfg.cutoff_mv) {
        ++policy->low_count;
    } else if (mv >= policy->cfg.cutoff_mv + policy->cfg.hysteresis_mv) {
        policy->low_count = 0;
    } /* inside the hysteresis band: counter unchanged */
    if (policy->low_count == policy->cfg.samples_required) {
        policy->latched = true;
        return true;
    }
    return false;
}
