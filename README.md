# rattlegram-openmodem
radio modem and TNC over rattlegram packet radio

## Overview
[Rattlegram-offline test](./firmware/test/rattle-offline/) is a test to check the rattlegram decoder on an ESP32.  The code runs on an ESP32-A1S, which has PSRAM.  With PSRAM disabled, the code crashes.  

Decoding a 1s mono 16-bit, 48kHz audio file took 37.2s.  After resampling that file to 8kHz, the decoding still took 34.2s.  

It's unfortunate, but the ESP32 is too slow to decode rattlegram in real-time.  Improvements to the code could be made, but that's not what I'm interested in.  