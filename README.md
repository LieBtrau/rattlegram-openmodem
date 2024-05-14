# rattlegram-openmodem
radio modem and TNC over rattlegram packet radio

## Overview
[Rattlegram-offline test](./firmware/test/rattle-offline/) is a test to check the rattlegram decoder on an ESP32.  The code runs on an ESP32-A1S, which has PSRAM.  With PSRAM disabled, the code crashes.  

Decoding a 1s mono 16-bit, 48kHz audio file took 37.2s.  After resampling that file to 8kHz, the decoding still took 34.2s.  
Some suggestions for speeding up the decoding (see [github.com/aicodix/modem](https://github.com/aicodix/modem/issues/9)) dramatically improved the decoding time.  The 1s mono 16-bit, 48kHz audio file now decodes in 6.4s.  The 1s mono 16-bit, 8kHz audio file decodes in 1.5s.

As it takes longer to decode a signal than to receive it, full real-time operation is not possible.  However, it would be possible to buffer a packet that is strong enough to break the squelch and then decode it.  This would allow for a real-time operation with a delay of a few seconds.