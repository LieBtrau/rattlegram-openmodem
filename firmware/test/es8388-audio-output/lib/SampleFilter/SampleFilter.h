#pragma once
/*

FIR filter designed with
 http://t-filter.appspot.com

sampling frequency: 48000 Hz

* 0 Hz - 4000 Hz
  gain = 1
  desired ripple = 1 dB
  actual ripple = 0.717687757593362 dB

* 4500 Hz - 24000 Hz
  gain = 0
  desired attenuation = -40 dB
  actual attenuation = -40.40552815878151 dB

*/

class SampleFilter
{
public:
    static const int SAMPLEFILTER_TAP_NUM = 149;
    SampleFilter();
    void put(float input);
    float get();

private:
    float history[SAMPLEFILTER_TAP_NUM];
    unsigned int last_index;
};
