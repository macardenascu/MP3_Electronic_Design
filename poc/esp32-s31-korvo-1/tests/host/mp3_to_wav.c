/* Host tool: decodes an MP3 with mp3_decoder and writes a 16-bit PCM WAV.
 * Usage: ./mp3_to_wav input.mp3 output.wav */
#include "mp3_decoder.h"
#include <stdio.h>
#include <string.h>

static mp3_decoder_t s_dec;
static int16_t s_pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];

static int file_read(void *ctx, void *buf, size_t len)
{
    FILE *f = ctx;
    const size_t n = fread(buf, 1, len, f);
    if (n < len && ferror(f)) return -1;
    return (int)n;
}

static void put16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static void put32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
}

static int write_header(FILE *out, uint32_t rate, uint16_t ch, uint32_t data_bytes)
{
    uint8_t h[44];
    memcpy(h, "RIFF", 4); put32(h + 4, 36 + data_bytes); memcpy(h + 8, "WAVE", 4);
    memcpy(h + 12, "fmt ", 4); put32(h + 16, 16); put16(h + 20, 1); put16(h + 22, ch);
    put32(h + 24, rate); put32(h + 28, rate * ch * 2u); put16(h + 32, (uint16_t)(ch * 2)); put16(h + 34, 16);
    memcpy(h + 36, "data", 4); put32(h + 40, data_bytes);
    return fseek(out, 0, SEEK_SET) == 0 && fwrite(h, 1, sizeof(h), out) == sizeof(h) ? 0 : -1;
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "usage: %s input.mp3 output.wav\n", argv[0]);
        return 2;
    }
    FILE *in = fopen(argv[1], "rb");
    if (in == NULL) {
        fprintf(stderr, "cannot open %s\n", argv[1]);
        return 1;
    }
    FILE *out = fopen(argv[2], "wb");
    if (out == NULL) {
        fprintf(stderr, "cannot create %s\n", argv[2]);
        fclose(in);
        return 1;
    }

    int rc = 0;
    mp3_frame_info_t info = {0};
    uint64_t samples_per_ch = 0;
    esp_err_t err = mp3_decoder_init(&s_dec, file_read, in);
    if (err == ESP_OK && write_header(out, 0, 0, 0) != 0) err = ESP_FAIL; /* placeholder */
    while (err == ESP_OK) {
        size_t n = 0;
        err = mp3_decoder_next(&s_dec, s_pcm, &info, &n);
        if (err != ESP_OK || n == 0) break;
        const size_t count = n * info.channels;
        if (fwrite(s_pcm, sizeof(int16_t), count, out) != count) { /* host assumed little-endian */
            err = ESP_FAIL;
            break;
        }
        samples_per_ch += n;
    }
    if (err != ESP_OK) {
        fprintf(stderr, "decode failed: error 0x%x\n", (unsigned)err);
        rc = 1;
    } else {
        const uint32_t data_bytes = (uint32_t)(samples_per_ch * info.channels * 2u);
        if (write_header(out, info.sample_rate, (uint16_t)info.channels, data_bytes) != 0) rc = 1;
    }
    printf("frames=%u sample_rate=%u channels=%u samples_per_ch=%llu skipped_bytes=%llu\n",
           (unsigned)mp3_decoder_frames(&s_dec), (unsigned)info.sample_rate, (unsigned)info.channels,
           (unsigned long long)samples_per_ch, (unsigned long long)mp3_decoder_skipped_bytes(&s_dec));
    fclose(in);
    if (fclose(out) != 0) rc = 1;
    return rc;
}
