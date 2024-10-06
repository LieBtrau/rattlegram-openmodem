#include "I2SAudio.h"

#include "BufferSync.h"

static const char *TAG = "audio";

/**
 * @brief Construct a new I2SAudio::I2SAudio object
 * 
 * @param sampleRate Sample rate in Hz (e.g. 8000)
 * @param pin_BCK I2S Bit Clock (BCK) pin
 * @param pin_WS I2S Word Select (WS) pin
 * @param pin_DOUT I2S Data Out (DOUT) pin (data from MCU to audio codec)
 * @param pin_DIN I2S Data In (DIN) pin (data from audio codec to MCU)
 */
I2SAudio::I2SAudio(const uint32_t sampleRate, int pin_BCK, int pin_WS, int pin_DOUT, int pin_DIN): 
    m_sampleRate(sampleRate), 
    m_i2sPort(I2S_NUM_0)
{
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
    m_i2s_pin_config =
        {
            .bck_io_num = pin_BCK,      // Serial Clock (SCK)
            .ws_io_num = pin_WS,        // Word Select (WS)
            .data_out_num = pin_DOUT,   // data out to audio codec
            .data_in_num = pin_DIN      // data from audio codec
        };
}

I2SAudio::~I2SAudio()
{
    stop();
    delete m_output;
}

/**
 * @brief Configure the ESP32's I2S peripheral
 * 
 */
void I2SAudio::init()
{
    ESP_ERROR_CHECK(i2s_driver_install(m_i2sPort, &m_i2sConfig, 0, NULL));
    ESP_ERROR_CHECK(i2s_zero_dma_buffer(m_i2sPort));
    ESP_ERROR_CHECK(i2s_set_pin(m_i2sPort, &m_i2s_pin_config));
    WRITE_PERI_REG(PIN_CTRL, READ_PERI_REG(PIN_CTRL) & 0xFFFFFFF0);
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0_CLK_OUT1);
    delay(5);
}

/**
 * @brief Start the I2S output task
 * 
 * @param dac_sample_queue Queue of samples to be sent to the DAC output
 */
void I2SAudio::start_output(BufferSync *dac_sample_queue)
{
    if(!m_output)
    {
        m_output = new I2SOutput(m_i2sPort);
    }
    m_output->start(dac_sample_queue);
}

/**
 * @brief Stop the I2S output task and the I2S peripheral
 * 
 */
void I2SAudio::stop()
{
    if(m_output)
    {
        m_output->stop();
    }
    ESP_ERROR_CHECK(i2s_stop(m_i2sPort));
    ESP_ERROR_CHECK(i2s_zero_dma_buffer(m_i2sPort));
    ESP_ERROR_CHECK(i2s_driver_uninstall(m_i2sPort));
}