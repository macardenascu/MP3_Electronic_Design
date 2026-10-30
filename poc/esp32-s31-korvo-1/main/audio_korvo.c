#include "audio_korvo.h"

#include <math.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "bsp/esp32_s31_korvo_1.h"
#include "driver/i2s_common.h"
#include "driver/i2s_tdm.h"
#include "esp_codec_dev.h"
#include "esp_codec_dev_defaults.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "es8389_codec.h"

#define PCM_SAMPLE_RATE_HZ   16000
#define PCM_BITS_PER_SAMPLE  16
#define PCM_CHANNELS         4
#define PCM_TONE_HZ          440
#define PCM_TONE_SECONDS     3
#define PCM_CHUNK_FRAMES     160
#define PCM_RAMP_FRAMES      1600
#define PCM_PEAK             2621 /* 8% of full scale */

#define ES8389_DAC_SERIAL_FORMAT_REG 0x40
#define ES8389_MASTER_SERIAL_FORMAT_REG 0x0C
#define ES8389_DAC_SLOT_SELECT_REG   0x49
#define ES8389_DAC_SLOT_MASK         0x70
#define ES8389_DAC_SLOT_0            0x00
#define ES8389_DAC_SLOT_RAMP_DEFAULT  0x0F
#define ES8389_SERIAL_FORMAT_MASK    0xE0
#define ES8389_DSP_A                 0x80

static const char *TAG = "audio_korvo";

typedef struct {
    i2s_chan_handle_t tx;
    const audio_codec_data_if_t *data_if;
    const audio_codec_ctrl_if_t *ctrl_if;
    const audio_codec_gpio_if_t *gpio_if;
    const audio_codec_if_t *codec_if;
    esp_codec_dev_handle_t device;
} audio_resources_t;

static int16_t s_pcm_chunk[PCM_CHUNK_FRAMES * PCM_CHANNELS];

static esp_err_t configure_tdm_tx(audio_resources_t *resources)
{
    const i2s_chan_config_t channel_config = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    esp_err_t err = i2s_new_channel(&channel_config, &resources->tx, NULL);
    if (err != ESP_OK) {
        return err;
    }

    i2s_tdm_config_t tdm_config = {
        .clk_cfg = I2S_TDM_CLK_DEFAULT_CONFIG(PCM_SAMPLE_RATE_HZ),
        .slot_cfg = I2S_TDM_PCM_SHORT_SLOT_DEFAULT_CONFIG(
            I2S_DATA_BIT_WIDTH_16BIT,
            I2S_SLOT_MODE_STEREO,
            I2S_TDM_SLOT0 | I2S_TDM_SLOT1 | I2S_TDM_SLOT2 | I2S_TDM_SLOT3),
        .gpio_cfg = {
            .mclk = BSP_I2S_MCLK,
            .bclk = BSP_I2S_SCLK,
            .ws = BSP_I2S_LCLK,
            .dout = BSP_I2S_DOUT,
            .din = GPIO_NUM_NC,
        },
    };
    tdm_config.slot_cfg.total_slot = PCM_CHANNELS;
    err = i2s_channel_init_tdm_mode(resources->tx, &tdm_config);
    if (err != ESP_OK) {
        return err;
    }
    /* esp_codec_dev reconfigures the TDM slot in READY during open(). */
    return ESP_OK;
}

