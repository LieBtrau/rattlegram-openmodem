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

## Options
* ES8388 : Used in Aithinker Audio toolkit, stereo in/out
    * available at JLCPCB, $0.99/pce
* WM8978 : Used in M5Stack, stereo in/out
    * JLCPCB : not available
* SGTL5000 : Used in Teensy Audio shield, stereo in/out