#pragma once

/* Maps a player volume in dB to the esp_codec_dev output percentage on the Korvo.
 * Linear in dB: -60 dB -> 0 %, -12 dB -> 60 %, rounded to nearest (halves away from zero);
 * values outside [-60, -12] saturate. 60 % is the ceiling validated by ear in E2.2. */
int korvo_volume_db_to_percent(int db);
