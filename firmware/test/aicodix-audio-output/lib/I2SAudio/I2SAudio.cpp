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
    delete m_input;
    vQueueDelete(m_sample_sink);
    vQueueDelete(m_sample_source);
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
void I2SAudio::start_input(std::size_t maxMessages)
{
    if (!m_input)
    {
        m_input = new I2SInput(m_i2sPort, &m_i2s_pin_config);
    }
    if (!m_sample_source)
    {
        m_sample_source = xQueueCreate(maxMessages, sizeof(BufferSyncMessage));
    }
    m_input->start(m_sample_source);
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
 * @param channel AudioChannel::LEFT, AudioChannel::RIGHT, or AudioChannel::BOTH (copy data to both channels)
 * @return true if the samples were added to the queue
 * @return false if the queue is full
 */
bool I2SAudio::addSinkSamples(int16_t samples[], int sample_count, AudioChannel channel)
{
    int16_t frames[2 * sample_count];
    for (int i = 0; i < sample_count; i++)
    {
        frames[2 * i] = (channel != AudioChannel::RIGHT ? samples[i] : 0);    // left channel
        frames[2 * i + 1] = (channel != AudioChannel::LEFT ? samples[i] : 0); // right channel
    }

    // Send samples in chunks of SAMPLE_BUFFER_SIZE
    size_t byte_count = 2 * sample_count * sizeof(int16_t);
    uint8_t *byte_samples = reinterpret_cast<uint8_t *>(frames);

    while (byte_count > 0)
    {
        size_t bytes_to_send = byte_count > SAMPLE_BUFFER_SIZE ? SAMPLE_BUFFER_SIZE : byte_count;
        if (!addRawSinkSamples(byte_samples, bytes_to_send))
        {
            return false;
        }
        byte_samples += bytes_to_send;
        byte_count -= bytes_to_send;
    }
    return true;
}

bool I2SAudio::addRawSinkSamples(uint8_t samples[], int count)
{
    BufferSyncMessage message;
    if (count > sizeof(message.data))
    {
        ESP_LOGE(TAG, "Sample count exceeds buffer size");
        return false;
    }
    message.size = count;
    memcpy(message.data, samples, count);
    return xQueueSend(m_sample_sink, &message, portMAX_DELAY) == pdTRUE;
}

/**
 * @brief Get samples from the I2S input queue
 *
 * @param samples Pointer to the array of channel samples
 * @param sample_count Number of samples
 *  Upon calling this function, sample_count should contain the maximum number of samples that can be stored in samples[].
 *  Upon return, sample_count will contain the actual number of samples stored in samples[].
 * @return true if the desired number of samples were read from the queue
 */
bool I2SAudio::getSourceSamples(int16_t samples[], size_t &sample_count, AudioChannel channel)
{
    size_t samples_read = 0;
    std::queue<int16_t>* queue = nullptr;

    switch(channel)
    {
        case AudioChannel::LEFT:
            queue = &m_left_samples;
            break;
        case AudioChannel::RIGHT:
            queue = &m_right_samples;
            break;
        case AudioChannel::BOTH:
            return false;
    }

    while (samples_read < sample_count)
    {
        if (!queue->empty())
        {
            samples[samples_read] = queue->front();
            queue->pop();
            samples_read++;
        }
        else
        {
            if (!fillSampleQueue())
            {
                break;
            }
        }
    }
    sample_count = samples_read;
    return true;
}

void I2SAudio::getRawSourceSamples(uint8_t samples[], size_t &byte_count)
{
    BufferSyncMessage message;
    byte_count = 0;
    if (xQueueReceive(m_sample_source, &message, portMAX_DELAY) == pdTRUE)
    {
        mempcpy(samples, message.data, message.size);
        byte_count = message.size;
    }
}

bool I2SAudio::fillSampleQueue()
{
    const size_t MAX_QUEUE_SIZE = 256;
    BufferSyncMessage message;
    int16_t *samples = nullptr;
    getRawSourceSamples(message.data, message.size);
    if (message.size == 0)
    {
        return false;
    }
    samples = reinterpret_cast<int16_t *>(message.data);
    size_t total_sample_count = message.size / sizeof(int16_t); // total number of samples for left + right channels
    for (int i = 0; i < total_sample_count; i++)
    {
        if (i % 2 == 0)
        {
            // Even samples = left channel
            if (m_left_samples.size() >= MAX_QUEUE_SIZE)
            {
                m_left_samples.pop(); // remove the oldest sample
            }
            m_left_samples.push(samples[i]);
        }
        else
        {
            // Odd samples = right channel
            if (m_right_samples.size() >= MAX_QUEUE_SIZE)
            {
                m_right_samples.pop(); // remove the oldest sample
            }
            m_right_samples.push(samples[i]);
        }
    }
    return true;
}