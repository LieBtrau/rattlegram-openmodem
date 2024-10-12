#include "I2SAudio.h"

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
I2SAudio::I2SAudio(const uint32_t sampleRate, int pin_BCK, int pin_WS, int pin_DOUT, int pin_DIN) : m_sampleRate(sampleRate),
                                                                                                    m_i2sPort(I2S_NUM_0)
{
    m_i2sConfig = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX),
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
            .bck_io_num = pin_BCK,    // Serial Clock (SCK)
            .ws_io_num = pin_WS,      // Word Select (WS)
            .data_out_num = pin_DOUT, // data out to audio codec
            .data_in_num = pin_DIN    // data from audio codec
        };
}

I2SAudio::~I2SAudio()
{
    stop();
    delete m_output;
    vQueueDelete(m_sample_sink);
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
void I2SAudio::start_output(std::size_t maxMessages)
{
    if (!m_output)
    {
        m_output = new I2SOutput(m_i2sPort);
    }
    if (!m_sample_sink)
    {
        m_sample_sink = xQueueCreate(maxMessages, sizeof(BufferSyncMessage));
    }
    m_output->start(m_sample_sink);
}

/**
 * @brief Start the I2S input task
 *
 * @param adc_sample_queue Queue of samples to be received from the ADC input
 */
void I2SAudio::start_input(std::size_t maxMessages, size_t maxSamples)
{
    if (!m_input)
    {
        m_input = new I2SInput(m_i2sPort, &m_i2s_pin_config);
    }
    if (!m_sample_source)
    {
        m_sample_source = xQueueCreate(maxMessages, sizeof(BufferSyncMessage));
    }
    m_input->start(m_sample_source, maxSamples);
}

/**
 * @brief Stop the I2S output task and the I2S peripheral
 *
 */
void I2SAudio::stop()
{
    if (m_output)
    {
        m_output->stop();
    }
    ESP_ERROR_CHECK(i2s_stop(m_i2sPort));
    ESP_ERROR_CHECK(i2s_zero_dma_buffer(m_i2sPort));
    ESP_ERROR_CHECK(i2s_driver_uninstall(m_i2sPort));
}

/**
 * @brief Add samples to the I2S output queue
 *
 * @param samples Pointer to the array of samples
 * @param count Number of samples
 * @param channel AudioSinkChannel::LEFT, AudioSinkChannel::RIGHT, or AudioSinkChannel::BOTH
 * @return true if the samples were added to the queue
 * @return false if the queue is full
 */
bool I2SAudio::addSinkSamples(int16_t samples[], int sample_count, AudioSinkChannel channel)
{
    int16_t *frames = new int16_t[2 * sample_count];
    if (frames == nullptr)
    {
        return false;
    }
    for (int i = 0; i < sample_count; i++)
    {
        frames[2 * i] = (channel != AudioSinkChannel::RIGHT ? samples[i] : 0);    // left channel
        frames[2 * i + 1] = (channel != AudioSinkChannel::LEFT ? samples[i] : 0); // right channel
    }

    return addRawSinkSamples(reinterpret_cast<uint8_t *>(frames), 2 * sample_count * sizeof(int16_t));
}

bool I2SAudio::addRawSinkSamples(uint8_t samples[], int count)
{
    BufferSyncMessage message;
    message.data = samples;
    message.size = count;
    return xQueueSend(m_sample_sink, &message, portMAX_DELAY) == pdTRUE;
}

/**
 * @brief Get samples from the I2S input queue
 *
 * @param samples Pointer to the array of samples
 * @param count Number of samples
 */
void I2SAudio::getSourceSamples(int16_t *samples[], size_t &count)
{
    BufferSyncMessage message;
    if (xQueueReceive(m_sample_source, &message, portMAX_DELAY) == pdTRUE)
    {
        *samples = reinterpret_cast<int16_t *>(message.data);
        count = message.size / sizeof(int16_t);
    }
}

void I2SAudio::getRawSourceSamples(uint8_t *samples[], size_t& byte_count)
{
    BufferSyncMessage message;
    if (xQueueReceive(m_sample_source, &message, portMAX_DELAY) == pdTRUE)
    {
        *samples = message.data;
        byte_count = message.size;
    }
}