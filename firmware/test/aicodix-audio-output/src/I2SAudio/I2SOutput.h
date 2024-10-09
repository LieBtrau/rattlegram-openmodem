#pragma once

#include <Arduino.h>
#include "driver/i2s.h"

class I2SOutput
{
private:
    // i2s port
    i2s_port_t m_i2sPort;
    // I2S write task
    TaskHandle_t m_i2s_writerTaskHandle = NULL;
    // src of samples for us to play
    xQueueHandle m_sample_sink;

public:
    I2SOutput(i2s_port_t i2sPort) : m_i2sPort(i2sPort) {}
    void start(xQueueHandle sample_sink);
    void stop();
    friend void i2sWriterTask(void *param);
};