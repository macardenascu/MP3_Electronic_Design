#include "ui_render.h"

#include <string.h>

#include "font8x8.h"

/* Own 8x8 icons, same row layout as the font (bit 0 = leftmost pixel). */
static const uint8_t k_icons[3][8] = {
    [UI_ICON_PLAY] = {0x03, 0x0F, 0x3F, 0xFF, 0xFF, 0x3F, 0x0F, 0x03},
    [UI_ICON_PAUSE] = {0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66},
    [UI_ICON_STOP] = {0x00, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x00},
};

void fb_clear(ui_fb_t *fb) { memset(fb->buf, 0, sizeof(fb->buf)); }

void fb_set_pixel(ui_fb_t *fb, int x, int y, bool on)
{
    if (x < 0 || y < 0 || x >= UI_FB_WIDTH || y >= UI_FB_HEIGHT) return;
    uint8_t *b = &fb->buf[(y / 8) * UI_FB_WIDTH + x];
    const uint8_t mask = (uint8_t)(1u << (y % 8));
    if (on) {
        *b |= mask;
    } else {
        *b &= (uint8_t)~mask;
    }
}

bool fb_get_pixel(const ui_fb_t *fb, int x, int y)
{
    if (x < 0 || y < 0 || x >= UI_FB_WIDTH || y >= UI_FB_HEIGHT) return false;
    return (fb->buf[(y / 8) * UI_FB_WIDTH + x] >> (y % 8)) & 1u;
}

const uint8_t *ui_font_glyph(char c)
{
    const unsigned char uc = (unsigned char)c;
    return font8x8_basic[uc < 128 ? uc : '?'];
}

static void draw_bitmap8(ui_fb_t *fb, int x, int y, const uint8_t rows[8])
{
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            if ((rows[r] >> c) & 1u) fb_set_pixel(fb, x + c, y + r, true);
        }
    }
}

int fb_draw_text(ui_fb_t *fb, int x, int y, const char *str)
{
    for (; str && *str; ++str, x += UI_FONT_W) {
        if (x >= UI_FB_WIDTH) {
            x += UI_FONT_W * (int)strlen(str);
            break;
        }
        if (x > -UI_FONT_W) draw_bitmap8(fb, x, y, ui_font_glyph(*str));
    }
    return x;
}

void fb_draw_hline(ui_fb_t *fb, int x, int y, int w)
{
    for (int i = 0; i < w; ++i) fb_set_pixel(fb, x + i, y, true);
}

void fb_draw_rect(ui_fb_t *fb, int x, int y, int w, int h)
{
    if (w <= 0 || h <= 0) return;
    fb_draw_hline(fb, x, y, w);
    fb_draw_hline(fb, x, y + h - 1, w);
    for (int j = 0; j < h; ++j) {
        fb_set_pixel(fb, x, y + j, true);
        fb_set_pixel(fb, x + w - 1, y + j, true);
    }
}

void fb_fill_rect(ui_fb_t *fb, int x, int y, int w, int h, bool on)
{
    for (int j = 0; j < h; ++j) {
        for (int i = 0; i < w; ++i) fb_set_pixel(fb, x + i, y + j, on);
    }
}

void fb_draw_icon(ui_fb_t *fb, int x, int y, ui_icon_t icon)
{
    if ((unsigned)icon > UI_ICON_STOP) return;
    draw_bitmap8(fb, x, y, k_icons[icon]);
}