static esp_err_t configure_es8389(audio_resources_t *resources)
{
    ESP_RETURN_ON_ERROR(bsp_i2c_init(), TAG, "BSP I2C init failed");

    audio_codec_i2c_cfg_t i2c_config = {
        .port = I2C_NUM_0,
        .addr = ES8389_CODEC_DEFAULT_ADDR,
        .bus_handle = bsp_i2c_get_handle(),
    };
    resources->ctrl_if = audio_codec_new_i2c_ctrl(&i2c_config);
    ESP_RETURN_ON_FALSE(resources->ctrl_if, ESP_ERR_NO_MEM, TAG, "create ES8389 I2C interface failed");

    resources->gpio_if = audio_codec_new_gpio();
    ESP_RETURN_ON_FALSE(resources->gpio_if, ESP_ERR_NO_MEM, TAG, "create codec GPIO interface failed");

    audio_codec_i2s_cfg_t data_config = {
        .port = I2S_NUM_0,
        .tx_handle = resources->tx,
    };
    resources->data_if = audio_codec_new_i2s_data(&data_config);
    ESP_RETURN_ON_FALSE(resources->data_if, ESP_ERR_NO_MEM, TAG, "create TDM data interface failed");

    const esp_codec_dev_hw_gain_t hw_gain = {
        .pa_voltage = 5.0f,
        .codec_dac_voltage = 3.3f,
    };
    es8389_codec_cfg_t codec_config = {
        .ctrl_if = resources->ctrl_if,
        .gpio_if = resources->gpio_if,
        .codec_mode = ESP_CODEC_DEV_WORK_MODE_DAC,
        .pa_pin = BSP_POWER_AMP_IO,
        .pa_reverted = false,
        .master_mode = false,
        .use_mclk = false,
        .digital_mic = false,
        .invert_mclk = false,
        .invert_sclk = false,
        .hw_gain = hw_gain,
        .no_dac_ref = false,
    };
    resources->codec_if = es8389_codec_new(&codec_config);
    ESP_RETURN_ON_FALSE(resources->codec_if, ESP_FAIL, TAG, "create ES8389 codec driver failed");

    esp_codec_dev_cfg_t device_config = {
        .dev_type = ESP_CODEC_DEV_TYPE_OUT,
        .codec_if = resources->codec_if,
        .data_if = resources->data_if,
    };
    resources->device = esp_codec_dev_new(&device_config);
    ESP_RETURN_ON_FALSE(resources->device, ESP_ERR_NO_MEM, TAG, "create codec device failed");

    return ESP_OK;
}

static esp_err_t configure_es8389_tdm_format(audio_resources_t *resources)
{
    /*
     * esp_codec_dev 1.5.11's ES8389 set_fs() forces the normal I2S format.
     * Re-select the datasheet's DSP/PCM mode A through its public register API.
     * 0x68 = 16-bit samples (0x60) + DSP mode (0x08), mode A (bit 4 clear).
     */
    int ret = esp_codec_dev_write_reg(resources->device, ES8389_DAC_SERIAL_FORMAT_REG, 0x68);
    if (ret != ESP_CODEC_DEV_OK) {
        ESP_LOGE(TAG, "write ES8389 DAC serial format failed: %d", ret);
        return ESP_FAIL;
    }
    /* Keep the codec's LRCK format fields aligned with DSP-A as in its driver. */
    int master_format = 0;
    ret = esp_codec_dev_read_reg(resources->device, ES8389_MASTER_SERIAL_FORMAT_REG, &master_format);
    if (ret != ESP_CODEC_DEV_OK) {
        ESP_LOGE(TAG, "read ES8389 master serial format failed: %d", ret);
        return ESP_FAIL;
    }
    master_format = (master_format & ~ES8389_SERIAL_FORMAT_MASK) | ES8389_DSP_A;
    ret = esp_codec_dev_write_reg(resources->device, ES8389_MASTER_SERIAL_FORMAT_REG, master_format);
    if (ret != ESP_CODEC_DEV_OK) {
        ESP_LOGE(TAG, "write ES8389 master serial format failed: %d", ret);
        return ESP_FAIL;
    }

    /* The codec's DAC S2P slot selector defaults to slot 0; set and verify it. */
    int slot_reg = 0;
    ret = esp_codec_dev_read_reg(resources->device, ES8389_DAC_SLOT_SELECT_REG, &slot_reg);
    if (ret != ESP_CODEC_DEV_OK) {
        ESP_LOGE(TAG, "read ES8389 DAC slot selection failed: %d", ret);
        return ESP_FAIL;
    }
    slot_reg = (slot_reg & ~ES8389_DAC_SLOT_MASK) | ES8389_DAC_SLOT_RAMP_DEFAULT | ES8389_DAC_SLOT_0;
    ret = esp_codec_dev_write_reg(resources->device, ES8389_DAC_SLOT_SELECT_REG, slot_reg);
    if (ret != ESP_CODEC_DEV_OK) {
        ESP_LOGE(TAG, "write ES8389 DAC slot selection failed: %d", ret);
        return ESP_FAIL;
    }

    int verify_format = 0;
    int verify_master_format = 0;
    int verify_slot = 0;
    ret = esp_codec_dev_read_reg(resources->device, ES8389_DAC_SERIAL_FORMAT_REG, &verify_format);
    if (ret != ESP_CODEC_DEV_OK) {
        ESP_LOGE(TAG, "read back ES8389 format failed: %d", ret);
        return ESP_FAIL;
    }
    ret = esp_codec_dev_read_reg(resources->device, ES8389_MASTER_SERIAL_FORMAT_REG, &verify_master_format);
    if (ret != ESP_CODEC_DEV_OK) {
        ESP_LOGE(TAG, "read back ES8389 master format failed: %d", ret);
        return ESP_FAIL;
    }
    ret = esp_codec_dev_read_reg(resources->device, ES8389_DAC_SLOT_SELECT_REG, &verify_slot);
    if (ret != ESP_CODEC_DEV_OK) {
        ESP_LOGE(TAG, "read back ES8389 slot failed: %d", ret);
        return ESP_FAIL;
    }
    if ((verify_format & 0xEC) != 0x68 ||
        (verify_master_format & ES8389_SERIAL_FORMAT_MASK) != ES8389_DSP_A ||
        (verify_slot & ES8389_DAC_SLOT_MASK) != ES8389_DAC_SLOT_0) {
        ESP_LOGE(TAG, "ES8389 TDM register mismatch: DAC fmt=0x%02x master fmt=0x%02x slot=0x%02x",
                 verify_format, verify_master_format, verify_slot);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "ES8389 TDM format verified: DAC fmt=0x%02x master fmt=0x%02x, DAC slot=%u",
             verify_format, verify_master_format, (unsigned)((verify_slot & ES8389_DAC_SLOT_MASK) >> 4));
    return ESP_OK;
}

