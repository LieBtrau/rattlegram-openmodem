#include <Arduino.h>
#include "Rattlegram.h"
#include "SPIFFS.h"

static const char *TAG = "main";
void setup()
{
  ESP_LOGI(TAG, "\r\nBuild %s, %s %s\r\n", AUTO_VERSION, __DATE__, __TIME__);
  if (!SPIFFS.begin())
  {
    ESP_LOGE(TAG, "Cannot mount SPIFFS volume...be sure to upload Filesystem Image before uploading the sketch");
    while (1)
      ;
  }
  main_decode("/spiffs/encoded.wav", "/spiffs/output.txt");
}

void loop()
{
  // put your main code here, to run repeatedly:
}
