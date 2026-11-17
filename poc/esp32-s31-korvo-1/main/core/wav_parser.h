#pragma once

#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

/* Reads up to len bytes into buf. Returns bytes read (0 at end of stream, fewer than
 * len only at end of stream) or a negative value on I/O error. */
typedef int (*wav_read_fn)(void *ctx, void *buf, size_t len);

typedef struct {
    uint32_t sample_rate;
    uint16_t channels;
    uint16_t bits_per_sample;
    uint32_t data_size;   /* size field of the "data" chunk, in bytes */
    uint32_t data_offset; /* byte offset of the first sample from the start of the stream */
} wav_info_t;

/* Parses a RIFF/WAVE header by reading sequentially from the start of the stream and
 * stops right after the "data" chunk header, so the stream is positioned at data_offset.
 * Unknown chunks (e.g. "LIST") are skipped, honouring the RIFF pad byte of odd sizes.
 * Accepts only PCM (audio_format 1), 16 bits, 1-2 channels, 8000..48000 Hz.
 * Errors: ESP_ERR_INVALID_ARG    (NULL argument, not RIFF/WAVE)
 *         ESP_ERR_NOT_SUPPORTED  (unsupported format)
 *         ESP_ERR_INVALID_SIZE   (truncated header or malformed chunk size)
 *         ESP_ERR_NOT_FOUND      (no "data" chunk, or "data" before "fmt ")
 *         ESP_FAIL               (read callback reported an error) */
esp_err_t wav_parse_header(wav_read_fn rd, void *ctx, wav_info_t *out);
