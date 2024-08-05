/**
 * @file main.cpp
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2024-07-30
 *
 * @copyright Copyright (c) 2024
 * @note
 * 	- When not adding a synchronization symbol, before every packet, the Es/N0 will drop at each packet, resulting in a packet loss after a few packets.
 */

#include <Arduino.h>
#include "encode.hh"
#include "decode.hh"
#include "modem_config.hh"
#include <queue>

typedef float value;
typedef DSP::Complex<value> cmplx;

static Encoder<value, cmplx, 8000> *encoder = nullptr;
static Decoder<value, cmplx, 8000> *decoder = nullptr;
static int16_t *outputBuffer = nullptr;
static const char *TAG = "main";
std::queue<int16_t> sampleQueue;

void sampleSink(int16_t samples[], int count)
{
	// ESP_LOGI(TAG, "sampleSink: %d", count);
	for (int i = 0; i < count; i++)
	{
		sampleQueue.push(samples[i]);
	}
	// ESP_LOGI(TAG, "sampleQueue: %d", sampleQueue.size());
}

bool sampleSource(int16_t *sample)
{
	if (sampleQueue.empty())
	{
		*sample = 0; // Dummy value
		return false;
	}
	*sample = sampleQueue.front();
	sampleQueue.pop();
	return true;
}

void setup()
{
	// Print flash size
	ESP_LOGI(TAG, "Flash size: %d bytes.", ESP.getFlashChipSize());
	ESP_LOGI(TAG, "Total RAM size: %lu bytes", heap_caps_get_total_size(MALLOC_CAP_8BIT));
	ESP_LOGI(TAG, "Free RAM size: %lu", heap_caps_get_free_size(MALLOC_CAP_8BIT));
	pinMode(22, OUTPUT);
	encoder = new Encoder<value, cmplx, 8000>();
	decoder = new Decoder<value, cmplx, 8000>();
	encoder->setSampleSink(sampleSink);
	decoder->setSampleSource(sampleSource);
	uint64_t call_sign = 1, rx_call_sign = 0;
	const modem_config_t *modem_config = &modem_configs[11];
	encoder->configure(1600, call_sign, modem_config);
	outputBuffer = new int16_t[encoder->getSymbolLen() + encoder->getGuardLen()];

	uint8_t msg[] = "No one would have believed in the last years of the nineteenth century that this world was being watched keenly and \
	closely by intelligences greater than man’s and yet as mortal as his own; that as men busied themselves about their various concerns \
	they were scrutinised and studied, perhaps almost as narrowly as a man with a microscope might scrutinise the transient creatures that \
	swarm and multiply in a drop of water. With infinite complacency men went to and fro over this globe about their little affairs, serene \
	in their assurance of their empire over matter. It is possible that the infusoria under the microscope do the same. No one gave a thought \
	to the older worlds of space as sources of human danger, or thought of them only to dismiss the idea of life upon them as impossible or \
	improbable. It is curious to recall some of the mental habits of those departed days. At most terrestrial men fancied there might be other \
	men upon Mars, perhaps inferior to themselves and ready to welcome a missionary enterprise. Yet across the gulf of space, minds that are to \
	our minds as ours are to those of the beasts that perish, intellects vast and cool and unsympathetic, regarded this earth with envious \
	eyes, and slowly and surely drew their plans against us. And early in the twentieth century came the great disillusionment.";

	ESP_LOGI(TAG, "Used RAM size: %lu", heap_caps_get_total_size(MALLOC_CAP_8BIT) - heap_caps_get_free_size(MALLOC_CAP_8BIT));
	uint32_t startTime = millis();
	// ESP_LOGI(TAG, "Creating pilot block");
	// encoder->noise_block();
	// ESP_LOGI(TAG, "Creating correlator block");

	// ESP_LOGI(TAG, "Creating metadata block");

	// encoder->noise_block();
	//  Payload
	ESP_LOGI(TAG, "Creating payload block");
	size_t packet_size = (1 << (modem_config->code_order - 4)) - 1;
	for (uint8_t *ptr = msg; ptr < msg + sizeof(msg); ptr += packet_size)
	{
		encoder->synchronization_symbol();
		encoder->metadata_symbol(call_sign);
		encoder->data_packet(ptr, packet_size);
	}
	// End of the transmission
	// ESP_LOGI(TAG, "Creating tail block");
	// encoder->silence_packet();
	// ESP_LOGI(TAG, "Time to build a packet: %d ms", millis() - startTime);

	// Start of the reception
	
	uint8_t *dec_msg = nullptr;
	int len;
	startTime = millis();
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
		ESP_LOGI(TAG, "Time to receive a packet: %d ms", millis() - startTime);
		startTime = millis();
	}
	
}

void loop()
{
	digitalWrite(22, HIGH);
	delay(1000);
	digitalWrite(22, LOW);
	delay(1000);
}

void base37_decoder(char *str, long long int val, int len)
{
	for (int i = len - 1; i >= 0; --i, val /= 37)
		str[i] = " 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[val % 37];
}

long long int base37_encoder(const char *str)
{
	long long int acc = 0;
	for (char c = *str++; c; c = *str++)
	{
		acc *= 37;
		if (c >= '0' && c <= '9')
			acc += c - '0' + 1;
		else if (c >= 'a' && c <= 'z')
			acc += c - 'a' + 11;
		else if (c >= 'A' && c <= 'Z')
			acc += c - 'A' + 11;
		else if (c != ' ')
			return -1;
	}
	return acc;
}