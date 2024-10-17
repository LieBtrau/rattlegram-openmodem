#pragma once

#include <Arduino.h>
#include "driver/i2s.h"

class I2SInput
{
private:
    // i2s port
    i2s_port_t m_i2sPort;
    // I2S reader task
    TaskHandle_t m_i2s_readerTaskHandle = NULL;
    // src of samples for us to play
    xQueueHandle m_sample_source;

public:
    I2SInput(i2s_port_t i2sPort, i2s_pin_config_t *pin_config) : m_i2sPort(i2sPort){};
    void start(xQueueHandle sample_source);
    void stop();
    friend void i2sReaderTask(void *param);
};
