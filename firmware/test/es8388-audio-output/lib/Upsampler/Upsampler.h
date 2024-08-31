#pragma once

#include "SampleSource.h"
#include "SampleFilter.h"

class Upsampler : public SampleSource
{
private:
    uint32_t m_sample_rate;
    SampleSource *m_input;
    SampleFilter *m_filter;
    int m_sample_count = 0;
    int m_factor;

public:
    Upsampler(int sample_rate, SampleSource *input, SampleFilter *filter);
    virtual uint32_t sampleRate() { return m_sample_rate; }
    virtual void getFrames(Frame_t *frames, int number_frames);
};

