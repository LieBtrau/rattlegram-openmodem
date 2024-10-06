#include "audio.h"
#include "driver/i2s.h"

static const char *TAG = "audio";
static I2SOutput *output;
static ES8388 audioShield(33, 32);
static i2s_port_t m_i2sPort;
static i2s_config_t m_i2sConfig;
static i2s_pin_config_t i2s_pin_config;

bool audio_init(BufferSync *dac_sample_generator, const uint32_t sampleRate)
{
    // init ES8388
    if (!audioShield.init())
    {
        ESP_LOGE(TAG, "ES8388 not found");
    }
    audioShield.outputSelect(ES8388::OutSel::OUT2); // EARPHONE-jack on ESP32-A1S
    audioShield.setOutputVolume(ES8388::OutSel::OUT2, 30);
    audioShield.mixerSourceControl(DACOUT); // Use LIN and RIN as output

    m_i2sPort = I2S_NUM_0;
    m_i2sConfig = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX), // Only TX
        .sample_rate = sampleRate,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT, // 16-bit per channel
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT, // 2-channels
        .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_STAND_I2S),
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1, // Interrupt level 1
        .dma_buf_count = 2,
        .dma_buf_len = 256,
        .use_apll = true,
        .tx_desc_auto_clear = true,
        .fixed_mclk = (int)(sampleRate * 256)};
    i2s_pin_config =
        {
            .bck_io_num = 27,   // Serial Clock (SCK)
            .ws_io_num = 25,    // Word Select (WS)
            .data_out_num = 26, // data out to audio codec
            .data_in_num = 35   // data from audio codec
        };

    // install and start i2s driver
    ESP_ERROR_CHECK(i2s_driver_install(m_i2sPort, &m_i2sConfig, 0, NULL));
    ESP_ERROR_CHECK(i2s_zero_dma_buffer(m_i2sPort));
    ESP_ERROR_CHECK(i2s_set_pin(m_i2sPort, &i2s_pin_config));
    WRITE_PERI_REG(PIN_CTRL, READ_PERI_REG(PIN_CTRL) & 0xFFFFFFF0);
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0_CLK_OUT1);
    delay(5);
    output = new I2SOutput(m_i2sPort);
    output->start(dac_sample_generator);

    return true;
}

void audio_stop()
{
    output->stop();
    ESP_ERROR_CHECK(i2s_stop(m_i2sPort));
    ESP_ERROR_CHECK(i2s_zero_dma_buffer(m_i2sPort));
    ESP_ERROR_CHECK(i2s_driver_uninstall(m_i2sPort));
}