#include "Upsampler.h"

/**
 * @brief Construct a new Upsampler:: Upsampler object
 * 
 * @param sample_rate Output sample rate
 * @param input source of samples, will be upsampled
 * @param filter interpolation filter
 * @param factor upsampling factor
 */
Upsampler::Upsampler(int sample_rate, SampleSource *input, SampleFilter *filter): 
    m_sample_rate(sample_rate), 
    m_input(input), 
    m_filter(filter)
    {
        m_factor = sample_rate / input->sampleRate();
    }


/**
 * @brief Called to get the next frame of samples
 * 
 * @param frames will contain the next frame of samples
 * @param number_frames number of frames to get
 */
void Upsampler::getFrames(Frame_t *frames, int number_frames)
{
    Frame_t sourceFrame;

    while(number_frames--)
    {
        // Put the input sample into the filter every m_factor samples
        if(m_sample_count == 0)
        {
            m_input->getFrames(&sourceFrame, 1);
            m_filter->put(sourceFrame.left);
        }
        else
        {
            m_filter->put(0);
        }
        m_sample_count = m_sample_count < (m_factor - 1) ? m_sample_count + 1 : 0;

        // Get the next sample from the filter
        frames->left = m_filter->get() * m_factor;
        frames->right = 0;
        frames++;
    }
}