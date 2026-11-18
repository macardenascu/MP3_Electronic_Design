#include "player_app.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/* BSP espressif/esp32_s31_korvo_1 1.0.1: bsp_iot_button_create(), bsp_button_t {SET, MODE, VOLP, VOLM}
 * (ADC buttons). espressif/button 4.2.2: iot_button_register_cb(handle, event, event_args, cb, usr_data),
 * button_cb_t = void (*)(void *button_handle, void *usr_data); callbacks run in the button timer
 * context, so they only enqueue. ESP-IDF 6.1: esp_timer_create/start_periodic/stop. */
#include "audio_engine.h"
#include "battery_sim.h"
#include "bsp/esp32_s31_korvo_1.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "iot_button.h"
#include "korvo_volume.h"
#include "music_catalog.h"
#include "player_core.h"
#include "power_policy.h"
#include "storage_korvo.h"

#define PLAYER_STACK_BYTES 8192
#define PLAYER_PRIORITY 5
#define PLAYER_QUEUE_LEN 16
#define PLAYER_PATH_MAX AUDIO_ENGINE_PATH_MAX
#define SHUTDOWN_TIMEOUT_MS 2000
#define BATTERY_PERIOD_US (1000 * 1000)

typedef enum {
    APP_BTN_PLAY_PAUSE,
    APP_BTN_NEXT,
    APP_BTN_PREV,
    APP_BTN_VOL_UP,
    APP_BTN_VOL_DOWN,
    APP_BTN_BATTERY_START,
    APP_AUTO_PLAY,
    APP_LOW_BATTERY,
    APP_ENGINE_OPENED,
    APP_ENGINE_END,
    APP_ENGINE_ERROR,
} app_evt_kind_t;

typedef struct {
    app_evt_kind_t kind;
    uint32_t gen;
    esp_err_t err;
} app_evt_t;

static const char *TAG = "player_app";
static const char *const k_state_names[] = {"STOPPED", "PLAYING", "PAUSED", "SHUTDOWN"};

static QueueHandle_t s_events;
static music_catalog_t s_catalog; /* ~64 KB: static, never on a stack */
static player_core_t s_core;
static uint32_t s_opens_sent; /* gen of the latest OPEN; older engine events are stale */
static size_t s_fail_streak;
static char s_path[PLAYER_PATH_MAX];

static button_handle_t s_buttons[BSP_BUTTON_NUM];
static esp_timer_handle_t s_batt_timer;
static bool s_batt_started;
static battery_sim_t s_batt;      /* touched only by the timer callback after start */
static power_policy_t s_policy;   /* idem */

static void post(app_evt_kind_t kind, uint32_t gen, esp_err_t err)
{
    const app_evt_t evt = {.kind = kind, .gen = gen, .err = err};
    if (xQueueSend(s_events, &evt, 0) != pdTRUE) ESP_LOGW(TAG, "event queue full, dropped %d", kind);
}

/* ---- producers ---- */

static void button_cb(void *button_handle, void *usr_data)
{
    (void)button_handle;
    post((app_evt_kind_t)(intptr_t)usr_data, 0, ESP_OK);
}

static void engine_cb(audio_engine_event_t evt, uint32_t gen, esp_err_t err, void *user)
{
    (void)user;
    const app_evt_kind_t kind = evt == AUDIO_ENGINE_EVT_OPENED      ? APP_ENGINE_OPENED
                                : evt == AUDIO_ENGINE_EVT_TRACK_END ? APP_ENGINE_END
                                                                    : APP_ENGINE_ERROR;
    post(kind, gen, err);
}

static void battery_timer_cb(void *arg)
{
    (void)arg;
    const uint32_t mv = battery_sim_tick(&s_batt);
    printf("[BATT] %" PRIu32 " mV\n", mv);
    if (power_policy_feed(&s_policy, mv)) post(APP_LOW_BATTERY, 0, ESP_OK);
}

/* ---- consumer ---- */

static void log_state(void)
{
    if (s_catalog.count == 0) {
        printf("[PLAYER] %s (no tracks)\n", k_state_names[s_core.state]);
        return;
    }
    printf("[PLAYER] %s %u/%u %s\n", k_state_names[s_core.state], (unsigned)(s_core.index + 1),
           (unsigned)s_catalog.count, s_catalog.tracks[s_core.index].name);
}

