#include "wav_parser.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* In-memory stream; max_chunk forces the parser to cope with partial reads. */
typedef struct {
    const uint8_t *data;
    size_t len;
    size_t pos;
    size_t max_chunk;
} mem_stream_t;

static int mem_read(void *ctx, void *buf, size_t len)
{
    mem_stream_t *s = ctx;
    size_t n = s->len - s->pos;
    if (n > len) n = len;
    if (s->max_chunk && n > s->max_chunk) n = s->max_chunk;
    memcpy(buf, s->data + s->pos, n);
    s->pos += n;
    return (int)n;
}

static int fail_read(void *ctx, void *buf, size_t len)
{
    (void)ctx; (void)buf; (void)len;
    return -1;
}

typedef struct {
    uint8_t b[512];
    size_t n;
} wav_buf_t;

static void put(wav_buf_t *w, const void *p, size_t n) { memcpy(w->b + w->n, p, n); w->n += n; }
static void put16(wav_buf_t *w, uint16_t v) { uint8_t p[2] = {(uint8_t)v, (uint8_t)(v >> 8)}; put(w, p, 2); }
static void put32(wav_buf_t *w, uint32_t v)
{
    uint8_t p[4] = {(uint8_t)v, (uint8_t)(v >> 8), (uint8_t)(v >> 16), (uint8_t)(v >> 24)};
    put(w, p, 4);
}

static void riff(wav_buf_t *w) { put(w, "RIFF", 4); put32(w, 0); put(w, "WAVE", 4); }

static void fmt(wav_buf_t *w, uint16_t format, uint16_t ch, uint32_t rate, uint16_t bits)
{
    put(w, "fmt ", 4);
    put32(w, 16);
    put16(w, format);
    put16(w, ch);
    put32(w, rate);
    put32(w, rate * ch * bits / 8);
    put16(w, (uint16_t)(ch * bits / 8));
    put16(w, bits);
}

static void data(wav_buf_t *w, uint32_t size)
{
    put(w, "data", 4);
    put32(w, size);
    for (uint32_t i = 0; i < size; ++i) w->b[w->n++] = (uint8_t)i;
}

static esp_err_t parse(const wav_buf_t *w, size_t max_chunk, wav_info_t *info)
{
    mem_stream_t s = {.data = w->b, .len = w->n, .pos = 0, .max_chunk = max_chunk};
    const esp_err_t err = wav_parse_header(mem_read, &s, info);
    if (err == ESP_OK) assert(s.pos == info->data_offset); /* stream left at first sample */
    return err;
}

