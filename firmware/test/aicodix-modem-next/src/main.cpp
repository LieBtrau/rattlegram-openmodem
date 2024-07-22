#include <Arduino.h>
#include "encode.hh"
#include "decode.hh"
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
	ESP_LOGI(TAG, "sampleSink: %d", count);
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
	pinMode(22, OUTPUT);
	encoder = new Encoder<value, cmplx, 8000>();
	decoder = new Decoder<value, cmplx, 8000>();
	encoder->setSampleSink(sampleSink);
	decoder->setSampleSource(sampleSource);
	uint64_t call_sign = 1, rx_call_sign = 0;
	int oper_mode = 22;
	encoder->configure(1600, call_sign, oper_mode);
	outputBuffer = new int16_t[encoder->symbol_len + encoder->guard_len];

	uint32_t startTime = millis();
	ESP_LOGI(TAG, "Creating pilot block");
	encoder->pilot_block();
	ESP_LOGI(TAG, "Creating correlator block");
	encoder->schmidl_cox();
	ESP_LOGI(TAG, "Creating metadata block");
	encoder->meta_data((call_sign << 8) | oper_mode);
	// Payload
	uint8_t msg[] = {'h', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd', '!', '\0'};
	ESP_LOGI(TAG, "Creating payload block");
	encoder->addPacket(msg, sizeof(msg));
	// End of the transmission
	ESP_LOGI(TAG, "Creating tail block");
	encoder->tail_block();
	ESP_LOGI(TAG, "Time to build a packet: %d ms", millis() - startTime);

	// Start of the reception
	startTime = millis();
	if (!decoder->feed())
	{
		ESP_LOGI(TAG, "Feed failed");
		return;
	}
	ESP_LOGI(TAG, "Feed successful");
	if (!decoder->preamble())
	{
		ESP_LOGI(TAG, "Preamble not detected");
		return;
	}
	ESP_LOGI(TAG, "Preamble detected");
	if (!decoder->meta_data(rx_call_sign))
	{
		ESP_LOGI(TAG, "Metadata not detected");
		return;
	}
	ESP_LOGI(TAG, "Metadata: %lu", rx_call_sign);
	uint8_t *dec_msg = nullptr;
	int len;
	if (decoder->fetch(&dec_msg, len))
	{
		ESP_LOGI(TAG, "Message: %s", dec_msg);
	}
	else
	{
		ESP_LOGI(TAG, "Message not detected");
	}
	ESP_LOGI(TAG, "Time to receive a packet: %d ms", millis() - startTime);
}

void loop()
{
	digitalWrite(22, HIGH);
	delay(1000);
	digitalWrite(22, LOW);
	delay(1000);
}