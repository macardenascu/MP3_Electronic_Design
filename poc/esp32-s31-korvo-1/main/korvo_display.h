#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "ui_render.h"

/* E6 (Korvo only): shows the portable 128x64 framebuffer on the Korvo's 800x480 RGB LCD as a
 * "virtual OLED" scaled x3, and offers 2x text for the diagnostics panel. Single-task use: call all
 * functions from the same task (the UI task). */

/* RGB565 colors of the virtual OLED: mimic a white SSD1306 module; change here for a blue or two-color module. */
#define KORVO_OLED_ON_COLOR 0xFFFFu  /* white */
#define KORVO_OLED_OFF_COLOR 0x0000u /* black */

#define KORVO_SCREEN_BG_COLOR 0x18E3u /* dark slate around the module */
#define KORVO_FRAME_COLOR 0x8410u     /* gray module frame */

esp_err_t korvo_display_init(void);

/* Redraws the virtual OLED only if fb changed. on = false paints the OLED area black (the Korvo
 * backlight is not controllable: BSP_LCD_BACKLIGHT is GPIO_NUM_NC). */
esp_err_t korvo_display_show(const ui_fb_t *fb, bool on);

/* Draws one line of text with the 8x8 font scaled x2 (16 px tall), padded with bg up to cols chars. */
esp_err_t korvo_display_text2x(int x, int y, const char *text, int cols, uint16_t fg, uint16_t bg);

/* Area outside the virtual OLED reserved for the diagnostics panel (top band of the screen). */
#define KORVO_DIAG_X 16
#define KORVO_DIAG_Y 8
#define KORVO_DIAG_LINE_H 20
