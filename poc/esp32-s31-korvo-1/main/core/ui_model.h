#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ui_render.h"

/* Portable player UI: what to draw on the 128x64 monochrome screen and when the screen is on.
 * Pure logic driven by the caller's clock (now_ms); no allocation, no hardware. */

typedef enum { UI_STATE_STOPPED, UI_STATE_PLAYING, UI_STATE_PAUSED, UI_STATE_SHUTDOWN } ui_state_t;

typedef struct {
    ui_state_t state;
    size_t index;           /* 0-based current track */
    size_t total;           /* tracks in the catalog (0 = empty) */
    const char *track_name; /* file name; the extension is hidden */
    uint32_t position_ms;
    int volume_db;
    int vol_min_db;
    int vol_max_db;
} ui_input_t;

typedef struct {
    uint32_t screen_off_ms; /* inactivity before the screen turns off, e.g. 10000 */
} ui_model_config_t;

#define UI_MSG_MAX 32
#define UI_MSG_MS 2000u
#define UI_SCROLL_HOLD_MS 1000u
#define UI_SCROLL_STEP_MS 300u
#define UI_SCROLL_GAP 3 /* blank characters between the end and the restart of a scrolling text */
#define UI_TEXT_COLS (UI_FB_WIDTH / UI_FONT_W) /* 16 */

/* Player messages; each fits in one row (<= UI_TEXT_COLS chars, checked by ui_model_test). */
#define UI_MSG_UNSUPPORTED "No soportado"
#define UI_MSG_LOW_BATTERY "Bateria baja"
#define UI_MSG_NO_TRACKS "Sin canciones"
#define UI_MSG_POWER_OFF "Apagando..."

/* Layout (y of each row). */
#define UI_ROW_STATUS 0
#define UI_ROW_RULE 10
#define UI_ROW_NAME 20
#define UI_ROW_VOLUME 40
#define UI_ROW_MESSAGE 56
#define UI_VOL_BAR_W 84 /* 84 px bar + 4 px gap + "-12dB" (40 px) = 128 px */
#define UI_VOL_BAR_H 8

typedef struct {
    ui_model_config_t cfg;
    uint32_t last_activity_ms;
    char msg[UI_MSG_MAX];
    uint32_t msg_start_ms;
    bool msg_active;
    size_t name_index; /* track the scroll belongs to */
    uint32_t name_hash;
    uint32_t name_start_ms;
    bool name_valid;
} ui_model_t;

void ui_model_init(ui_model_t *model, const ui_model_config_t *cfg, uint32_t now_ms);

/* Shows text on the message row for UI_MSG_MS and wakes the screen. Longer than 16 chars: scrolls. */
void ui_model_post_message(ui_model_t *model, const char *text, uint32_t now_ms);

/* Registers a button press. Returns true if the screen was off: that press only wakes the screen
 * and the caller must discard its action. */
bool ui_model_on_button(ui_model_t *model, uint32_t now_ms);

/* False once screen_off_ms have passed since the last button or message. */
bool ui_model_display_on(const ui_model_t *model, uint32_t now_ms);

/* Draws the full screen for input at now_ms (the caller decides whether to show it). */
void ui_model_render(ui_model_t *model, const ui_input_t *input, uint32_t now_ms, ui_fb_t *fb);

/* Helpers exposed for tests. */
/* First character shown for a scrolling text of len chars, elapsed_ms after it appeared. */
size_t ui_scroll_offset(size_t len, uint32_t elapsed_ms);
/* Writes "mm:ss" (minutes capped at 99) into out[6]. */
void ui_format_time(uint32_t ms, char out[6]);
/* Filled width in px of the volume bar's inner area (UI_VOL_BAR_W - 2). */
int ui_volume_fill_px(int db, int min_db, int max_db);
/* Length of name without its extension (text after the last '.'). */
size_t ui_name_len_no_ext(const char *name);
