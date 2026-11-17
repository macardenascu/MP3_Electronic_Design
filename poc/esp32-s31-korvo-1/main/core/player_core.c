#include "player_core.h"

typedef struct {
    player_action_t items[PLAYER_MAX_ACTIONS];
    size_t count;
} action_list_t;

static void push(action_list_t *list, player_action_kind_t kind, int value)
{
    list->items[list->count].kind = kind;
    list->items[list->count].value = value;
    ++list->count;
}

esp_err_t player_init(player_core_t *player, const player_config_t *cfg)
{
    if (!player || !cfg) return ESP_ERR_INVALID_ARG;
    player_core_t next = {0};
    esp_err_t err = volume_init(&next.volume, &cfg->volume);
    if (err != ESP_OK) return err;
    next.state = PLAYER_STOPPED;
    next.track_count = cfg->track_count;
    next.index = 0;
    next.prev_restart_ms = cfg->prev_restart_ms;
    *player = next;
    return ESP_OK;
}

/* Moves to track `target`, emitting the actions required by the current state. */
static void change_track(player_core_t *p, size_t target, action_list_t *list)
{
    if (p->state == PLAYER_PLAYING) {
        push(list, ACT_AUDIO_MUTE, 0);
        push(list, ACT_CLOSE_TRACK, 0);
        push(list, ACT_OPEN_TRACK, (int)target);
        push(list, ACT_AUDIO_UNMUTE, 0);
    } else if (p->state == PLAYER_PAUSED) {
        push(list, ACT_CLOSE_TRACK, 0);
        push(list, ACT_OPEN_TRACK, (int)target);
    }
    p->index = target;
}

esp_err_t player_handle_event(player_core_t *player, const player_event_t *event,
                              player_action_t actions_out[], size_t cap, size_t *n)
{
    if (!n) return ESP_ERR_INVALID_ARG;
    *n = 0;
    if (!player || !event) return ESP_ERR_INVALID_ARG;
    if (player->state == PLAYER_SHUTDOWN) return ESP_OK;

    /* Work on a copy and commit only if the actions fit in the caller's buffer. */
    player_core_t p = *player;
    action_list_t list = {.count = 0};
    const int has_tracks = p.track_count > 0;

    switch (event->kind) {
    case EVT_PLAY_PAUSE:
        if (!has_tracks) break;
        if (p.state == PLAYER_STOPPED) {
            push(&list, ACT_OPEN_TRACK, (int)p.index);
            push(&list, ACT_AUDIO_UNMUTE, 0);
            p.state = PLAYER_PLAYING;
        } else if (p.state == PLAYER_PLAYING) {
            push(&list, ACT_AUDIO_MUTE, 0);
            p.state = PLAYER_PAUSED;
        } else {
            push(&list, ACT_AUDIO_UNMUTE, 0);
            p.state = PLAYER_PLAYING;
        }
        break;
    case EVT_NEXT:
        if (!has_tracks) break;
        change_track(&p, (p.index + 1) % p.track_count, &list);
        break;
    case EVT_PREV:
        if (!has_tracks) break;
        if (p.state != PLAYER_STOPPED && event->position_ms > p.prev_restart_ms) {
            if (p.state == PLAYER_PLAYING) push(&list, ACT_AUDIO_MUTE, 0);
            push(&list, ACT_RESTART_TRACK, 0);
            if (p.state == PLAYER_PLAYING) push(&list, ACT_AUDIO_UNMUTE, 0);
        } else {
            change_track(&p, p.index == 0 ? p.track_count - 1 : p.index - 1, &list);
        }
        break;
    case EVT_TRACK_END:
        if (!has_tracks || p.state != PLAYER_PLAYING) break;
        if (p.index + 1 < p.track_count) {
            push(&list, ACT_CLOSE_TRACK, 0);
            push(&list, ACT_OPEN_TRACK, (int)(p.index + 1));
            ++p.index;
        } else {
            push(&list, ACT_AUDIO_MUTE, 0);
            push(&list, ACT_CLOSE_TRACK, 0);
            p.state = PLAYER_STOPPED;
            p.index = 0;
        }
        break;
    case EVT_VOL_UP:
        push(&list, ACT_SET_VOLUME, volume_up(&p.volume));
        break;
    case EVT_VOL_DOWN:
        push(&list, ACT_SET_VOLUME, volume_down(&p.volume));
        break;
    case EVT_LOW_BATTERY:
        if (p.state == PLAYER_PLAYING) push(&list, ACT_AUDIO_MUTE, 0);
        if (p.state != PLAYER_STOPPED) push(&list, ACT_CLOSE_TRACK, 0);
        push(&list, ACT_POWER_OFF, 0);
        p.state = PLAYER_SHUTDOWN;
        break;
    default:
        return ESP_ERR_INVALID_ARG;
    }

    if (list.count > 0 && (!actions_out || cap < list.count)) return ESP_ERR_INVALID_SIZE;
    for (size_t i = 0; i < list.count; ++i) actions_out[i] = list.items[i];
    *n = list.count;
    *player = p;
    return ESP_OK;
}
