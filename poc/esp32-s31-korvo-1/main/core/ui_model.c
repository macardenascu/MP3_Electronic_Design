#include "ui_model.h"

#include <stdio.h>
#include <string.h>

void ui_model_init(ui_model_t *model, const ui_model_config_t *cfg, uint32_t now_ms)
{
    memset(model, 0, sizeof(*model));
    model->cfg = *cfg;
    model->last_activity_ms = now_ms; /* screen on at start */
}

void ui_model_post_message(ui_model_t *model, const char *text, uint32_t now_ms)
{
    snprintf(model->msg, sizeof(model->msg), "%s", text ? text : "");
    model->msg_start_ms = now_ms;
    model->msg_active = true;
    model->last_activity_ms = now_ms;
}

bool ui_model_display_on(const ui_model_t *model, uint32_t now_ms)
{
    return (uint32_t)(now_ms - model->last_activity_ms) < model->cfg.screen_off_ms;
}

bool ui_model_on_button(ui_model_t *model, uint32_t now_ms)
{
    const bool was_off = !ui_model_display_on(model, now_ms);
    model->last_activity_ms = now_ms;
    return was_off;
}

size_t ui_scroll_offset(size_t len, uint32_t elapsed_ms)
{
    if (len <= UI_TEXT_COLS) return 0;
    const uint32_t lap_chars = (uint32_t)len + UI_SCROLL_GAP;
    const uint32_t lap_ms = UI_SCROLL_HOLD_MS + lap_chars * UI_SCROLL_STEP_MS;
    const uint32_t t = elapsed_ms % lap_ms; /* each lap starts with the hold */
    if (t < UI_SCROLL_HOLD_MS) return 0;
    return (size_t)((t - UI_SCROLL_HOLD_MS) / UI_SCROLL_STEP_MS);
}

void ui_format_time(uint32_t ms, char out[6])
{
    uint32_t s = ms / 1000u;
    uint32_t m = s / 60u;
    s %= 60u;
    if (m > 99u) {
        m = 99u;
        s = 59u;
    }
    out[0] = (char)('0' + m / 10u);
    out[1] = (char)('0' + m % 10u);
    out[2] = ':';
    out[3] = (char)('0' + s / 10u);
    out[4] = (char)('0' + s % 10u);
    out[5] = '\0';
}

int ui_volume_fill_px(int db, int min_db, int max_db)
{
    const int inner = UI_VOL_BAR_W - 2;
    if (max_db <= min_db || db <= min_db) return 0;
    if (db >= max_db) return inner;
    return ((db - min_db) * inner + (max_db - min_db) / 2) / (max_db - min_db);
}

size_t ui_name_len_no_ext(const char *name)
{
    if (name == NULL) return 0;
    const char *dot = strrchr(name, '.');
    return dot && dot != name ? (size_t)(dot - name) : strlen(name);
}

static uint32_t hash_text(const char *s, size_t len)
{
    uint32_t h = 2166136261u; /* FNV-1a */
    for (size_t i = 0; i < len; ++i) h = (h ^ (uint8_t)s[i]) * 16777619u;
    return h;
}

/* Draws len chars of text on row y: centered if they fit, otherwise a 16-column window that
 * scrolls in a loop with a blank gap, elapsed_ms after the text appeared. */
static void draw_line(ui_fb_t *fb, int y, const char *text, size_t len, uint32_t elapsed_ms)
{
    char window[UI_TEXT_COLS + 1];
    if (len <= UI_TEXT_COLS) {
        memcpy(window, text, len);
        window[len] = '\0';
        fb_draw_text(fb, (UI_FB_WIDTH - (int)len * UI_FONT_W) / 2, y, window);
        return;
    }
    const size_t lap = len + UI_SCROLL_GAP;
    const size_t off = ui_scroll_offset(len, elapsed_ms);
    for (size_t i = 0; i < UI_TEXT_COLS; ++i) {
        const size_t k = (off + i) % lap;
        window[i] = k < len ? text[k] : ' ';
    }
    window[UI_TEXT_COLS] = '\0';
    fb_draw_text(fb, 0, y, window);
}

void ui_model_render(ui_model_t *model, const ui_input_t *in, uint32_t now_ms, ui_fb_t *fb)
{
    fb_clear(fb);
    char text[24];

    /* Row 0: state icon + " 03/06" on the left, "mm:ss" on the right. */
    const ui_icon_t icon = in->state == UI_STATE_PLAYING  ? UI_ICON_PLAY
                           : in->state == UI_STATE_PAUSED ? UI_ICON_PAUSE
                                                          : UI_ICON_STOP;
    fb_draw_icon(fb, 0, UI_ROW_STATUS, icon);
    if (in->total > 0) {
        snprintf(text, sizeof(text), " %02u/%02u", (unsigned)(in->index + 1), (unsigned)in->total);
    } else {
        snprintf(text, sizeof(text), " --/--");
    }
    fb_draw_text(fb, UI_FONT_W, UI_ROW_STATUS, text);
    char time_text[6];
    ui_format_time(in->position_ms, time_text);
    fb_draw_text(fb, UI_FB_WIDTH - 5 * UI_FONT_W, UI_ROW_STATUS, time_text);

    fb_draw_hline(fb, 0, UI_ROW_RULE, UI_FB_WIDTH);

    /* Row 20: track name without extension; the scroll restarts when the track changes. */
    if (in->total > 0 && in->track_name) {
        const size_t len = ui_name_len_no_ext(in->track_name);
        const uint32_t h = hash_text(in->track_name, len);
        if (!model->name_valid || model->name_index != in->index || model->name_hash != h) {
            model->name_valid = true;
            model->name_index = in->index;
            model->name_hash = h;
            model->name_start_ms = now_ms;
        }
        draw_line(fb, UI_ROW_NAME, in->track_name, len, now_ms - model->name_start_ms);
    }

    /* Row 40: volume bar + "-12dB". */
    fb_draw_rect(fb, 0, UI_ROW_VOLUME, UI_VOL_BAR_W, UI_VOL_BAR_H);
    fb_fill_rect(fb, 1, UI_ROW_VOLUME + 1, ui_volume_fill_px(in->volume_db, in->vol_min_db, in->vol_max_db),
                 UI_VOL_BAR_H - 2, true);
    snprintf(text, sizeof(text), "%ddB", in->volume_db);
    fb_draw_text(fb, UI_VOL_BAR_W + 4, UI_ROW_VOLUME, text);

    /* Row 56: temporary message. */
    if (model->msg_active && (uint32_t)(now_ms - model->msg_start_ms) >= UI_MSG_MS) model->msg_active = false;
    if (model->msg_active) {
        draw_line(fb, UI_ROW_MESSAGE, model->msg, strlen(model->msg), now_ms - model->msg_start_ms);
    }
}
