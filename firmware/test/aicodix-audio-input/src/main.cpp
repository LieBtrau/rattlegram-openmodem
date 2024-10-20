/**
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
#include "decode.hh"
#include "I2SAudio.h"
#include "ES8388.h"

static ES8388 audioShield(33, 32);
static const char *TAG = "main";
static const uint32_t PIN_LED = 22;

typedef float value;
typedef DSP::Complex<value> cmplx;
static const int SAMPLE_RATE = 8000;
static Decoder<value, cmplx, 8000> *decoder = nullptr;
static I2SAudio *i2sAudio;

bool sampleSource(int16_t *sample)
{
	size_t sample_count = 1;
	return i2sAudio->getSourceSamples(sample, sample_count, I2SAudio::AudioChannel::LEFT);
}

void setup()
{
	ESP_LOGI(TAG, "Build %s, %s %s\r\n", AUTO_VERSION, __DATE__, __TIME__);
	pinMode(PIN_LED, OUTPUT);

	// init ES8388
	if (!audioShield.init())
	{
		ESP_LOGE(TAG, "ES8388 not found");
	}
	audioShield.outputSelect(ES8388::OutSel::OUT2); // EARPHONE-jack on ESP32-A1S
	audioShield.setOutputVolume(ES8388::OutSel::OUT2, 30);
	audioShield.mixerSourceControl(DACOUT); // Use LIN and RIN as output

	i2sAudio = new I2SAudio(SAMPLE_RATE, 27, 25, 26, 35);
	i2sAudio->init();
	i2sAudio->start_input(16);

	int config_index = 4; // operating mode 23
	decoder = new Decoder<value, cmplx, 8000>();
	decoder->setSampleSource(sampleSource);

	ESP_LOGI(TAG, "Setup complete");
}

void loop()
{
	uint64_t rx_call_sign = 0;
	uint8_t *dec_msg = nullptr;
	int len;

	while (decoder->synchronization_symbol())
	{
		if (!decoder->metadata_symbol(rx_call_sign))
		{
			ESP_LOGE(TAG, "Metadata not detected");
			return;
		}
		ESP_LOGI(TAG, "Metadata: %lu", rx_call_sign);
		if (decoder->data_packet(&dec_msg, len))
		{
			dec_msg[len - 1] = '\0';
			ESP_LOGI(TAG, "Message: %s, length: %d", dec_msg, len);
		}
	}
	ESP_LOGI(TAG, "End of loop");
}