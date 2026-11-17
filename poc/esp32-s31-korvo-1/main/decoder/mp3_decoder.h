#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"
#include "minimp3.h" /* types only; the implementation lives in mp3_decoder.c */

#define MP3_DECODER_INPUT_BYTES (16 * 1024)

/* Reads up to len bytes into buf. Returns bytes read (0 at end of stream) or < 0 on I/O error. */
typedef int (*mp3_read_fn)(void *ctx, void *buf, size_t len);

typedef struct {
    uint32_t sample_rate;
    uint32_t channels;
    uint32_t bitrate_kbps;
} mp3_frame_info_t;

/* Large (~23 KB): the caller declares it static. */
typedef struct {
    mp3dec_t mp3d;
    uint8_t in[MP3_DECODER_INPUT_BYTES];
    size_t in_len; /* valid bytes in in[] */
    size_t in_pos; /* first unconsumed byte */
    mp3_read_fn rd;
    void *ctx;
    bool eof;
    bool format_set;
    uint32_t sample_rate;
    uint32_t channels;
    uint32_t frames;           /* decoded frames */
    uint64_t skipped_bytes;    /* garbage / resync bytes, excluding the ID3v2 tag */
    uint32_t id3_bytes;        /* ID3v2 tag bytes skipped by init */
    uint32_t bytes_since_frame;/* bytes consumed since the last valid frame */
} mp3_decoder_t;

/* Total ID3v2 tag size (10-byte header + syncsafe size + 10-byte footer when flagged)
 * for a 10-byte header, or 0 if hdr is not a valid ID3v2 header. */
uint32_t mp3_id3v2_size(const uint8_t hdr[10]);

/* Resets the decoder and skips a leading ID3v2 tag. ESP_FAIL on read error,
 * ESP_ERR_INVALID_SIZE if the tag is truncated, ESP_ERR_INVALID_ARG on NULL arguments. */
esp_err_t mp3_decoder_init(mp3_decoder_t *dec, mp3_read_fn rd, void *ctx);

/* Decodes the next frame into pcm (interleaved). *samples_per_ch = 0 with ESP_OK at end of stream.
 * ESP_ERR_INVALID_STATE  rate or channel count changed after the first frame
 * ESP_ERR_NOT_SUPPORTED  64 KB scanned without a valid frame, or stream ended with no frame at all
 * ESP_FAIL               read callback reported an error */
esp_err_t mp3_decoder_next(mp3_decoder_t *dec, int16_t pcm[MINIMP3_MAX_SAMPLES_PER_FRAME],
                           mp3_frame_info_t *info, size_t *samples_per_ch);

static inline uint32_t mp3_decoder_frames(const mp3_decoder_t *dec) { return dec->frames; }
static inline uint64_t mp3_decoder_skipped_bytes(const mp3_decoder_t *dec) { return dec->skipped_bytes; }
