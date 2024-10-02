#pragma once
#include <stdint.h>
#include "SampleSource.h"

void sampleSink(int16_t samples[], int count);

class QueueGenerator : public SampleSource
{
private:
    int m_sampleRate;
public:
    QueueGenerator(int sampleRate);
    virtual uint32_t sampleRate() { return m_sampleRate; }
    virtual void getFrames(Frame_t *frames, int number_frames);
};

