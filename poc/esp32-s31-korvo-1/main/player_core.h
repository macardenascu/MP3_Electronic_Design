#pragma once

/* Interface placeholder for the next playback stage; not implemented yet. */
typedef enum {
    PLAYER_STATE_DIAGNOSTIC,
    PLAYER_STATE_READY,
    PLAYER_STATE_PLAYING,
    PLAYER_STATE_PAUSED,
    PLAYER_STATE_ERROR
} player_state_t;
