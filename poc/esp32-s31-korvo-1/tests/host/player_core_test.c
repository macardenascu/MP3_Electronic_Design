#include "player_core.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define OPEN(i) {ACT_OPEN_TRACK, (i)}
#define RESTART {ACT_RESTART_TRACK, 0}
#define CLOSE {ACT_CLOSE_TRACK, 0}
#define UNMUTE {ACT_AUDIO_UNMUTE, 0}
#define MUTE {ACT_AUDIO_MUTE, 0}
#define VOL(db) {ACT_SET_VOLUME, (db)}
#define OFF {ACT_POWER_OFF, 0}

static player_core_t make(size_t tracks)
{
    const player_config_t cfg = {
        .track_count = tracks,
        .prev_restart_ms = 3000,
        .volume = {.max_db = -12, .min_db = -60, .step_db = 2, .startup_db = -30},
    };
    player_core_t p;
    assert(player_init(&p, &cfg) == ESP_OK);
    assert(p.state == PLAYER_STOPPED && p.index == 0);
    return p;
}

/* Sends an event and checks the exact ordered action list. */
static void expect(player_core_t *p, player_event_kind_t kind, uint32_t pos,
                   const player_action_t *want, size_t want_n)
{
    player_action_t out[8];
    size_t n = 99;
    const player_event_t ev = {.kind = kind, .position_ms = pos};
    assert(player_handle_event(p, &ev, out, 8, &n) == ESP_OK);
    assert(n == want_n);
    for (size_t i = 0; i < n; ++i) {
        assert(out[i].kind == want[i].kind);
        assert(out[i].value == want[i].value);
    }
}

#define EXPECT(p, kind, pos, ...)                                                            \
    do {                                                                                     \
        const player_action_t want_[] = {__VA_ARGS__};                                       \
        expect((p), (kind), (pos), want_, sizeof(want_) / sizeof(want_[0]));                 \
    } while (0)
#define EXPECT_NONE(p, kind, pos) expect((p), (kind), (pos), NULL, 0)

static void play_pause(void)
{
    player_core_t p = make(3);
    EXPECT(&p, EVT_PLAY_PAUSE, 0, OPEN(0), UNMUTE);
    assert(p.state == PLAYER_PLAYING);
    EXPECT(&p, EVT_PLAY_PAUSE, 0, MUTE);
    assert(p.state == PLAYER_PAUSED);
    EXPECT(&p, EVT_PLAY_PAUSE, 0, UNMUTE);
    assert(p.state == PLAYER_PLAYING);
}

static void next_rules(void)
{
    player_core_t p = make(3);
    EXPECT_NONE(&p, EVT_NEXT, 0); /* STOPPED: index only */
    assert(p.index == 1 && p.state == PLAYER_STOPPED);
    EXPECT(&p, EVT_PLAY_PAUSE, 0, OPEN(1), UNMUTE);
    EXPECT(&p, EVT_NEXT, 0, MUTE, CLOSE, OPEN(2), UNMUTE);
    assert(p.index == 2 && p.state == PLAYER_PLAYING);
    EXPECT(&p, EVT_NEXT, 0, MUTE, CLOSE, OPEN(0), UNMUTE); /* wrap */
    assert(p.index == 0);
    EXPECT(&p, EVT_PLAY_PAUSE, 0, MUTE);
    EXPECT(&p, EVT_NEXT, 0, CLOSE, OPEN(1));
    assert(p.index == 1 && p.state == PLAYER_PAUSED);
    p.index = 2;
    EXPECT(&p, EVT_NEXT, 0, CLOSE, OPEN(0)); /* wrap while paused */
    assert(p.state == PLAYER_PAUSED);

    player_core_t s = make(3);
    s.index = 2;
    EXPECT_NONE(&s, EVT_NEXT, 0);
    assert(s.index == 0);
}

static void prev_rules(void)
{
    /* PLAYING, 5000 ms: restart. */
    player_core_t p = make(3);
    EXPECT(&p, EVT_PLAY_PAUSE, 0, OPEN(0), UNMUTE);
    EXPECT(&p, EVT_PREV, 5000, MUTE, RESTART, UNMUTE);
    assert(p.index == 0 && p.state == PLAYER_PLAYING);
    /* PLAYING, 1000 ms at track 0: wrap to last. */
    EXPECT(&p, EVT_PREV, 1000, MUTE, CLOSE, OPEN(2), UNMUTE);
    assert(p.index == 2);
    EXPECT(&p, EVT_PREV, 1000, MUTE, CLOSE, OPEN(1), UNMUTE);
    assert(p.index == 1);
    /* Exactly at threshold is not "greater than": previous track. */
    EXPECT(&p, EVT_PREV, 3000, MUTE, CLOSE, OPEN(0), UNMUTE);

    /* PAUSED. */
    EXPECT(&p, EVT_PLAY_PAUSE, 0, MUTE);
    EXPECT(&p, EVT_PREV, 5000, RESTART);
    assert(p.state == PLAYER_PAUSED && p.index == 0);
    EXPECT(&p, EVT_PREV, 1000, CLOSE, OPEN(2));
    assert(p.state == PLAYER_PAUSED && p.index == 2);

    /* STOPPED: index only, even with a large position. */
    player_core_t s = make(3);
    EXPECT_NONE(&s, EVT_PREV, 5000);
    assert(s.index == 2 && s.state == PLAYER_STOPPED);
    EXPECT_NONE(&s, EVT_PREV, 1000);
    assert(s.index == 1);
}

