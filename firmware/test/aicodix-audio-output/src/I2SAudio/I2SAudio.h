#pragma once

#include "I2SOutput.h"
#include "driver/i2s.h"

struct BufferSyncMessage
{
    uint8_t *data;
    std::size_t size;
};

class I2SAudio
{
private:
    I2SOutput *m_output = nullptr;
    const i2s_port_t m_i2sPort;
    i2s_config_t m_i2sConfig;
    i2s_pin_config_t m_i2s_pin_config;
    const uint32_t m_sampleRate;
    xQueueHandle m_sample_sink = nullptr;

public:
    I2SAudio(const uint32_t sampleRate, int pin_BCK, int pin_WS, int pin_DOUT, int pin_DIN);
    ~I2SAudio();
    void init();
    void start_output(std::size_t maxMessages);
    bool addSinkSamples(uint8_t *data, std::size_t size);
    void stop();
};