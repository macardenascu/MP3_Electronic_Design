/* Host tool: renders the player UI for fixed cases into 128x64 PBM (P4) files ui_<case>.pbm in the
 * current directory. Usage: ./ui_preview */
#include "ui_model.h"
#include <stdio.h>

static ui_fb_t s_fb;

static int write_pbm(const char *case_name, const ui_fb_t *fb)
{
    char path[64];
    snprintf(path, sizeof(path), "ui_%s.pbm", case_name);
    FILE *f = fopen(path, "wb");
    if (f == NULL) return -1;
    fprintf(f, "P4\n%d %d\n", UI_FB_WIDTH, UI_FB_HEIGHT);
    for (int y = 0; y < UI_FB_HEIGHT; ++y) {
        for (int xb = 0; xb < UI_FB_WIDTH / 8; ++xb) {
            unsigned char byte = 0; /* P4: MSB is the leftmost pixel, 1 = black */
            for (int b = 0; b < 8; ++b) {
                if (fb_get_pixel(fb, xb * 8 + b, y)) byte |= (unsigned char)(0x80u >> b);
            }
            fputc(byte, f);
        }
    }
    const int rc = fclose(f);
    printf("%s\n", path);
    return rc;
}

static int render_case(const char *case_name, const ui_input_t *in, const char *msg, uint32_t now_ms)
{
    const ui_model_config_t cfg = {.screen_off_ms = 10000};
    ui_model_t m;
    ui_model_init(&m, &cfg, 0);
    ui_fb_t *fb = &s_fb;
    ui_model_render(&m, in, 0, fb); /* starts the name scroll at t = 0 */
    if (msg) ui_model_post_message(&m, msg, now_ms);
    ui_model_render(&m, in, now_ms, fb);
    return write_pbm(case_name, fb);
}

int main(void)
{
    const ui_input_t playing = {.state = UI_STATE_PLAYING, .index = 2, .total = 6, .track_name = "Idilio.mp3",
                                .position_ms = 83000, .volume_db = -30, .vol_min_db = -60, .vol_max_db = -12};
    ui_input_t in = playing;
    int rc = 0;

    rc |= render_case("playing_short", &in, NULL, 0);
    in.track_name = "La_Mujer_que_Bota_Fuego_nombre_mas_largo.mp3";
    rc |= render_case("long_name_t0", &in, NULL, 0);
    rc |= render_case("long_name_t2000", &in, NULL, 2000);
    in = playing;
    in.state = UI_STATE_PAUSED;
    rc |= render_case("paused", &in, NULL, 0);
    in = playing;
    in.volume_db = -12;
    rc |= render_case("volume_max", &in, NULL, 0);
    in = playing;
    rc |= render_case("msg_format", &in, UI_MSG_UNSUPPORTED, 0);
    rc |= render_case("msg_battery", &in, UI_MSG_LOW_BATTERY, 0);
    const ui_input_t empty = {.state = UI_STATE_STOPPED, .total = 0, .volume_db = -30, .vol_min_db = -60,
                              .vol_max_db = -12};
    rc |= render_case("empty_catalog", &empty, UI_MSG_NO_TRACKS, 0);
    return rc ? 1 : 0;
}
