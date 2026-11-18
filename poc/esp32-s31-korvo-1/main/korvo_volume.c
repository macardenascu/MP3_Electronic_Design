#include "korvo_volume.h"

#define KV_MIN_DB (-60)
#define KV_MAX_DB (-12)
#define KV_MAX_PERCENT 60

int korvo_volume_db_to_percent(int db)
{
    if (db <= KV_MIN_DB) return 0;
    if (db >= KV_MAX_DB) return KV_MAX_PERCENT;
    const int num = (db - KV_MIN_DB) * KV_MAX_PERCENT; /* > 0 here */
    const int den = KV_MAX_DB - KV_MIN_DB;
    return (num + den / 2) / den;
}
