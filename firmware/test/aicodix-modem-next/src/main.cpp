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
		*sample = 0;
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
	ESP_LOGI(TAG, "Free RAM size: %lu",  heap_caps_get_free_size(MALLOC_CAP_8BIT));
	pinMode(22, OUTPUT);
	encoder = new Encoder<value, cmplx, 8000>();
	decoder = new Decoder<value, cmplx, 8000>();
	encoder->setSampleSink(sampleSink);
	decoder->setSampleSource(sampleSource);
	uint64_t call_sign = 1, rx_call_sign = 0;
	const modem_config_t *modem_config = &modem_configs[1];
	encoder->configure(1600, call_sign, modem_config);
	outputBuffer = new int16_t[encoder->getSymbolLen() + encoder->getGuardLen()];

	uint8_t msg1[] = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut \
		labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip \
		ex ea commodo consequat. Duis aute irure ";

	uint8_t msg2[] = "dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat \
		nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.";

	ESP_LOGI(TAG, "Used RAM size: %lu",  heap_caps_get_total_size(MALLOC_CAP_8BIT) - heap_caps_get_free_size(MALLOC_CAP_8BIT));
		uint32_t startTime = millis();
		// ESP_LOGI(TAG, "Creating pilot block");
		//encoder->pilot_block();
		// ESP_LOGI(TAG, "Creating correlator block");
		encoder->schmidl_cox();
		// ESP_LOGI(TAG, "Creating metadata block");
		encoder->meta_data((call_sign << 8) | modem_config->oper_mode);
		//encoder->pilot_block();
		// Payload
		ESP_LOGI(TAG, "Creating payload block");
		if (!encoder->addPacket(msg1, sizeof(msg1)))
		{
			ESP_LOGE(TAG, "Failed to add packet");
			return;
		}
		if (!encoder->addPacket(msg2, sizeof(msg2)))
		{
			ESP_LOGE(TAG, "Failed to add packet");
			return;
		}
		// End of the transmission
		// ESP_LOGI(TAG, "Creating tail block");
		//encoder->tail_block();
		// ESP_LOGI(TAG, "Time to build a packet: %d ms", millis() - startTime);

		// Start of the reception
		startTime = millis();
		if (!decoder->synchronize())
		{
			ESP_LOGE(TAG, "Feed failed");
			return;
		}
		ESP_LOGI(TAG, "Feed successful");
		if (!decoder->preamble())
		{
			ESP_LOGE(TAG, "Preamble not detected");
			return;
		}
		ESP_LOGI(TAG, "Preamble detected");
		if (!decoder->meta_data(rx_call_sign))
		{
			ESP_LOGE(TAG, "Metadata not detected");
			return;
		}
		ESP_LOGI(TAG, "Metadata: %lu", rx_call_sign);
		uint8_t *dec_msg = nullptr;
		int len;
		for(int i=0; i<2; i++)
		{
			if (!decoder->demodulate())
			{
				ESP_LOGE(TAG, "Demodulation failed");
				return;
			}
			if (decoder->decode(&dec_msg, len))
			{
				ESP_LOGI(TAG, "Message: %s", dec_msg);
				ESP_LOGI(TAG, "Time to receive a packet: %d ms", millis() - startTime);
			}
			else
			{
				ESP_LOGE(TAG, "Message not detected");
			}
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
	for (int i = len-1; i >= 0; --i, val /= 37)
		str[i] = " 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[val%37];
}

long long int base37_encoder(const char *str)
{
	long long int acc = 0;
	for (char c = *str++; c; c = *str++) {
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