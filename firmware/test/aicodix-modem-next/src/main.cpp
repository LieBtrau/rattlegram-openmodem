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
}

bool sampleSource(int16_t* sample)
{
  if (sampleQueue.empty())
  {
    return false;
  }
  *sample = sampleQueue.front();
  sampleQueue.pop();
  return true;
}

void setup()
{
  encoder = new Encoder<value, cmplx, 8000>();
  decoder = new Decoder<value, cmplx, 8000>();
  encoder->setSampleSink(sampleSink);
  decoder->setSampleSource(sampleSource);
  uint64_t call_sign = 1;
  int oper_mode = 22;
  encoder->configure(1600, call_sign, oper_mode);
  outputBuffer = new int16_t[encoder->symbol_len + encoder->guard_len];

  uint32_t startTime = millis();
  //Start of the transmission
  encoder->pilot_block();
  //Correlator
  encoder->schmidl_cox();
  //Metadata
  encoder->meta_data((call_sign << 8) | oper_mode);
  //Payload
  uint8_t msg[] = {'h', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd', '!','\0'};
  encoder->addPacket(msg, sizeof(msg));
  //End of the transmission
  encoder->tail_block();
  ESP_LOGI(TAG, "Time to build a packet: %d ms", millis() - startTime);

  //Start of the reception
  startTime = millis();
  while(!decoder->feed());
  if(decoder->preamble())
  {
    ESP_LOGI(TAG, "Preamble detected");
  }
  else
  {
    ESP_LOGI(TAG, "Preamble not detected");
  }
  ESP_LOGI(TAG, "Time to receive a packet: %d ms", millis() - startTime);
  pinMode(22, OUTPUT);
}

void loop()
{
	digitalWrite(22, HIGH);
	delay(1000);
	digitalWrite(22, LOW);
	delay(1000);
}