static esp_err_t restore_pcm_short_framing(audio_resources_t *resources)
{
    i2s_tdm_slot_config_t slot_config = I2S_TDM_PCM_SHORT_SLOT_DEFAULT_CONFIG(
        I2S_DATA_BIT_WIDTH_16BIT,
        I2S_SLOT_MODE_STEREO,
        I2S_TDM_SLOT0 | I2S_TDM_SLOT1 | I2S_TDM_SLOT2 | I2S_TDM_SLOT3);
    slot_config.total_slot = PCM_CHANNELS;
    slot_config.slot_bit_width = I2S_SLOT_BIT_WIDTH_16BIT;

    /* esp_codec_dev_open() enables TX, so stop it before restoring PCM-short WS. */
    ESP_RETURN_ON_ERROR(i2s_channel_disable(resources->tx), TAG, "disable I2S TDM before framing update failed");
    ESP_RETURN_ON_ERROR(i2s_channel_reconfig_tdm_slot(resources->tx, &slot_config),
                        TAG, "configure PCM-short TDM framing failed");
    ESP_RETURN_ON_ERROR(i2s_channel_enable(resources->tx), TAG, "enable I2S TDM after framing update failed");
    ESP_LOGI(TAG, "TDM framing: PCM-short, 4 slots x 16 bits, 16 kHz, WS pulse 1 BCLK");
    return ESP_OK;
}

static void generate_tone_chunk(uint32_t first_frame, size_t frame_count)
{
    for (size_t i = 0; i < frame_count; ++i) {
        const uint32_t frame = first_frame + (uint32_t)i;
        float envelope = 1.0f;
        if (frame < PCM_RAMP_FRAMES) {
            envelope = (float)frame / (float)PCM_RAMP_FRAMES;
        } else if (frame >= PCM_SAMPLE_RATE_HZ * PCM_TONE_SECONDS - PCM_RAMP_FRAMES) {
            const uint32_t remaining = PCM_SAMPLE_RATE_HZ * PCM_TONE_SECONDS - frame - 1;
            envelope = (float)remaining / (float)PCM_RAMP_FRAMES;
        }
        const float phase = 6.2831853071795864769f * (float)PCM_TONE_HZ * (float)frame / (float)PCM_SAMPLE_RATE_HZ;
        const int16_t sample = (int16_t)lrintf(sinf(phase) * (float)PCM_PEAK * envelope);
        /* Repeating the low-level tone in all slots tolerates stereo DAC S2P routing. */
        for (size_t channel = 0; channel < PCM_CHANNELS; ++channel) {
            s_pcm_chunk[i * PCM_CHANNELS + channel] = sample;
        }
    }
}

