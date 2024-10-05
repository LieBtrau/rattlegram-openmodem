#pragma once
#include "I2SOutput.h"
#include "BufferSync.h"

class ES8388Output : public I2SOutput
{
public:
    ES8388Output(i2s_port_t i2sPort, i2s_pin_config_t* pin_config);
    void start(BufferSync *sample_generator, const int sampleRate);

protected:
    void configureI2S();

private:
    i2s_pin_config_t m_pin_config;
};