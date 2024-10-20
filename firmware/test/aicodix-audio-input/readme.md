# Purpose
Test that modem can decode incoming data

# Test setup
## Hardware
3.5mm audio cable splitter (Nedis CAGB22150BK02) connected with two audio jack cables: 
* green port connected to LINEIN of ESP32-A1S kit.
* red port connected to EARPHONES of ESP32-A1S kit.

## Software
1. Create aicodix encoded audio sample:
```
dd if=/dev/urandom of=uncoded.dat bs=1 count=256
./encode encoded.wav 8000 16 1 1600 23 NOCALL ./uncoded.dat
```
2. Set laptop's audio input and output levels correctly.  This can be done by loading the "es8388-esp32a1-s" example in the ESP32.  This example does I2S-loopback.  
* Open [REW](https://www.roomeqwizard.com/) and start its "Scope" feature. 
* Play the encoded audio:
```
aplay ./encoded.wav
```
* Adjust the microphone and speaker levels so that the signal is not clipped.

3. Upload this firmware to ESP32-A1S kit.
4. Play the encoded audio once more (using the same command as above).
5. The ESP32 should decode the audio and show the logging on its serial port.

# Expected output
```
symbol pos: 904
coarse cfo: 1599.99 Hz 
oper mode: 23
[ 60901][I][main.cpp:82] loop(): [main] Metadata: 1711783377
modulation bits: 2
demod 8 rows
........ done
Es/N0 (dB): 27.954 27.0725 25.2305 23.8397 23.7701 24.0801 24.2568 24.3311
data bits: 2048
```