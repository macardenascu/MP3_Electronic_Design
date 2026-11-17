#include "wav_parser.h"

#include <stdbool.h>
#include <string.h>

#define WAV_FORMAT_PCM 1u
#define WAV_FMT_MIN_SIZE 16u
#define WAV_MIN_RATE 8000u
#define WAV_MAX_RATE 48000u

/* Outcome of an exact read: OK, clean end of stream (0 bytes), short read, or I/O error. */
typedef enum { RD_OK, RD_EOF, RD_SHORT, RD_ERROR } rd_result_t;

static rd_result_t read_exact(wav_read_fn rd, void *ctx, void *buf, size_t len, uint32_t *pos)
{
    size_t got = 0;
    while (got < len) {
        const int n = rd(ctx, (uint8_t *)buf + got, len - got);
        if (n < 0) return RD_ERROR;
        if (n == 0) break;
        got += (size_t)n;
    }
    *pos += (uint32_t)got;
    if (got == len) return RD_OK;
    return got == 0 ? RD_EOF : RD_SHORT;
}

static esp_err_t rd_to_err(rd_result_t r)
{
    return r == RD_ERROR ? ESP_FAIL : ESP_ERR_INVALID_SIZE;
}

/* Discards len bytes without seeking, so the callback only needs sequential reads. */
static esp_err_t skip(wav_read_fn rd, void *ctx, uint32_t len, uint32_t *pos)
{
    uint8_t scratch[64];
    while (len > 0) {
        const size_t step = len < sizeof(scratch) ? len : sizeof(scratch);
        const rd_result_t r = read_exact(rd, ctx, scratch, step, pos);
        if (r != RD_OK) return rd_to_err(r);
        len -= (uint32_t)step;
    }
    return ESP_OK;
}

static uint16_t le16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }

static uint32_t le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

esp_err_t wav_parse_header(wav_read_fn rd, void *ctx, wav_info_t *out)
{
    if (rd == NULL || out == NULL) return ESP_ERR_INVALID_ARG;
    memset(out, 0, sizeof(*out));

    uint32_t pos = 0;
    uint8_t hdr[12];
    rd_result_t r = read_exact(rd, ctx, hdr, sizeof(hdr), &pos);
    if (r == RD_ERROR) return ESP_FAIL;
    /* Too short to even hold the identifiers: decide on what was read. */
    if (r != RD_OK) {
        if (pos >= 4 && memcmp(hdr, "RIFF", 4) != 0) return ESP_ERR_INVALID_ARG;
        return ESP_ERR_INVALID_SIZE;
    }
    if (memcmp(hdr, "RIFF", 4) != 0 || memcmp(hdr + 8, "WAVE", 4) != 0) return ESP_ERR_INVALID_ARG;

    bool have_fmt = false;
    wav_info_t info = {0};
    for (;;) {
        uint8_t ch[8];
        r = read_exact(rd, ctx, ch, sizeof(ch), &pos);
        if (r == RD_EOF) return ESP_ERR_NOT_FOUND;
        if (r != RD_OK) return rd_to_err(r);
        const uint32_t size = le32(ch + 4);

        if (memcmp(ch, "data", 4) == 0) {
            if (!have_fmt) return ESP_ERR_NOT_FOUND;
            info.data_size = size;
            info.data_offset = pos;
            *out = info;
            return ESP_OK;
        }

        uint32_t to_skip = size;
        if (memcmp(ch, "fmt ", 4) == 0) {
            if (size < WAV_FMT_MIN_SIZE) return ESP_ERR_INVALID_SIZE;
            uint8_t fmt[WAV_FMT_MIN_SIZE];
            r = read_exact(rd, ctx, fmt, sizeof(fmt), &pos);
            if (r != RD_OK) return rd_to_err(r);
            const uint16_t format = le16(fmt + 0);
            info.channels = le16(fmt + 2);
            info.sample_rate = le32(fmt + 4);
            info.bits_per_sample = le16(fmt + 14);
            if (format != WAV_FORMAT_PCM || info.bits_per_sample != 16 ||
                info.channels < 1 || info.channels > 2 ||
                info.sample_rate < WAV_MIN_RATE || info.sample_rate > WAV_MAX_RATE) {
                return ESP_ERR_NOT_SUPPORTED;
            }
            have_fmt = true;
            to_skip = size - WAV_FMT_MIN_SIZE;
        }
        /* RIFF chunks are word aligned: odd sizes are followed by one pad byte. */
        const esp_err_t err = skip(rd, ctx, to_skip, &pos);
        if (err != ESP_OK) return err;
        if (size & 1u) {
            const esp_err_t pad_err = skip(rd, ctx, 1, &pos);
            if (pad_err != ESP_OK) return pad_err;
        }
    }
}
