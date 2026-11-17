#include "power_policy.h"
#include <assert.h>
#include <stdio.h>

static const power_policy_config_t CFG = {
    .enabled = true, .cutoff_mv = 3500, .hysteresis_mv = 100, .samples_required = 5};

int main(void)
{
    power_policy_t p;

    /* 4 low -> false, 5th -> true, 6th -> false (latched). */
    power_policy_init(&p, &CFG);
    for (int i = 0; i < 4; ++i) assert(!power_policy_feed(&p, 3400));
    assert(power_policy_feed(&p, 3400));
    assert(!power_policy_feed(&p, 3400));
    assert(!power_policy_feed(&p, 4200)); /* still latched */
    for (int i = 0; i < 10; ++i) assert(!power_policy_feed(&p, 3000));

    /* Readings inside [cutoff, cutoff+hyst) do not reset the counter. */
    power_policy_init(&p, &CFG);
    assert(!power_policy_feed(&p, 3499));
    assert(!power_policy_feed(&p, 3500));
    assert(!power_policy_feed(&p, 3499));
    assert(!power_policy_feed(&p, 3599));
    assert(!power_policy_feed(&p, 3499));
    assert(!power_policy_feed(&p, 3550));
    assert(!power_policy_feed(&p, 3499));
    assert(power_policy_feed(&p, 3499)); /* 5th low sample */

    /* A reading >= cutoff+hyst resets the counter. */
    power_policy_init(&p, &CFG);
    for (int i = 0; i < 4; ++i) assert(!power_policy_feed(&p, 3400));
    assert(!power_policy_feed(&p, 3600));
    for (int i = 0; i < 4; ++i) assert(!power_policy_feed(&p, 3400));
    assert(power_policy_feed(&p, 3400));

    /* Disabled never fires. */
    power_policy_config_t off = CFG;
    off.enabled = false;
    power_policy_init(&p, &off);
    for (int i = 0; i < 100; ++i) assert(!power_policy_feed(&p, 0));

    puts("power_policy_test: OK");
    return 0;
}
