#include "korvo_audio_caps.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
    /* Rows of coeff_div[] with Ratio 64 (esp_codec_dev 1.5.11, ES8389 without MCLK, 16 bits). */
    assert(korvo_audio_rate_supported(8000));
    assert(korvo_audio_rate_supported(16000));
    assert(korvo_audio_rate_supported(44100));
    assert(korvo_audio_rate_supported(48000));

    /* No Ratio-64 row: 22050 is the observed "bubbling" case. */
    assert(!korvo_audio_rate_supported(22050));
    assert(!korvo_audio_rate_supported(11025));
    assert(!korvo_audio_rate_supported(12000));
    assert(!korvo_audio_rate_supported(24000));
    assert(!korvo_audio_rate_supported(32000));
    assert(!korvo_audio_rate_supported(0));
    assert(!korvo_audio_rate_supported(96000));

    puts("korvo_audio_caps tests passed");
    return 0;
}
