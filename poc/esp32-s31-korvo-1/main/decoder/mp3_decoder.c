#define MINIMP3_ONLY_MP3
#define MINIMP3_IMPLEMENTATION
#include "minimp3.h"

#include "mp3_decoder.h"

#include <string.h>

#define MP3_NO_FRAME_LIMIT (64u * 1024u)
/* Once synced, minimp3 needs one frame (max 1441 bytes); keep well above that. */
#define MP3_REFILL_THRESHOLD (MP3_DECODER_INPUT_BYTES / 2)

uint32_t mp3_id3v2_size(const uint8_t hdr[10])
{
    if (memcmp(hdr, "ID3", 3) != 0 || hdr[3] == 0xFF || hdr[4] == 0xFF) return 0;
    for (int i = 6; i < 10; ++i) {
        if (hdr[i] & 0x80) return 0; /* syncsafe bytes never have the top bit set */
    }
    const uint32_t body = ((uint32_t)hdr[6] << 21) | ((uint32_t)hdr[7] << 14) |
                          ((uint32_t)hdr[8] << 7) | (uint32_t)hdr[9];
    const uint32_t footer = (hdr[5] & 0x10) ? 10u : 0u;
    return 10u + body + footer;
}

/* Compacts the buffer and reads until it is full or the stream ends. */
static esp_err_t refill(mp3_decoder_t *dec)
{
    if (dec->in_pos > 0) {
        memmove(dec->in, dec->in + dec->in_pos, dec->in_len - dec->in_pos);
        dec->in_len -= dec->in_pos;
        dec->in_pos = 0;
    }
    while (!dec->eof && dec->in_len < sizeof(dec->in)) {
        const int n = dec->rd(dec->ctx, dec->in + dec->in_len, sizeof(dec->in) - dec->in_len);
        if (n < 0) return ESP_FAIL;
        if (n == 0) {
            dec->eof = true;
            break;
        }
        dec->in_len += (size_t)n;
    }
    return ESP_OK;
}

static void consume(mp3_decoder_t *dec, size_t n, bool skipped)
{
    dec->in_pos += n;
    if (skipped) {
        dec->skipped_bytes += n;
        dec->bytes_since_frame += (uint32_t)n;
    }
}

esp_err_t mp3_decoder_init(mp3_decoder_t *dec, mp3_read_fn rd, void *ctx)
{
    if (dec == NULL || rd == NULL) return ESP_ERR_INVALID_ARG;
    memset(dec, 0, sizeof(*dec));
    mp3dec_init(&dec->mp3d);
    dec->rd = rd;
    dec->ctx = ctx;

    esp_err_t err = refill(dec);
    if (err != ESP_OK) return err;
    if (dec->in_len < 10) return ESP_OK; /* too short for a tag; next() decides */

    uint32_t remaining = mp3_id3v2_size(dec->in);
    if (remaining == 0) return ESP_OK;
    dec->id3_bytes = remaining;
    /* The tag may be larger than the buffer: discard it in buffer-sized steps. */
    while (remaining > 0) {
        const size_t avail = dec->in_len - dec->in_pos;
        if (avail == 0) {
            if (dec->eof) return ESP_ERR_INVALID_SIZE;
            err = refill(dec);
            if (err != ESP_OK) return err;
            continue;
        }
        const size_t step = avail < remaining ? avail : remaining;
        dec->in_pos += step;
        remaining -= (uint32_t)step;
    }
    return refill(dec);
}

esp_err_t mp3_decoder_next(mp3_decoder_t *dec, int16_t pcm[MINIMP3_MAX_SAMPLES_PER_FRAME],
                           mp3_frame_info_t *info, size_t *samples_per_ch)
{
    if (dec == NULL || pcm == NULL || info == NULL || samples_per_ch == NULL) return ESP_ERR_INVALID_ARG;
    *samples_per_ch = 0;

    for (;;) {
        if (dec->bytes_since_frame >= MP3_NO_FRAME_LIMIT) return ESP_ERR_NOT_SUPPORTED;
        if (dec->in_len - dec->in_pos < MP3_REFILL_THRESHOLD) {
            const esp_err_t err = refill(dec);
            if (err != ESP_OK) return err;
        }
        const size_t avail = dec->in_len - dec->in_pos;
        if (avail == 0) {
            /* End of stream: data that never yielded a frame is not MP3. */
            return (dec->frames == 0 && dec->skipped_bytes > 0) ? ESP_ERR_NOT_SUPPORTED : ESP_OK;
        }

        mp3dec_frame_info_t fi;
        const int samples = mp3dec_decode_frame(&dec->mp3d, dec->in + dec->in_pos, (int)avail, pcm, &fi);

        if (fi.frame_bytes == 0) {
            /* No frame in the window. At EOF the rest is garbage; otherwise drop half of a
             * full buffer and keep the tail, where a frame may start. */
            if (dec->eof) {
                consume(dec, avail, true);
            } else if (avail == sizeof(dec->in)) {
                consume(dec, avail / 2, true);
            } else {
                const esp_err_t err = refill(dec);
                if (err != ESP_OK) return err;
                if (dec->in_len - dec->in_pos == avail) consume(dec, avail, true); /* no progress */
            }
            continue;
        }
        if (samples == 0) {
            consume(dec, (size_t)fi.frame_bytes, true); /* invalid or skipped frame */
            continue;
        }

        if (!dec->format_set) {
            dec->format_set = true;
            dec->sample_rate = (uint32_t)fi.hz;
            dec->channels = (uint32_t)fi.channels;
        } else if ((uint32_t)fi.hz != dec->sample_rate || (uint32_t)fi.channels != dec->channels) {
            return ESP_ERR_INVALID_STATE;
        }
        /* frame_bytes includes the frame_offset garbage before the sync word. */
        dec->skipped_bytes += (uint64_t)fi.frame_offset;
        dec->in_pos += (size_t)fi.frame_bytes;
        dec->bytes_since_frame = 0;
        dec->frames++;

        info->sample_rate = (uint32_t)fi.hz;
        info->channels = (uint32_t)fi.channels;
        info->bitrate_kbps = (uint32_t)fi.bitrate_kbps;
        *samples_per_ch = (size_t)samples;
        return ESP_OK;
    }
}