static void release_audio(audio_resources_t *resources)
{
    if (resources->device) {
        (void)esp_codec_dev_close(resources->device);
        esp_codec_dev_delete(resources->device);
    }
    if (resources->codec_if) {
        (void)audio_codec_delete_codec_if(resources->codec_if);
    }
    if (resources->data_if) {
        (void)audio_codec_delete_data_if(resources->data_if);
    }
    if (resources->ctrl_if) {
        (void)audio_codec_delete_ctrl_if(resources->ctrl_if);
    }
    if (resources->gpio_if) {
        (void)audio_codec_delete_gpio_if(resources->gpio_if);
    }
    if (resources->tx) {
        (void)i2s_channel_disable(resources->tx);
        (void)i2s_del_channel(resources->tx);
    }
}

esp_err_t audio_korvo_play_test_tone(void)
{
    audio_resources_t resources = {0};
    esp_err_t err = ESP_FAIL;
    printf("[AUDIO] PCM tone test starting\n");

    err = configure_tdm_tx(&resources);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2S0 TDM TX init failed: %s", esp_err_to_name(err));
        goto cleanup;
    }
    ESP_LOGI(TAG, "I2S0 TDM TX initialized at %d Hz, %d-bit, %d slots",
             PCM_SAMPLE_RATE_HZ, PCM_BITS_PER_SAMPLE, PCM_CHANNELS);

    err = configure_es8389(&resources);
    if (err != ESP_OK) {
        goto cleanup;
    }
    ESP_LOGI(TAG, "ES8389 and GPIO%d speaker PA initialized", BSP_POWER_AMP_IO);

    esp_codec_dev_sample_info_t sample_info = {
        .bits_per_sample = PCM_BITS_PER_SAMPLE,
        .channel = PCM_CHANNELS,
        .channel_mask = 0x0F,
        .sample_rate = PCM_SAMPLE_RATE_HZ,
        .mclk_multiple = I2S_MCLK_MULTIPLE_256,
    };
    err = (esp_codec_dev_open(resources.device, &sample_info) == ESP_CODEC_DEV_OK) ? ESP_OK : ESP_FAIL;
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "open ES8389 PCM stream failed");
        goto cleanup;
    }

    err = configure_es8389_tdm_format(&resources);
    if (err != ESP_OK) {
        goto cleanup;
    }
    err = restore_pcm_short_framing(&resources);
    if (err != ESP_OK) {
        goto cleanup;
    }
    if (esp_codec_dev_set_out_vol(resources.device, 20) != ESP_CODEC_DEV_OK) {
        ESP_LOGE(TAG, "set initial low codec volume failed");
        err = ESP_FAIL;
        goto cleanup;
    }
    ESP_LOGI(TAG, "PCM: %d Hz, signed 16-bit, 4 TDM slots, 440 Hz, peak 8%%, volume 20%%",
             PCM_SAMPLE_RATE_HZ);
    printf("[AUDIO] Starting bounded PCM transmission\n");

    const uint32_t total_frames = PCM_SAMPLE_RATE_HZ * PCM_TONE_SECONDS;
    uint32_t sent_frames = 0;
    while (sent_frames < total_frames) {
        const size_t frames = (total_frames - sent_frames < PCM_CHUNK_FRAMES)
                                  ? (size_t)(total_frames - sent_frames)
                                  : PCM_CHUNK_FRAMES;
        generate_tone_chunk(sent_frames, frames);
        const int bytes = (int)(frames * PCM_CHANNELS * sizeof(int16_t));
        if (esp_codec_dev_write(resources.device, s_pcm_chunk, bytes) != ESP_CODEC_DEV_OK) {
            ESP_LOGE(TAG, "PCM write failed at frame %" PRIu32, sent_frames);
            err = ESP_FAIL;
            goto cleanup;
        }
        sent_frames += (uint32_t)frames;
    }
    printf("[AUDIO] PCM transmission finished: %" PRIu32 " frames\n", sent_frames);
    ESP_LOGI(TAG, "PCM transmission completed; this is software status, not audible validation");
    err = ESP_OK;

cleanup:
    release_audio(&resources);
    if (err != ESP_OK) {
        printf("[AUDIO] PCM test failed: %s\n", esp_err_to_name(err));
    }
    return err;
}
