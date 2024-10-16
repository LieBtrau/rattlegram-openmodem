#pragma once

#include "I2SOutput.h"
#include "I2SInput.h"
#include "driver/i2s.h"

using namespace std;

static const size_t SAMPLE_BUFFER_SIZE = 256;

struct BufferSyncMessage
{
    uint8_t data[SAMPLE_BUFFER_SIZE];      //!< Pointer to the array of samples
    size_t size;                     
};

class I2SAudio
{
private:
    const i2s_port_t m_i2sPort;
    i2s_config_t m_i2sConfig;
    i2s_pin_config_t m_i2s_pin_config;
    const uint32_t m_sampleRate;

    I2SOutput *m_output = nullptr;
    xQueueHandle m_sample_sink = nullptr;
    
    I2SInput *m_input = nullptr;
    xQueueHandle m_sample_source = nullptr;

public:
    enum class AudioSinkChannel
    {
        LEFT, RIGHT, BOTH
    };
    I2SAudio(const uint32_t sampleRate, int pin_BCK, int pin_WS, int pin_DOUT, int pin_DIN);
    ~I2SAudio();
    void init();
    void start_output(size_t maxMessages);
    void start_input(size_t maxMessages);
    bool addSinkSamples(int16_t samples[], int count, AudioSinkChannel channel);
    bool addRawSinkSamples(uint8_t samples[], int count);
    bool getSourceSamples(int16_t left_samples[], int16_t right_samples[], size_t &sample_count_per_channel);
    void getRawSourceSamples(uint8_t samples[], size_t& count);
    void stop();
};