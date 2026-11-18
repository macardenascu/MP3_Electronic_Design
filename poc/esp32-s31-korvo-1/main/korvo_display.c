#include "korvo_display.h"

#include <stdio.h>
#include <string.h>

/* BSP espressif/esp32_s31_korvo_1 1.0.1 (bsp/display.h): bsp_display_new() creates the 800x480 RGB565
 * parallel RGB panel (esp_lcd_new_rgb_panel, 1 framebuffer in PSRAM, needs CONFIG_SPIRAM) and only
 * resets/initialises it; the display must be enabled with esp_lcd_panel_disp_on_off(). The backlight
 * is not controllable on this board (BSP_LCD_BACKLIGHT = GPIO_NUM_NC; brightness_set returns
 * ESP_ERR_NOT_SUPPORTED). ESP-IDF 6.1 esp_lcd_panel_draw_bitmap(): x_end/y_end exclusive; on an RGB
 * panel it copies color_data into the driver's framebuffer, so color_data can be reused right after. */
#include "bsp/esp32_s31_korvo_1.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"

#define OLED_SCALE 3
#define OLED_W (UI_FB_WIDTH * OLED_SCALE)  /* 384 */
#define OLED_H (UI_FB_HEIGHT * OLED_SCALE) /* 192 */
#define OLED_X ((BSP_LCD_H_RES - OLED_W) / 2)
#define OLED_Y ((BSP_LCD_V_RES - OLED_H) / 2)
#define FRAME_PX 2

/* One strip buffer (internal RAM) shared by every draw: 800 x 16 px = 25.6 KB. A virtual-OLED strip
 * is 384 x 24 px (8 framebuffer rows scaled x3) and a 2x text line is up to 800 x 16 px. */
#define STRIP_W BSP_LCD_H_RES
#define STRIP_ROWS 16
#define OLED_STRIP_ROWS (8 * OLED_SCALE) /* 24 */
#define STRIP_PX (STRIP_W * STRIP_ROWS)

_Static_assert(OLED_W * OLED_STRIP_ROWS <= STRIP_PX, "OLED strip must fit in the strip buffer");

static const char *TAG = "korvo_display";

static esp_lcd_panel_handle_t s_panel;
static uint16_t s_strip[STRIP_PX];
static ui_fb_t s_shown;   /* copy of what the virtual OLED shows (black when off) */
static bool s_shown_valid;
static const ui_fb_t s_black; /* all pixels off */

static esp_err_t fill_rect(int x, int y, int w, int h, uint16_t color)
{
    if (w <= 0 || h <= 0) return ESP_OK;
    const int rows_per_strip = STRIP_PX / w;
    const int fill = (h < rows_per_strip ? h : rows_per_strip) * w;
    for (int i = 0; i < fill; ++i) s_strip[i] = color;
    for (int row = 0; row < h; row += rows_per_strip) {
        const int n = (h - row) < rows_per_strip ? (h - row) : rows_per_strip;
        const esp_err_t err = esp_lcd_panel_draw_bitmap(s_panel, x, y + row, x + w, y + row + n, s_strip);
        if (err != ESP_OK) return err;
    }
    return ESP_OK;
}

esp_err_t korvo_display_init(void)
{
    const bsp_display_config_t cfg = {0};
    esp_lcd_panel_io_handle_t io = NULL; /* NULL for an RGB panel */
    esp_err_t err = bsp_display_new(&cfg, &s_panel, &io);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "bsp_display_new failed: %s (is CONFIG_SPIRAM enabled?)", esp_err_to_name(err));
        return err;
    }
    err = esp_lcd_panel_disp_on_off(s_panel, true);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_lcd_panel_disp_on_off failed: %s", esp_err_to_name(err));
        return err;
    }
    /* Background, gray module frame (2 px) and a black OLED area. */
    err = fill_rect(0, 0, BSP_LCD_H_RES, BSP_LCD_V_RES, KORVO_SCREEN_BG_COLOR);
    if (err == ESP_OK) {
        err = fill_rect(OLED_X - FRAME_PX, OLED_Y - FRAME_PX, OLED_W + 2 * FRAME_PX, OLED_H + 2 * FRAME_PX,
                        KORVO_FRAME_COLOR);
    }
    if (err == ESP_OK) err = fill_rect(OLED_X, OLED_Y, OLED_W, OLED_H, KORVO_OLED_OFF_COLOR);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "initial draw failed: %s", esp_err_to_name(err));
        return err;
    }
    s_shown = s_black;
    s_shown_valid = true;
    printf("[UI] LCD %dx%d ready, virtual OLED %dx%d at (%d,%d)\n", BSP_LCD_H_RES, BSP_LCD_V_RES, OLED_W,
           OLED_H, OLED_X, OLED_Y);
    return ESP_OK;
}

