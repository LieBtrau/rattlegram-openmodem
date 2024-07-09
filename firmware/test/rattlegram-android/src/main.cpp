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
	ESP_LOGI(TAG, "\r\nBuild %s, %s %s\r\n", AUTO_VERSION, __DATE__, __TIME__);
	encoder = new (std::nothrow) Encoder<8000>();
	int channelCount = 1;
	int symbolLength = (1280 * outputRate) / 8000;
	int guardLength = symbolLength / 8;
	int extendedLength = symbolLength + guardLength;
	outputBuffer = new short[extendedLength * channelCount];

	uint8_t* mesg = new uint8_t[170];
	int8_t* callSign = new int8_t[9];
	encoder->configure(mesg, callSign, carrierFrequency, noiseSymbols, fancyHeader);
	for (int i = 0; i < 5; ++i)
	{
		encoder->produce(outputBuffer, outputChannel);
	}
	delete outputBuffer;
	delete encoder;
}

void loop()
{
	// put your main code here, to run repeatedly:
}