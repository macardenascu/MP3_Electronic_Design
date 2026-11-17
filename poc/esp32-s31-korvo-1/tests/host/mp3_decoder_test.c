#include "mp3_decoder.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    const uint8_t *data;
    size_t len;
    size_t pos;
} mem_stream_t;

static int mem_read(void *ctx, void *buf, size_t len)
{
    mem_stream_t *s = ctx;
    size_t n = s->len - s->pos;
    if (n > len) n = len;
    memcpy(buf, s->data + s->pos, n);
    s->pos += n;
    return (int)n;
}

static int fail_read(void *ctx, void *buf, size_t len)
{
    (void)ctx; (void)buf; (void)len;
    return -1;
}

static mp3_decoder_t s_dec;
static int16_t s_pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];
static uint8_t s_data[96 * 1024];

static void id3_header(uint8_t h[10], uint8_t flags, uint32_t size)
{
    memcpy(h, "ID3", 3);
    h[3] = 4; h[4] = 0; h[5] = flags;
    h[6] = (uint8_t)((size >> 21) & 0x7F);
    h[7] = (uint8_t)((size >> 14) & 0x7F);
    h[8] = (uint8_t)((size >> 7) & 0x7F);
    h[9] = (uint8_t)(size & 0x7F);
}

int main(void)
{
    uint8_t h[10];
    mp3_frame_info_t info;
    size_t spc;

    /* Syncsafe size: 0x00 0x00 0x02 0x01 = 2*128 + 1 = 257 body bytes. */
    const uint8_t raw[10] = {'I', 'D', '3', 3, 0, 0, 0x00, 0x00, 0x02, 0x01};
    assert(mp3_id3v2_size(raw) == 10 + 257);
    id3_header(h, 0x00, 0x0FFFFFFF);
    assert(mp3_id3v2_size(h) == 10u + 0x0FFFFFFFu);
    id3_header(h, 0x10, 1000); /* footer flag adds 10 */
    assert(mp3_id3v2_size(h) == 10 + 1000 + 10);
    id3_header(h, 0x00, 0);
    assert(mp3_id3v2_size(h) == 10);
    h[8] = 0x80; /* not syncsafe */
    assert(mp3_id3v2_size(h) == 0);
    const uint8_t not_tag[10] = {0xFF, 0xFB, 0x90, 0x64, 0, 0, 0, 0, 0, 0};
    assert(mp3_id3v2_size(not_tag) == 0);

    /* Empty file: end of stream, no frames, no error. */
    mem_stream_t s = {.data = s_data, .len = 0};
    assert(mp3_decoder_init(&s_dec, mem_read, &s) == ESP_OK);
    assert(mp3_decoder_next(&s_dec, s_pcm, &info, &spc) == ESP_OK && spc == 0);
    assert(mp3_decoder_frames(&s_dec) == 0 && mp3_decoder_skipped_bytes(&s_dec) == 0);

    /* ID3 tag only (with footer), larger than the input buffer: skipped, then clean end. */
    const uint32_t body = 20000;
    id3_header(s_data, 0x10, body);
    memset(s_data + 10, 0, body + 10);
    s = (mem_stream_t){.data = s_data, .len = 10 + body + 10};
    assert(mp3_decoder_init(&s_dec, mem_read, &s) == ESP_OK);
    assert(s_dec.id3_bytes == 10 + body + 10);
    assert(mp3_decoder_next(&s_dec, s_pcm, &info, &spc) == ESP_OK && spc == 0);
    assert(mp3_decoder_skipped_bytes(&s_dec) == 0);

    /* Truncated ID3 tag. */
    s = (mem_stream_t){.data = s_data, .len = 5000};
    assert(mp3_decoder_init(&s_dec, mem_read, &s) == ESP_ERR_INVALID_SIZE);

    /* Pseudo-random data (deterministic LCG, no sync bytes kept): not MP3. */
    uint32_t x = 12345;
    for (size_t i = 0; i < sizeof(s_data); ++i) {
        x = x * 1103515245u + 12345u;
        s_data[i] = (uint8_t)(x >> 16);
    }
    s = (mem_stream_t){.data = s_data, .len = sizeof(s_data)};
    assert(mp3_decoder_init(&s_dec, mem_read, &s) == ESP_OK);
    esp_err_t err;
    do {
        err = mp3_decoder_next(&s_dec, s_pcm, &info, &spc);
    } while (err == ESP_OK && spc > 0);
    assert(err == ESP_ERR_NOT_SUPPORTED);
    assert(mp3_decoder_frames(&s_dec) == 0);

    /* Short random data that ends before 64 KB is also rejected at end of stream. */
    s = (mem_stream_t){.data = s_data, .len = 2000};
    assert(mp3_decoder_init(&s_dec, mem_read, &s) == ESP_OK);
    do {
        err = mp3_decoder_next(&s_dec, s_pcm, &info, &spc);
    } while (err == ESP_OK && spc > 0);
    assert(err == ESP_ERR_NOT_SUPPORTED);

    /* Read errors. */
    assert(mp3_decoder_init(&s_dec, fail_read, NULL) == ESP_FAIL);
    assert(mp3_decoder_init(NULL, mem_read, &s) == ESP_ERR_INVALID_ARG);

    puts("mp3_decoder tests passed");
    return 0;
}
