# rattlegram-openmodem
radio modem and TNC over rattlegram packet radio

## Overview
[Rattlegram-offline test](./firmware/test/rattle-offline/) is a test to check the rattlegram decoder on an ESP32.  The code runs on an ESP32-A1S, which has PSRAM.  With PSRAM disabled, the code crashes.  

Decoding a 1s mono 16-bit, 48kHz audio file took 37.2s.  After resampling that file to 8kHz, the decoding still took 34.2s.  
Some suggestions for speeding up the decoding (see [github.com/aicodix/modem](https://github.com/aicodix/modem/issues/9)) dramatically improved the decoding time.  The 1s mono 16-bit, 48kHz audio file now decodes in 6.4s.

| Setup      | Audio File length | Audio data         | Decoding Time [ms] |
| ----------- | ---------------- | ----------------------- | ------------------ |
| ESP32-A1S, master-branch   | 1.6s             | mono 16-bit, 8kHz       | 1476               |
| ESP32-A1S, Espressif 6.0.1, master-branch   | 1.6s             | mono 16-bit, 8kHz       | 1429               |
| ESP32-A1S, Espressif 6.0.1, short-branch   | 1.2s             | mono 16-bit, 8kHz      | 806               |