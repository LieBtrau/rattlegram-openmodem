/**
 * Output a sine wave out on the left channel of the analog line-out of the ESP32-A1S
 *
 * Maybe helpful for debugging:
 *  http://www.iotsharing.com/2017/07/how-to-use-arduino-esp32-i2s-to-play-wav-music-from-sdcard.html
 *  https://diyi0t.com/i2s-sound-tutorial-for-esp32/
 *  https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/i2s.html
 *
 * Signal name                              	ESP32A1-S
 * 3V3
 * GND
 *
 * MCLK (Audio Master Clock)                	GPIO0
 *
 * LRCLK                                    	GPIO25
 *      Audio Left/Right Clock
 *      WS (Word Select)
 *      48kHz square wave
 * BCLK                                     	GPIO27
 *      SCLK (Audio Bit Clock)
 *      1.54MHz
 * DOUT                                     	GPIO35
 *      Audio Data from Audio Shield to MCU
 * DIN                                      	GPIO26
 *      Audio Data from MCU to Audio Shield
 * SDA                                      	GPIO33
 * SCL                                      	GPIO32
 */

#include <Arduino.h>
#include "ES8388.h"

static const char *TAG = "main";

// i2c control
static ES8388 audioShield(33, 32);
void setup_analog_loopback();

void setup()
{
	ESP_LOGI(TAG, "Build %s, %s %s\r\n", AUTO_VERSION, __DATE__, __TIME__);

	// init ES8388
	if (!audioShield.init())
	{
		ESP_LOGE(TAG, "ES8388 not found");
	}
	setup_analog_loopback();
	ESP_LOGI(TAG, "Setup complete");
}

void loop()
{
	// nothing to do here - everything is taken care of by tasks
}

/**
 * ESP32-A1S : Analog audio from the LINEIN will be looped back to the EARPHONE-jack.
 * 
 * 1. Power the ESP32-A1S with using the BAT connector from a 3.7V LiPo battery or lab power supply.  Powering from USB causes a lot of noise because of ground loops.
 * 2. Connect laptop to LINEIN of ESP32-A1S.
 * 3. Connect headphones to EARPHONE-jack of ESP32-A1S.
 * Alternatively, connect laptop to EARPHONE-jack (be careful with cable-splitters).  
 * My splitter shorts the left and right output channel to the single microphone input of the laptop.
 */
void setup_analog_loopback()
{
	audioShield.inputSelect(IN2);					// stereo LIN2/RIN2 (LINEIN on ESP32-A1S)
	audioShield.setInputGain(8);					// Micamp gain
	audioShield.outputSelect(ES8388::OutSel::OUT2); // EARPHONE-jack on ESP32-A1S
	audioShield.setOutputVolume(ES8388::OutSel::OUT2, 30);
	audioShield.mixerSourceSelect(MIXIN2, MIXIN2); // Select LIN and RIN source
	audioShield.mixerSourceControl(SRCSELOUT);	   // Use LIN and RIN as output
}