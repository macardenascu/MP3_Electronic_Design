#pragma once

#include <stdbool.h>
#include <stdint.h>

/* Monochrome 128x64 framebuffer in SSD1306 page layout: buffer[page * 128 + x], pixel (x, y) is
 * bit (y % 8) of the byte, page = y / 8. 1 = pixel on. All drawing clips to the screen. */
#define UI_FB_WIDTH 128
#define UI_FB_HEIGHT 64
#define UI_FB_BYTES (UI_FB_WIDTH * UI_FB_HEIGHT / 8)
#define UI_FONT_W 8
#define UI_FONT_H 8

typedef struct {
    uint8_t buf[UI_FB_BYTES];
} ui_fb_t;

typedef enum { UI_ICON_PLAY, UI_ICON_PAUSE, UI_ICON_STOP } ui_icon_t;

void fb_clear(ui_fb_t *fb);
void fb_set_pixel(ui_fb_t *fb, int x, int y, bool on);
bool fb_get_pixel(const ui_fb_t *fb, int x, int y);
/* Draws ASCII text (non-ASCII bytes show as '?') starting at (x, y); returns the x after the text. */
int fb_draw_text(ui_fb_t *fb, int x, int y, const char *str);
void fb_draw_hline(ui_fb_t *fb, int x, int y, int w);
void fb_draw_rect(ui_fb_t *fb, int x, int y, int w, int h);
void fb_fill_rect(ui_fb_t *fb, int x, int y, int w, int h, bool on);
void fb_draw_icon(ui_fb_t *fb, int x, int y, ui_icon_t icon);

/* 8 row bytes of the glyph for c (bit 0 = leftmost pixel); '?' for non-ASCII. */
const uint8_t *ui_font_glyph(char c);
