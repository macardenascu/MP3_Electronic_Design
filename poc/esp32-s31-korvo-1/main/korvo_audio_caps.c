#include "korvo_audio_caps.h"

#include <stddef.h>

/* Source: managed_components/espressif__esp_codec_dev 1.5.11, device/es8389/es8389.c.
 * The BSP (espressif/esp32_s31_korvo_1 1.0.1, bsp_audio_codec_speaker_init) leaves use_mclk and
 * no_dac_ref false and MCLK unconnected, so es8389_set_fs() calls es8389_config_sample(), which uses
 *   mclk = fs * bits * 4 = fs * 16 * 4,  ratio = mclk / fs = 64
 * and get_coeff() looks for a coeff_div[] row with Ratio == 64 and MCLK == fs * 64.
 * Only these rows exist: 512000/8000, 1024000/16000, 2822400/44100, 3072000/48000.
 * For any other rate it logs an error and returns ESP_CODEC_DEV_NOT_SUPPORT, but es8389_set_fs()
 * ignores that result, so the codec keeps stale clock dividers (audible as "bubbling" at 22050 Hz). */
static const uint32_t k_supported_hz[] = {8000, 16000, 44100, 48000};

bool korvo_audio_rate_supported(uint32_t hz)
{
    for (size_t i = 0; i < sizeof(k_supported_hz) / sizeof(k_supported_hz[0]); ++i) {
        if (k_supported_hz[i] == hz) return true;
    }
    return false;
}
