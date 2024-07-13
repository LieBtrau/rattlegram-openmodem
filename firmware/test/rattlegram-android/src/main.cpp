#include <Arduino.h>
#include "encoder.hh"
#include "decoder.hh"

static const char *TAG = "main";
static EncoderInterface *encoder;
static DecoderInterface *decoder;
static short *outputBuffer;
static const int outputRate = 8000;
static const int outputChannel = 0;
static int carrierFrequency = 1600;
static int noiseSymbols = 6;
static bool fancyHeader = false;

void setup()
{
	ESP_LOGI(TAG, "Build %s, %s %s\r\n", AUTO_VERSION, __DATE__, __TIME__);
	encoder = new (std::nothrow) Encoder<8000>();
	decoder = new (std::nothrow) Decoder<8000>();
	int channelCount = 1;
	int symbolLength = (1280 * outputRate) / 8000;
	int guardLength = symbolLength / 8;
	int extendedLength = symbolLength + guardLength;

	outputBuffer = new int16_t[extendedLength * channelCount];
	uint8_t mesg[] = {'h', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd', '!','\0'};
	byte *payload = new byte[170];
	float stagedCFO;
	int stagedMode;
	byte* stagedCall = new byte[10];
	memset(stagedCall, 0, 10);
	int result;
	const char call[] = "NOCALL";
	encoder->configure(mesg, (const int8_t*)call, carrierFrequency, noiseSymbols, fancyHeader);
	ESP_LOGI(TAG, "Configured encoder");
	bool encoder_busy = true;
	do
	{
		encoder_busy = encoder->produce(outputBuffer, outputChannel);
		if (!decoder->feed(outputBuffer, extendedLength, outputChannel))
		{
			continue;
		}
		switch (decoder->process())
		{
		case STATUS_OKAY:
			break;
		case STATUS_FAIL:
			ESP_LOGE(TAG, "Decoding failed");
			break;
		case STATUS_SYNC:
			ESP_LOGI(TAG, "Syncing");
			decoder->staged(&stagedCFO, &stagedMode, stagedCall);
			ESP_LOGI(TAG, "CFO: %f, Mode: %d, Call: %s", stagedCFO, stagedMode, stagedCall);
			break;
		case STATUS_DONE:
			result = decoder->fetch(payload);
			if (result >= 0)
			{
				ESP_LOGI(TAG, "Decoded success");
				ESP_LOGI(TAG, "Payload: %s", payload);
			}
			else
			{
				ESP_LOGE(TAG, "Decoding failed");
			}
			break;
		case STATUS_HEAP:
			ESP_LOGE(TAG, "Heap error");
			break;
		case STATUS_NOPE:
			ESP_LOGI(TAG, "Nope");
			decoder->staged(&stagedCFO, &stagedMode, stagedCall);
			break;
		case STATUS_PING:
			ESP_LOGI(TAG, "Ping");
			decoder->staged(&stagedCFO, &stagedMode, stagedCall);
			break;
		default:
			ESP_LOGE(TAG, "Unknown status");
			break;
		}
	}while(encoder_busy);
	delete payload;
	delete stagedCall;
	delete outputBuffer;
	delete encoder;
	delete decoder;
	pinMode(22, OUTPUT);
}

void loop()
{
	digitalWrite(22, HIGH);
	delay(1000);
	digitalWrite(22, LOW);
	delay(1000);
}