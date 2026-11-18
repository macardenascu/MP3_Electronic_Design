#include "ui_model.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static ui_fb_t s_fb;
static ui_fb_t s_ref;

static ui_input_t base_input(const char *name)
{
    ui_input_t in = {.state = UI_STATE_PLAYING, .index = 2, .total = 6, .track_name = name,
                     .position_ms = 83000, .volume_db = -30, .vol_min_db = -60, .vol_max_db = -12};
    return in;
}

/* True if row band [y, y+8) of a and b is identical. */
static bool band_equal(const ui_fb_t *a, const ui_fb_t *b, int y)
{
    for (int yy = y; yy < y + 8; ++yy)
        for (int x = 0; x < UI_FB_WIDTH; ++x)
            if (fb_get_pixel(a, x, yy) != fb_get_pixel(b, x, yy)) return false;
    return true;
}

static bool band_empty(const ui_fb_t *fb, int y)
{
    for (int yy = y; yy < y + 8; ++yy)
        for (int x = 0; x < UI_FB_WIDTH; ++x)
            if (fb_get_pixel(fb, x, yy)) return false;
    return true;
}

static int leftmost_lit(const ui_fb_t *fb, int y)
{
    for (int x = 0; x < UI_FB_WIDTH; ++x)
        for (int yy = y; yy < y + 8; ++yy)
            if (fb_get_pixel(fb, x, yy)) return x;
    return -1;
}

int main(void)
{
    const ui_model_config_t cfg = {.screen_off_ms = 10000};
    ui_model_t m;

    /* Framebuffer layout and clipping. */
    fb_clear(&s_fb);
    fb_set_pixel(&s_fb, 5, 10, true);
    assert(s_fb.buf[1 * 128 + 5] == (1u << 2));
    fb_set_pixel(&s_fb, -1, 0, true);
    fb_set_pixel(&s_fb, 128, 0, true);
    fb_set_pixel(&s_fb, 0, 64, true);
    fb_draw_text(&s_fb, 120, 0, "ABC"); /* clipped, must not crash */
    fb_fill_rect(&s_fb, -10, -10, 300, 300, false);
    for (size_t i = 0; i < UI_FB_BYTES; ++i) assert(s_fb.buf[i] == 0);

    /* Scroll offsets for a long name (> 16 chars). */
    assert(ui_scroll_offset(30, 0) == 0);
    assert(ui_scroll_offset(30, 1000) == 0);
    assert(ui_scroll_offset(30, 1300) == 1);
    assert(ui_scroll_offset(30, 1600) == 2);
    const uint32_t lap = 1000 + (30 + UI_SCROLL_GAP) * 300; /* new lap starts with the hold */
    assert(ui_scroll_offset(30, lap) == 0 && ui_scroll_offset(30, lap + 999) == 0);
    assert(ui_scroll_offset(16, 5000) == 0); /* fits: never scrolls */

    /* Rendered: the name row changes between 1000 and 1300 ms, not between 0 and 1000. */
    const char *long_name = "Una_cancion_con_nombre_muy_largo.mp3";
    ui_input_t in = base_input(long_name);
    ui_model_init(&m, &cfg, 0);
    ui_model_render(&m, &in, 0, &s_ref);
    ui_model_render(&m, &in, 1000, &s_fb);
    assert(band_equal(&s_ref, &s_fb, UI_ROW_NAME));
    ui_model_render(&m, &in, 1300, &s_fb);
    assert(!band_equal(&s_ref, &s_fb, UI_ROW_NAME));
    ui_model_render(&m, &in, 1600, &s_ref);
    assert(!band_equal(&s_ref, &s_fb, UI_ROW_NAME));

    /* Short name centered and extension hidden: "Idilio" = 6 chars -> x = (128 - 48) / 2 = 40. */
    assert(ui_name_len_no_ext("Idilio.mp3") == 6);
    assert(ui_name_len_no_ext("sin_extension") == 13);
    in = base_input("Idilio.mp3");
    ui_model_init(&m, &cfg, 0);
    ui_model_render(&m, &in, 0, &s_fb);
    ui_fb_t expect;
    fb_clear(&expect);
    fb_draw_text(&expect, 40, UI_ROW_NAME, "Idilio");
    assert(band_equal(&expect, &s_fb, UI_ROW_NAME));
    assert(leftmost_lit(&s_fb, UI_ROW_NAME) >= 40);

    /* mm:ss. */
    char t[6];
    ui_format_time(0, t);
    assert(strcmp(t, "00:00") == 0);
    ui_format_time(83000, t);
    assert(strcmp(t, "01:23") == 0);
    ui_format_time(59999, t);
    assert(strcmp(t, "00:59") == 0);
    ui_format_time(6000000, t); /* 100 min: capped */
    assert(strcmp(t, "99:59") == 0);

    /* Volume bar at min / middle / max (inner width 82 px). */
    assert(ui_volume_fill_px(-60, -60, -12) == 0);
    assert(ui_volume_fill_px(-36, -60, -12) == 41);
    assert(ui_volume_fill_px(-12, -60, -12) == UI_VOL_BAR_W - 2);
    assert(ui_volume_fill_px(-100, -60, -12) == 0 && ui_volume_fill_px(0, -60, -12) == UI_VOL_BAR_W - 2);

    /* Message lasts 2 s. */
    in = base_input("Idilio.mp3");
    ui_model_init(&m, &cfg, 0);
    ui_model_post_message(&m, "Bateria baja", 500);
    ui_model_render(&m, &in, 500, &s_fb);
    assert(!band_empty(&s_fb, UI_ROW_MESSAGE));
    ui_model_render(&m, &in, 2499, &s_fb);
    assert(!band_empty(&s_fb, UI_ROW_MESSAGE));
    ui_model_render(&m, &in, 2500, &s_fb);
    assert(band_empty(&s_fb, UI_ROW_MESSAGE));

    /* Screen off after 10 s without buttons; first press only wakes, second acts. */
    ui_model_init(&m, &cfg, 0);
    assert(ui_model_display_on(&m, 9999));
    assert(!ui_model_display_on(&m, 10000));
    assert(ui_model_on_button(&m, 12000) == true);  /* discarded: wake only */
    assert(ui_model_display_on(&m, 12000));
    assert(ui_model_on_button(&m, 12500) == false); /* normal press */
    assert(ui_model_display_on(&m, 22499) && !ui_model_display_on(&m, 22500));

    /* A message wakes the screen. */
    ui_model_init(&m, &cfg, 0);
    assert(!ui_model_display_on(&m, 15000));
    ui_model_post_message(&m, "Apagando...", 15000);
    assert(ui_model_display_on(&m, 15000));
    assert(ui_model_on_button(&m, 15100) == false);

    /* Every player message fits in one 16-column row (no scrolling needed). */
    const char *const messages[] = {UI_MSG_UNSUPPORTED, UI_MSG_LOW_BATTERY, UI_MSG_NO_TRACKS, UI_MSG_POWER_OFF};
    for (size_t i = 0; i < sizeof(messages) / sizeof(messages[0]); ++i) {
        assert(strlen(messages[i]) <= UI_TEXT_COLS);
    }

    puts("ui_model tests passed");
    return 0;
}