int main(void)
{
    wav_info_t info;

    /* Stereo 44.1 kHz, valid; also with 1-byte reads. */
    wav_buf_t w = {0};
    riff(&w); fmt(&w, 1, 2, 44100, 16); data(&w, 16);
    for (size_t chunk = 0; chunk <= 1; ++chunk) {
        assert(parse(&w, chunk, &info) == ESP_OK);
        assert(info.sample_rate == 44100 && info.channels == 2 && info.bits_per_sample == 16);
        assert(info.data_size == 16 && info.data_offset == 44);
    }

    /* Mono 16 kHz, valid. */
    w = (wav_buf_t){0};
    riff(&w); fmt(&w, 1, 1, 16000, 16); data(&w, 8);
    assert(parse(&w, 0, &info) == ESP_OK);
    assert(info.sample_rate == 16000 && info.channels == 1 && info.data_offset == 44);

    /* LIST chunk between fmt and data is skipped. */
    w = (wav_buf_t){0};
    riff(&w); fmt(&w, 1, 2, 48000, 16);
    put(&w, "LIST", 4); put32(&w, 26); put(&w, "INFOISFT\x0e\0\0\0Lavf58.76.100\0", 26);
    data(&w, 4);
    assert(parse(&w, 0, &info) == ESP_OK);
    assert(info.data_offset == 44 + 8 + 26 && info.data_size == 4);

    /* Odd-sized chunk before data: the pad byte must be skipped. */
    w = (wav_buf_t){0};
    riff(&w); put(&w, "junk", 4); put32(&w, 3); put(&w, "abc", 3); w.b[w.n++] = 0; /* pad */
    fmt(&w, 1, 1, 8000, 16); data(&w, 2);
    assert(parse(&w, 0, &info) == ESP_OK);
    assert(info.sample_rate == 8000 && info.data_offset == 12 + 8 + 4 + 24 + 8);

    /* fmt chunk larger than 16 bytes (cbSize present) is accepted. */
    w = (wav_buf_t){0};
    riff(&w); put(&w, "fmt ", 4); put32(&w, 18);
    put16(&w, 1); put16(&w, 2); put32(&w, 22050); put32(&w, 88200); put16(&w, 4); put16(&w, 16); put16(&w, 0);
    data(&w, 4);
    assert(parse(&w, 0, &info) == ESP_OK);
    assert(info.sample_rate == 22050 && info.data_offset == 12 + 8 + 18 + 8);

    /* Unsupported formats. */
    w = (wav_buf_t){0};
    riff(&w); fmt(&w, 1, 2, 44100, 24); data(&w, 6);
    assert(parse(&w, 0, &info) == ESP_ERR_NOT_SUPPORTED);
    w = (wav_buf_t){0};
    riff(&w); fmt(&w, 3, 2, 44100, 32); data(&w, 8);
    assert(parse(&w, 0, &info) == ESP_ERR_NOT_SUPPORTED);
    w = (wav_buf_t){0};
    riff(&w); fmt(&w, 1, 6, 44100, 16); data(&w, 12);
    assert(parse(&w, 0, &info) == ESP_ERR_NOT_SUPPORTED);
    w = (wav_buf_t){0};
    riff(&w); fmt(&w, 1, 2, 96000, 16); data(&w, 4);
    assert(parse(&w, 0, &info) == ESP_ERR_NOT_SUPPORTED);

    /* Truncated: inside the RIFF header, inside fmt, inside a chunk header, inside a skipped chunk. */
    w = (wav_buf_t){0};
    riff(&w); fmt(&w, 1, 2, 44100, 16); data(&w, 0);
    const size_t cuts[] = {6, 30, 40};
    for (size_t i = 0; i < sizeof(cuts) / sizeof(cuts[0]); ++i) {
        wav_buf_t t = w;
        t.n = cuts[i];
        assert(parse(&t, 0, &info) == ESP_ERR_INVALID_SIZE);
    }
    w = (wav_buf_t){0};
    riff(&w); put(&w, "LIST", 4); put32(&w, 100); put(&w, "INFO", 4);
    assert(parse(&w, 0, &info) == ESP_ERR_INVALID_SIZE);

    /* No data chunk, and data before fmt. */
    w = (wav_buf_t){0};
    riff(&w); fmt(&w, 1, 2, 44100, 16);
    assert(parse(&w, 0, &info) == ESP_ERR_NOT_FOUND);
    w = (wav_buf_t){0};
    riff(&w); data(&w, 4); fmt(&w, 1, 2, 44100, 16);
    assert(parse(&w, 0, &info) == ESP_ERR_NOT_FOUND);

    /* Not RIFF/WAVE. */
    w = (wav_buf_t){0};
    put(&w, "RIFX", 4); put32(&w, 0); put(&w, "WAVE", 4); fmt(&w, 1, 2, 44100, 16); data(&w, 4);
    assert(parse(&w, 0, &info) == ESP_ERR_INVALID_ARG);
    w = (wav_buf_t){0};
    put(&w, "RIFF", 4); put32(&w, 0); put(&w, "AVI ", 4);
    assert(parse(&w, 0, &info) == ESP_ERR_INVALID_ARG);
    w = (wav_buf_t){0};
    put(&w, "ID3\x04", 4);
    assert(parse(&w, 0, &info) == ESP_ERR_INVALID_ARG);

    /* Bad arguments and read errors. */
    assert(wav_parse_header(NULL, NULL, &info) == ESP_ERR_INVALID_ARG);
    assert(wav_parse_header(mem_read, NULL, NULL) == ESP_ERR_INVALID_ARG);
    assert(wav_parse_header(fail_read, NULL, &info) == ESP_FAIL);

    puts("wav_parser tests passed");
    return 0;
}
