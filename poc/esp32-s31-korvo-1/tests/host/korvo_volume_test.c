#include "korvo_volume.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
    assert(korvo_volume_db_to_percent(-60) == 0);
    /* Linear in dB: (-30 + 60) * 60 / 48 = 37.5 -> 38 (round half up). */
    assert(korvo_volume_db_to_percent(-30) == 38);
    assert(korvo_volume_db_to_percent(-12) == 60);
    assert(korvo_volume_db_to_percent(-100) == 0);
    assert(korvo_volume_db_to_percent(0) == 60);
    /* Player steps of 2 dB stay monotonic and inside [0, 60]. */
    int prev = -1;
    for (int db = -60; db <= -12; db += 2) {
        const int pct = korvo_volume_db_to_percent(db);
        assert(pct >= prev && pct >= 0 && pct <= 60);
        prev = pct;
    }
    assert(korvo_volume_db_to_percent(-58) == 3); /* 2.5 -> 3 */
    assert(korvo_volume_db_to_percent(-14) == 58); /* 57.5 -> 58 */
    puts("korvo_volume tests passed");
    return 0;
}