/* Executes player_core actions in order. Returns false after POWER_OFF. */
static bool run_actions(const player_action_t *actions, size_t n)
{
    for (size_t i = 0; i < n; ++i) {
        esp_err_t err = ESP_OK;
        switch (actions[i].kind) {
        case ACT_OPEN_TRACK:
            snprintf(s_path, sizeof(s_path), "%s/MUSIC/%s", storage_korvo_mount_point(),
                     s_catalog.tracks[actions[i].value].name);
            s_opens_sent++;
            err = audio_engine_open(s_path);
            break;
        case ACT_AUDIO_MUTE:
            err = audio_engine_pause();
            break;
        case ACT_AUDIO_UNMUTE:
            err = audio_engine_resume();
            break;
        case ACT_CLOSE_TRACK:
            err = audio_engine_close();
            break;
        case ACT_RESTART_TRACK:
            err = audio_engine_restart();
            break;
        case ACT_SET_VOLUME:
            printf("[VOL] %d dB -> %d %%\n", actions[i].value, korvo_volume_db_to_percent(actions[i].value));
            err = audio_engine_set_volume(actions[i].value);
            break;
        case ACT_POWER_OFF:
            /* Previous MUTE/CLOSE are already queued; the engine runs commands in order. */
            err = audio_engine_shutdown(SHUTDOWN_TIMEOUT_MS);
            if (err != ESP_OK) ESP_LOGE(TAG, "engine shutdown not confirmed: %s", esp_err_to_name(err));
            /* On the Korvo there is no power switch to open: the firmware just stops here. On the
             * Rev A PCB this is where the load switch would be released. */
            printf("[POWER] orderly shutdown complete\n");
            return false;
        }
        if (err != ESP_OK) ESP_LOGE(TAG, "engine command %d failed: %s", actions[i].kind, esp_err_to_name(err));
    }
    return true;
}

static bool feed_core(player_event_kind_t kind)
{
    const player_event_t evt = {
        .kind = kind,
        .position_ms = kind == EVT_PREV ? audio_engine_position_ms() : 0,
    };
    player_action_t actions[PLAYER_MAX_ACTIONS];
    size_t n = 0;
    const esp_err_t err = player_handle_event(&s_core, &evt, actions, PLAYER_MAX_ACTIONS, &n);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "player_handle_event(%d): %s", kind, esp_err_to_name(err));
        return true;
    }
    const bool keep_running = run_actions(actions, n);
    if (keep_running) log_state();
    return keep_running;
}

static void start_battery(void)
{
    if (s_batt_started) {
        printf("[BATT] simulation already running\n");
        return;
    }
    const battery_sim_config_t bcfg = {.start_mv = 3700, .step_mv = 20};
    const power_policy_config_t pcfg = {.enabled = true, .cutoff_mv = 3500, .hysteresis_mv = 100,
                                        .samples_required = 5};
    battery_sim_init(&s_batt, &bcfg);
    power_policy_init(&s_policy, &pcfg);
    const esp_err_t err = esp_timer_start_periodic(s_batt_timer, BATTERY_PERIOD_US);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "battery timer start failed: %s", esp_err_to_name(err));
        return;
    }
    s_batt_started = true;
    printf("[BATT] simulation started: 3700 mV, -20 mV/s, cutoff 3500 mV x5\n");
}

/* Returns false when the player must stop processing events. */
static bool handle(const app_evt_t *evt)
{
    switch (evt->kind) {
    case APP_BTN_PLAY_PAUSE:
        printf("[BTN] SET short -> PLAY_PAUSE\n");
        return feed_core(EVT_PLAY_PAUSE);
    case APP_AUTO_PLAY:
        printf("[PLAYER] auto start\n");
        return feed_core(EVT_PLAY_PAUSE);
    case APP_BTN_BATTERY_START:
        printf("[BTN] SET long -> battery simulation\n");
        start_battery();
        return true;
    case APP_BTN_NEXT:
        printf("[BTN] MODE short -> NEXT\n");
        return feed_core(EVT_NEXT);
    case APP_BTN_PREV:
        printf("[BTN] MODE long -> PREV (position %" PRIu32 " ms)\n", audio_engine_position_ms());
        return feed_core(EVT_PREV);
    case APP_BTN_VOL_UP:
        printf("[BTN] VOL+ -> VOL_UP\n");
        return feed_core(EVT_VOL_UP);
    case APP_BTN_VOL_DOWN:
        printf("[BTN] VOL- -> VOL_DOWN\n");
        return feed_core(EVT_VOL_DOWN);
    case APP_LOW_BATTERY:
        esp_timer_stop(s_batt_timer);
        printf("[BATT] LOW_BATTERY\n");
        return feed_core(EVT_LOW_BATTERY);
    case APP_ENGINE_OPENED:
        if (evt->gen == s_opens_sent) s_fail_streak = 0;
        return true;
    case APP_ENGINE_END:
        if (evt->gen != s_opens_sent) return true; /* from a track already left */
        printf("[PLAYER] track end\n");
        return feed_core(EVT_TRACK_END);
    case APP_ENGINE_ERROR: {
        if (evt->gen != s_opens_sent) return true;
        printf("[PLAYER] track error: %s\n", esp_err_to_name(evt->err));
        s_fail_streak++;
        bool keep_running = feed_core(EVT_TRACK_END); /* skip it like a finished track */
        if (keep_running && s_fail_streak >= s_catalog.count) {
            printf("[PLAYER] no playable tracks\n");
            s_fail_streak = 0;
            /* Only pause if still PLAYING; from STOPPED, PLAY_PAUSE would start the loop again. */
            if (s_core.state == PLAYER_PLAYING) keep_running = feed_core(EVT_PLAY_PAUSE);
        }
        return keep_running;
    }
    }
    return true;
}

