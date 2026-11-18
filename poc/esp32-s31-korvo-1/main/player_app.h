#pragma once

/* E5/E7: MP3 player on the Korvo. Mounts the SD card, scans /sdcard/MUSIC, drives core/player_core
 * from the BSP buttons and the engine, and runs the simulated battery shutdown. Starts the "player"
 * task and returns. */
void player_app_run(void);
