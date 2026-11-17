#pragma once

#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"
#include "volume.h"

typedef enum {
    PLAYER_STOPPED,
    PLAYER_PLAYING,
    PLAYER_PAUSED,
    PLAYER_SHUTDOWN
} player_core_state_t;

typedef enum {
    EVT_PLAY_PAUSE,
    EVT_NEXT,
    EVT_PREV,
    EVT_VOL_UP,
    EVT_VOL_DOWN,
    EVT_TRACK_END,
    EVT_LOW_BATTERY
} player_event_kind_t;

typedef struct {
    player_event_kind_t kind;
    uint32_t position_ms; /* current track position; used only by EVT_PREV */
} player_event_t;

typedef enum {
    ACT_OPEN_TRACK, /* value = track index */
    ACT_RESTART_TRACK,
    ACT_CLOSE_TRACK,
    ACT_AUDIO_UNMUTE, /* high level; the board expands it with mute_sequencer */
    ACT_AUDIO_MUTE,
    ACT_SET_VOLUME, /* value = dB */
    ACT_POWER_OFF
} player_action_kind_t;

typedef struct {
    player_action_kind_t kind;
    int value;
} player_action_t;

#define PLAYER_MAX_ACTIONS 4

typedef struct {
    size_t track_count;       /* from music_catalog */
    uint32_t prev_restart_ms; /* e.g. 3000 */
    volume_config_t volume;
} player_config_t;

typedef struct {
    player_core_state_t state;
    size_t track_count;
    size_t index;
    uint32_t prev_restart_ms;
    volume_t volume;
} player_core_t;

esp_err_t player_init(player_core_t *player, const player_config_t *cfg);
/* Writes ordered actions. Short cap: ESP_ERR_INVALID_SIZE, *n = 0 and the state is unchanged. */
esp_err_t player_handle_event(player_core_t *player, const player_event_t *event,
                              player_action_t actions_out[], size_t cap, size_t *n);
