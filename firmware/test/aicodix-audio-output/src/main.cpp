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
#include "encode.hh"
#include "BufferSync.h"
#include "I2SAudio/I2SAudio.h"
#include "ES8388.h"

static ES8388 audioShield(33, 32);
static const char *TAG = "main";
static const uint32_t PIN_LED = 22;

typedef float value;
typedef DSP::Complex<value> cmplx;
static const int SAMPLE_RATE = 8000;
Encoder<value, cmplx, SAMPLE_RATE> *encoder = nullptr;
BufferSync *bufferSync = nullptr;
static I2SAudio *i2sAudio;

void sampleSink(int16_t samples[], int count)
{
	int16_t frames[2*count];
	for (int i = 0; i < count; i++)
	{
		frames[2*i] = samples[i];	// left channel
		frames[2*i+1] = 0;			// right channel
	}
	bufferSync->send(frames, sizeof(frames));
}

void setup()
{
	ESP_LOGI(TAG, "Build %s, %s %s\r\n", AUTO_VERSION, __DATE__, __TIME__);
	pinMode(PIN_LED, OUTPUT);

	bufferSync = new BufferSync(16);
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
	i2sAudio->start_output(bufferSync);

	int config_index = 4; // operating mode 23
	encoder = new Encoder<value, cmplx, SAMPLE_RATE>();
    encoder->configure(1600, &modem_configs[config_index]);
    encoder->setSampleSink(sampleSink);

	ESP_LOGI(TAG, "Setup complete");
}

void sendPacket(const char *msg, int len)
{
	// const uint8_t callsign[9] = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	// encoder->spectrogram_block(callsign);

	uint64_t call_sign = 0x12345678;
	int packet_size = encoder->getPacketSize();
	for (uint8_t *ptr = reinterpret_cast<uint8_t*>(const_cast<char*>(msg)); ptr < reinterpret_cast<uint8_t*>(const_cast<char*>(msg)) + sizeof(msg); ptr += packet_size)
	{
		encoder->synchronization_symbol();
		encoder->metadata_symbol(call_sign);
		encoder->data_packet(ptr, packet_size);
	}    
}

void loop()
{
	const char *msg = "Hello, World!";
	sendPacket(msg, strlen(msg));
	delay(5000);
}