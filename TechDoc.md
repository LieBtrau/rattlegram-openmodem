# Microcontroller
Support for bluetooth serial port profile ([SPP](https://www.bluetooth.com/specifications/specs/serial-port-profile-1-1/)) is needed.  Only the ESP32 has this feature.  There's already support for it in Arduino/PlatformIO as well.

# Analog audio path
An audio codec will be used instead of the ADC/DAC inside the ESP32 because it offers more functions.
* Sampling rate of 8kHz and 48kHz must be supported.
* Two audio inputs needed : 
    * Microphone input for operator
    * Line input for connecting to output of radio
* Two audio outputs needed : 
    * Line output to operator (headset)
    * Line output to radio's microphone input
Suggestion to use left channel as audio flowing from operator to HT and right channel as audio flowing from HT to operator.

# OFDM-modem
The OFDM modem will run at 8kHz.  At 48kHz, the ESP32 is too slow to encode the data (x20 slower than on 8kHz).  For the decoder, downsampling from 48kHz to 8kHz also speeds up the decoding process, but not as much as encoding.

# Resampling
* [Aicodix DSP resampler](https://github.com/aicodix/dsp/blob/master/resampler.hh)
  * [example](https://github.com/aicodix/disorders/blob/dd82ccef185ed7746f732bbf85a10187095f3599/sfo.cc#L10)
* [Audio-Smarc](https://audio-smarc.sourceforge.net/)
* [Speex-Resample](https://android.googlesource.com/platform/external/speex/+/donut-release/libspeex/resample.c)


## Options
* ES8388 : Used in Aithinker Audio toolkit, stereo in/out
    * available at JLCPCB, $0.99/pce
* WM8978 : Used in M5Stack, stereo in/out
    * JLCPCB : not available
* SGTL5000 : Used in Teensy Audio shield, stereo in/out