static void track_end_rules(void)
{
    player_core_t p = make(2);
    EXPECT(&p, EVT_PLAY_PAUSE, 0, OPEN(0), UNMUTE);
    EXPECT(&p, EVT_TRACK_END, 0, CLOSE, OPEN(1));
    assert(p.state == PLAYER_PLAYING && p.index == 1);
    EXPECT(&p, EVT_TRACK_END, 0, MUTE, CLOSE); /* last track */
    assert(p.state == PLAYER_STOPPED && p.index == 0);
    EXPECT_NONE(&p, EVT_TRACK_END, 0); /* ignored when stopped */
}

static void volume_rules(void)
{
    player_core_t p = make(1);
    EXPECT(&p, EVT_VOL_UP, 0, VOL(-28));
    EXPECT(&p, EVT_VOL_DOWN, 0, VOL(-30));
    EXPECT(&p, EVT_PLAY_PAUSE, 0, OPEN(0), UNMUTE);
    for (int i = 0; i < 50; ++i) EXPECT(&p, EVT_VOL_UP, 0, VOL(i < 8 ? -28 + 2 * i : -12));
    EXPECT(&p, EVT_PLAY_PAUSE, 0, MUTE);
    EXPECT(&p, EVT_VOL_DOWN, 0, VOL(-14));
}

static void empty_catalog(void)
{
    player_core_t p = make(0);
    EXPECT_NONE(&p, EVT_PLAY_PAUSE, 0);
    EXPECT_NONE(&p, EVT_NEXT, 0);
    EXPECT_NONE(&p, EVT_PREV, 5000);
    EXPECT_NONE(&p, EVT_TRACK_END, 0);
    assert(p.state == PLAYER_STOPPED && p.index == 0);
    EXPECT(&p, EVT_VOL_UP, 0, VOL(-28));
    EXPECT(&p, EVT_LOW_BATTERY, 0, OFF);
    assert(p.state == PLAYER_SHUTDOWN);
}

static void low_battery_and_shutdown(void)
{
    player_core_t p = make(3);
    EXPECT(&p, EVT_PLAY_PAUSE, 0, OPEN(0), UNMUTE);
    EXPECT(&p, EVT_LOW_BATTERY, 0, MUTE, CLOSE, OFF);
    assert(p.state == PLAYER_SHUTDOWN);

    p = make(3);
    EXPECT(&p, EVT_PLAY_PAUSE, 0, OPEN(0), UNMUTE);
    EXPECT(&p, EVT_PLAY_PAUSE, 0, MUTE);
    EXPECT(&p, EVT_LOW_BATTERY, 0, CLOSE, OFF);
    assert(p.state == PLAYER_SHUTDOWN);

    p = make(3);
    EXPECT(&p, EVT_LOW_BATTERY, 0, OFF);
    assert(p.state == PLAYER_SHUTDOWN);

    const player_event_kind_t all[] = {EVT_PLAY_PAUSE, EVT_NEXT, EVT_PREV, EVT_VOL_UP,
                                       EVT_VOL_DOWN, EVT_TRACK_END, EVT_LOW_BATTERY};
    for (size_t i = 0; i < sizeof(all) / sizeof(all[0]); ++i) {
        EXPECT_NONE(&p, all[i], 5000);
        assert(p.state == PLAYER_SHUTDOWN && volume_get_db(&p.volume) == -30);
    }
}

static void short_capacity(void)
{
    player_core_t p = make(3);
    player_action_t out[4];
    size_t n = 99;
    player_event_t ev = {.kind = EVT_PLAY_PAUSE};
    player_core_t before = p;
    assert(player_handle_event(&p, &ev, out, 1, &n) == ESP_ERR_INVALID_SIZE && n == 0);
    assert(memcmp(&p, &before, sizeof(p)) == 0);
    assert(player_handle_event(&p, &ev, out, 2, &n) == ESP_OK && n == 2);

    ev.kind = EVT_NEXT; /* needs 4 */
    before = p;
    n = 99;
    assert(player_handle_event(&p, &ev, out, 3, &n) == ESP_ERR_INVALID_SIZE && n == 0);
    assert(memcmp(&p, &before, sizeof(p)) == 0);

    ev.kind = EVT_VOL_UP; /* volume must not move either */
    n = 99;
    assert(player_handle_event(&p, &ev, NULL, 0, &n) == ESP_ERR_INVALID_SIZE && n == 0);
    assert(volume_get_db(&p.volume) == -30);

    ev.kind = EVT_LOW_BATTERY; /* needs 3 */
    n = 99;
    assert(player_handle_event(&p, &ev, out, 2, &n) == ESP_ERR_INVALID_SIZE && n == 0);
    assert(p.state == PLAYER_PLAYING);

    /* Events with no actions succeed even with cap 0. */
    player_core_t s = make(3);
    ev.kind = EVT_NEXT;
    assert(player_handle_event(&s, &ev, NULL, 0, &n) == ESP_OK && n == 0 && s.index == 1);
}

static void invalid_config(void)
{
    player_config_t cfg = {.track_count = 1, .prev_restart_ms = 3000,
                           .volume = {.max_db = -12, .min_db = -60, .step_db = 0, .startup_db = -30}};
    player_core_t p;
    assert(player_init(&p, &cfg) == ESP_ERR_INVALID_ARG);
}

int main(void)
{
    play_pause();
    next_rules();
    prev_rules();
    track_end_rules();
    volume_rules();
    empty_catalog();
    low_battery_and_shutdown();
    short_capacity();
    invalid_config();
    puts("player_core_test: OK");
    return 0;
}
