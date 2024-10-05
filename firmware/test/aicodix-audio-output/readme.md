# Testing audio output from the OFDM-modem
[Git ID](https://github.com/LieBtrau/rattlegram-openmodem/commit/db3b2bcddd17e0563c977b4d5ea86abc27864711)

1. Record the audio.  Be sure to start the command between packets.  On the command line, run the following:
```
rec -c 1 -b 16 -t wav -r 48000 test.wav silence 1 0.1 2% 1 3.0 2% 
```
2. Build the ['next'-branch of aicodix modem](https://github.com/aicodix/modem/tree/next).  You'll also need the CODE and DSP repositories from aicodix.
3. Decode the recorded audio:
```
./decode test.wav decode.dat
symbol pos: 5128
coarse cfo: 1600.01 Hz 
oper mode: 23
call sign:    3DYN1S
demod ........ done
Es/N0 (dB): 24.3353 24.988 25.8578 26.2067 26.2162 26.2056 26.4491 26.5571
```
Continuous decoding can be done with the following command:
```
while arecord -f S16_LE -c 1 -r 48000 - | ./decode - - ; do echo ; sleep 1 ; done
```

