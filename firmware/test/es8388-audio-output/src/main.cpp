/**
 * Output a 200Hz sine wave out on the left channel of the analog line-out of the ESP32-A1S
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
#include "SinWaveGenerator.h"
#include "ES8388Output.h"
#include "ES8388.h"

static const char *TAG = "main";
// i2s pins
static ES8388Output *output;
static SampleSource *sampleSource;
// i2c control
static ES8388 audioShield(33, 32);
static QueueHandle_t xQueue;
static i2s_pin_config_t i2s_pin_config =
	{
		.bck_io_num = 27,	// Serial Clock (SCK)
		.ws_io_num = 25,	// Word Select (WS)
		.data_out_num = 26, // data out to audio codec
		.data_in_num = 35	// data from audio codec
};

typedef enum audio_mode_t
{
	ANALOG_LOOPBACK,
	I2S_OUTPUT
} audio_mode_t;
static audio_mode_t mode = I2S_OUTPUT;

void vSenderTask(void *pvParameters)
{
	SampleSource *source = (SampleSource *)pvParameters;
	Frame_t samples[source->getFrameSampleCount()];

	for (;;)
	{
		do
		{
			source->getFrames(samples, source->getFrameSampleCount());
		} while (xQueueSendToBack(xQueue, &samples, portMAX_DELAY) == pdTRUE);
		taskYIELD();
	}
}

void setup()
{
	ESP_LOGI(TAG, "Build %s, %s %s\r\n", AUTO_VERSION, __DATE__, __TIME__);

	// init ES8388
	if (!audioShield.init())
	{
		ESP_LOGE(TAG, "ES8388 not found");
	}

	switch (mode)
	{
	case ANALOG_LOOPBACK:
		/**
		 * Analog audio from the LINEIN will be looped back to the EARPHONE-jack.
		 * 
		 * Connect laptop to LINEIN of ESP32-A1S.
		 * Connect headphones to EARPHONE-jack of ESP32-A1S.
		 * Alternatively, connect EARPHONE-jack to laptop (be careful with cable-splitters).  
		 * My splitter shorts the left and right output channel to the single microphone input of the laptop.
		 */
		audioShield.inputSelect(IN2);					// stereo LIN2/RIN2 (LINEIN on ESP32-A1S)
		audioShield.setInputGain(8);					// Micamp gain
		audioShield.outputSelect(ES8388::OutSel::OUT2); // EARPHONE-jack on ESP32-A1S
		audioShield.setOutputVolume(ES8388::OutSel::OUT2, 30);
		audioShield.mixerSourceSelect(MIXIN2, MIXIN2); // Select LIN and RIN source
		audioShield.mixerSourceControl(SRCSELOUT);	   // Use LIN and RIN as output
		break;
	case I2S_OUTPUT:
		/** 
		 * Output a 100Hz sine wave out on the left channel of the analog line-out of the ESP32-A1S
		 * 
		 * Connect headphones to EARPHONE-jack of ESP32-A1S.
		 * Alternatively, connect EARPHONE-jack to laptop (be careful with cable-splitters).  
		 * My splitter shorts the left and right output channel to the single microphone input of the laptop.
		*/
		audioShield.outputSelect(ES8388::OutSel::OUT2); // EARPHONE-jack on ESP32-A1S
		audioShield.setOutputVolume(ES8388::OutSel::OUT2, 30);
		audioShield.mixerSourceControl(DACOUT);	   // Use LIN and RIN as output
		sampleSource = new SinWaveGenerator(8000, 100, 32000);
		xQueue = xQueueCreate(3, sizeof(Frame_t) * sampleSource->getFrameSampleCount());
		if (xQueue == NULL)
		{
			ESP_LOGE(TAG, "Can't create queue");
		}
		ESP_LOGI(TAG, "Starting I2S Output");
		output = new ES8388Output(I2S_NUM_0, &i2s_pin_config);
		// init needed here to generate MCLK
		output->start(sampleSource, xQueue);
		TaskHandle_t writerTaskHandle;
		xTaskCreate(vSenderTask, "Sender1", 8192, (void *)sampleSource, 2, &writerTaskHandle);
		break;
	default:
		break;
	}

	ESP_LOGI(TAG, "Setup complete");
}

void loop()
{
	// nothing to do here - everything is taken care of by tasks
}
