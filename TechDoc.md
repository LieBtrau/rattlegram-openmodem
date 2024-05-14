# Microcontroller
Support for bluetooth serial port profile ([SPP](https://www.bluetooth.com/specifications/specs/serial-port-profile-1-1/)) is needed.  Only the ESP32 has this feature.  There's already support for it in Arduino/PlatformIO as well.

# Analog audio path
An audio codec will be used instead of the ADC/DAC inside the ESP32 because it offers more functions.
* Sampling rate of 8kHz and 48kHz must be supported.
* Microphone input for operator and for radio
* Line output to operator (headset) and to radio

## Options
* ES8388 : Used in Aithinker Audio toolkit, stereo in/out 