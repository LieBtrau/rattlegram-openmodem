#pragma once

#include <Arduino.h>
#include "driver/i2s.h"
#include "BufferSync.h"

/**
 * Base Class for both the ADC and I2S sampler
 **/
class I2SOutput
{
private:
    // i2s port
    i2s_port_t m_i2sPort;
    // I2S write task
    TaskHandle_t m_i2s_writerTaskHandle = NULL;
    // src of samples for us to play
    BufferSync *m_sample_generator;

public:
    I2SOutput(i2s_port_t i2sPort) : m_i2sPort(i2sPort) {}
    void start(BufferSync *sample_generator);
    void stop();
    friend void i2sWriterTask(void *param);
};