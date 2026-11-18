#pragma once

#include <stdint.h>
#include "esp_err.h"

/* E5: MP3 playback engine for the player. Runs in its own task and executes commands in order.
 * Starts paused: OPEN loads a track; RESUME starts feeding the codec. The paused/playing state
 * survives CLOSE/OPEN, so a track change while playing keeps playing. */

#define AUDIO_ENGINE_PATH_MAX 300

typedef enum {
    AUDIO_ENGINE_EVT_OPENED,      /* track loaded and codec ready */
    AUDIO_ENGINE_EVT_TRACK_END,   /* end of file reached */
    AUDIO_ENGINE_EVT_TRACK_ERROR, /* open, unsupported rate or decode failure; track closed */
} audio_engine_event_t;

/* gen identifies the OPEN the event belongs to (1 for the first OPEN, 2 for the second...),
 * so the listener can drop events from a track it already left. Called from the engine task. */
typedef void (*audio_engine_cb_t)(audio_engine_event_t evt, uint32_t gen, esp_err_t err, void *user);

/* Creates the codec handle and the "audio_engine" task (32 KB stack). */
esp_err_t audio_engine_init(audio_engine_cb_t cb, void *user);

/* Non-blocking enqueue (waits up to 100 ms if the queue is full). */
esp_err_t audio_engine_open(const char *path);
esp_err_t audio_engine_pause(void);
esp_err_t audio_engine_resume(void);
esp_err_t audio_engine_close(void);
esp_err_t audio_engine_restart(void);
esp_err_t audio_engine_set_volume(int db);

/* Closes file and codec, then blocks until the engine confirms (or timeout_ms elapses). */
esp_err_t audio_engine_shutdown(uint32_t timeout_ms);

/* Playback position of the current track, from samples written to the codec. */
uint32_t audio_engine_position_ms(void);

/* Diagnostics of the current track (zeros before the first OPEN). Read-only: measuring does not
 * change playback. Decode times include the SD reads done inside mp3_decoder_next(). */
typedef struct {
    uint32_t sample_rate;
    uint32_t channels;
    uint32_t bitrate_kbps; /* of the last decoded frame */
    uint32_t decode_avg_us;
    uint32_t decode_max_us;
} audio_engine_stats_t;

void audio_engine_get_stats(audio_engine_stats_t *out);