esp_err_t korvo_display_show(const ui_fb_t *fb, bool on)
{
    if (s_panel == NULL) return ESP_ERR_INVALID_STATE;
    const ui_fb_t *src = on ? fb : &s_black;
    if (s_shown_valid && memcmp(src->buf, s_shown.buf, UI_FB_BYTES) == 0) return ESP_OK; /* unchanged */

    /* One strip per framebuffer page (8 rows -> 24 LCD rows); unchanged pages are skipped. */
    for (int page = 0; page < UI_FB_HEIGHT / 8; ++page) {
        const uint8_t *row_bytes = &src->buf[page * UI_FB_WIDTH];
        if (s_shown_valid && memcmp(row_bytes, &s_shown.buf[page * UI_FB_WIDTH], UI_FB_WIDTH) == 0) continue;
        for (int bit = 0; bit < 8; ++bit) {
            uint16_t *line = &s_strip[bit * OLED_SCALE * OLED_W];
            for (int x = 0; x < UI_FB_WIDTH; ++x) {
                const uint16_t c = (row_bytes[x] >> bit) & 1u ? KORVO_OLED_ON_COLOR : KORVO_OLED_OFF_COLOR;
                for (int s = 0; s < OLED_SCALE; ++s) line[x * OLED_SCALE + s] = c;
            }
            for (int s = 1; s < OLED_SCALE; ++s) memcpy(line + s * OLED_W, line, OLED_W * sizeof(uint16_t));
        }
        const int y = OLED_Y + page * OLED_STRIP_ROWS;
        const esp_err_t err =
            esp_lcd_panel_draw_bitmap(s_panel, OLED_X, y, OLED_X + OLED_W, y + OLED_STRIP_ROWS, s_strip);
        if (err != ESP_OK) {
            s_shown_valid = false; /* force a full redraw next time */
            return err;
        }
    }
    s_shown = *src;
    s_shown_valid = true;
    return ESP_OK;
}

esp_err_t korvo_display_text2x(int x, int y, const char *text, int cols, uint16_t fg, uint16_t bg)
{
    if (s_panel == NULL) return ESP_ERR_INVALID_STATE;
    const int max_cols = (BSP_LCD_H_RES - x) / (UI_FONT_W * 2);
    if (cols > max_cols) cols = max_cols;
    if (cols <= 0) return ESP_OK;
    const int w = cols * UI_FONT_W * 2;
    const size_t len = text ? strlen(text) : 0;
    for (int r = 0; r < UI_FONT_H; ++r) {
        uint16_t *line = &s_strip[(2 * r) * w];
        for (int col = 0; col < cols; ++col) {
            const uint8_t bits = (size_t)col < len ? ui_font_glyph(text[col])[r] : 0;
            for (int c = 0; c < UI_FONT_W; ++c) {
                const uint16_t px = (bits >> c) & 1u ? fg : bg;
                line[(col * UI_FONT_W + c) * 2] = px;
                line[(col * UI_FONT_W + c) * 2 + 1] = px;
            }
        }
        memcpy(line + w, line, (size_t)w * sizeof(uint16_t));
    }
    return esp_lcd_panel_draw_bitmap(s_panel, x, y, x + w, y + UI_FONT_H * 2, s_strip);
}