static void player_task(void *arg)
{
    (void)arg;
    app_evt_t evt;
    for (;;) {
        if (xQueueReceive(s_events, &evt, portMAX_DELAY) != pdTRUE) continue;
        if (!handle(&evt)) break;
    }
    /* Orderly shutdown done: stop producers and the task. */
    esp_timer_stop(s_batt_timer);
    for (int i = 0; i < BSP_BUTTON_NUM; ++i) {
        if (s_buttons[i]) iot_button_delete(s_buttons[i]);
    }
    vTaskDelete(NULL);
}

static esp_err_t register_buttons(void)
{
    int count = 0;
    esp_err_t err = bsp_iot_button_create(s_buttons, &count, BSP_BUTTON_NUM);
    if (err != ESP_OK) return err;
    static const struct {
        bsp_button_t button;
        button_event_t event;
        app_evt_kind_t kind;
    } k_map[] = {
        {BSP_BUTTON_SET, BUTTON_SINGLE_CLICK, APP_BTN_PLAY_PAUSE},
        {BSP_BUTTON_SET, BUTTON_LONG_PRESS_START, APP_BTN_BATTERY_START},
        {BSP_BUTTON_MODE, BUTTON_SINGLE_CLICK, APP_BTN_NEXT},
        {BSP_BUTTON_MODE, BUTTON_LONG_PRESS_START, APP_BTN_PREV},
        {BSP_BUTTON_VOLP, BUTTON_PRESS_DOWN, APP_BTN_VOL_UP},
        {BSP_BUTTON_VOLM, BUTTON_PRESS_DOWN, APP_BTN_VOL_DOWN},
    };
    for (size_t i = 0; i < sizeof(k_map) / sizeof(k_map[0]); ++i) {
        err = iot_button_register_cb(s_buttons[k_map[i].button], k_map[i].event, NULL, button_cb,
                                     (void *)(intptr_t)k_map[i].kind);
        if (err != ESP_OK) return err;
    }
    printf("[BTN] %d buttons ready: SET=play/pause (long: battery sim), MODE=next (long: prev), VOL+/VOL-\n",
           count);
    return ESP_OK;
}

void player_app_run(void)
{
    printf("[PLAYER] Starting (E5/E7)...\n");
    esp_err_t err = storage_korvo_mount();
    if (err != ESP_OK) {
        printf("[SD] Mount failed: %s\n", esp_err_to_name(err));
        return;
    }
    err = music_catalog_scan(storage_korvo_mount_point(), &s_catalog);
    if (err != ESP_OK) {
        printf("[MUSIC] Scan failed: %s\n", esp_err_to_name(err));
        return;
    }
    printf("[MUSIC] Tracks: %u\n", (unsigned)s_catalog.count);
    for (size_t i = 0; i < s_catalog.count; ++i) {
        printf("[TRACK %03u] %s\n", (unsigned)(i + 1), s_catalog.tracks[i].name);
    }

    const player_config_t cfg = {
        .track_count = s_catalog.count,
        .prev_restart_ms = 3000,
        .volume = {.max_db = -12, .min_db = -60, .step_db = 2, .startup_db = -30},
    };
    err = player_init(&s_core, &cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "player_init: %s", esp_err_to_name(err));
        return;
    }

    s_events = xQueueCreate(PLAYER_QUEUE_LEN, sizeof(app_evt_t));
    if (s_events == NULL) {
        ESP_LOGE(TAG, "event queue: no memory");
        return;
    }
    const esp_timer_create_args_t targs = {.callback = battery_timer_cb, .name = "batt_sim"};
    err = esp_timer_create(&targs, &s_batt_timer);
    if (err == ESP_OK) err = audio_engine_init(engine_cb, NULL);
    if (err == ESP_OK) err = register_buttons();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "player start failed: %s", esp_err_to_name(err));
        return;
    }

    const int db = volume_get_db(&s_core.volume);
    printf("[VOL] %d dB -> %d %%\n", db, korvo_volume_db_to_percent(db));
    audio_engine_set_volume(db);

    if (xTaskCreate(player_task, "player", PLAYER_STACK_BYTES, NULL, PLAYER_PRIORITY, NULL) != pdPASS) {
        ESP_LOGE(TAG, "player task: no memory");
        return;
    }
    if (s_catalog.count == 0) {
        printf("[PLAYER] no tracks in MUSIC; waiting\n");
    }
    post(APP_AUTO_PLAY, 0, ESP_OK); /* start track 1 */
}
