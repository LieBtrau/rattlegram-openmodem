# Requirement
## Firmware
* Should be firmware compatible to the [OpenModem](https://unsigned.io/hardware/OpenModem.html), so that the Unsigned.io tools can be used to interact with it.  This also includes the ability to function as a TNC for use with:
  * ~~[APRSdroid](https://aprsdroid.org/)~~ : APRS is an awful protocol to support : inconsistent, inefficient, awful documentation
  * [Sideband for Android](https://unsigned.io/software/Sideband.html).  It's better thought out than APRSdroid and includes encryption.
  * [Codec2_talkie](https://github.com/sh123/codec2_talkie/releases)
* Interfacing with the openModem should be done over Bluetooth.  As such, it can easily be used with laptops as well as mobile devices.

## Hardware
* Bluetooth serial port support
* Battery operated
* Charge via USB-C
* [OHIS-SATA-interface](https://github.com/LieBtrau/ohis-ht-interface?tab=readme-ov-file#sata-interface) to HTs.
* analog audio in/out for interfacing with HTs

# Prior Art
* [M17 Analog Hotspot Gateway Project](https://github.com/nakhonthai/M17AnalogGateway/tree/master):
  * using ESP-Arduino development on VScode
  * support M17 mref reflector
  * support noise cancellation
  * support AGC
  * doesn't use an audio codec.  Uses opamp circuits for audio filtering
* [Turn your Android phone into a modern ham radio transceiver](https://kv4p.com)
* [SHARI construction manual](https://wiki.fm-funknetz.de/lib/exe/fetch.php?media=fm-funknetz:technik:shari_construction_manual_v1-07.pdf)
  * Pairs a USB audio codec with a DRA818V module.
  * Includes a LPF for the antenna output.