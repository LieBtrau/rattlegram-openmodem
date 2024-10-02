#include "QueueGenerator.h"
// #include "modem_config.hh"

#include <queue>
static std::queue<int16_t> sampleQueue;

void sampleSink(int16_t samples[], int count)
{
	// ESP_LOGI(TAG, "sampleSink: %d", count);
	for (int i = 0; i < count; i++)
	{
		sampleQueue.push(samples[i]);
	}
	// ESP_LOGI(TAG, "sampleQueue: %d", sampleQueue.size());
}

QueueGenerator::QueueGenerator(int sampleRate): m_sampleRate(sampleRate)
{
}

void QueueGenerator::getFrames(Frame_t *frames, int number_frames)
{
    for (int i = 0; i < number_frames; i++)
    {
        int16_t sample;
        if (sampleQueue.empty())
        {
            sample = 0; // Dummy value
        }
        else
        {
            sample = sampleQueue.front();
            sampleQueue.pop();
        }
        frames[i].left = sample;
        frames[i].right = 0;
    }
}
