#pragma once

#include "SampleSource.h"

class SinWaveGenerator : public SampleSource
{
private:
    uint32_t m_sample_rate;
    uint16_t m_phaseAccu;
    uint16_t m_tuningWord;
    uint16_t m_lut[256];

public:
    SinWaveGenerator(int sample_rate, int frequency, float magnitude, float offset=0);
    virtual uint32_t sampleRate() { return m_sample_rate; }
    virtual void getFrames(Frame_t *frames, int number_frames);
};

