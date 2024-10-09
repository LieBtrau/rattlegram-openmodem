// #include <Arduino.h>
// #include "I2SInput.h"
// #include "driver/i2s.h"

// static bool wanttoStopSampling = false;
// static TaskHandle_t xTaskToNotify = NULL;

// void i2sReaderTask(void *param)
// {
//     I2SInput *sampler = (I2SInput *)param;
//     size_t bytesRead = 0;
//     const int I2S_IN_SAMPLE_COUNT = 64;
//     int16_t *i2sData;
//     for (;;)
//     {
//         if (wanttoStopSampling)
//         {
//             xTaskNotifyGive(xTaskToNotify);
//             vTaskSuspend(NULL);
//         }
//         else
//         {
//             i2sData = new int16_t[I2S_IN_SAMPLE_COUNT];
//             if (i2sData!=nullptr && i2s_read(sampler->m_i2sPort, i2sData, I2S_IN_SAMPLE_COUNT << 1, &bytesRead, portMAX_DELAY) == ESP_OK)
//             {
//                 BufferSyncMessage message;
//                 message.data = reinterpret_cast<uint8_t*>(i2sData);
//                 message.size = bytesRead;
//                 sampler->m_sample_source->send(&message);
//             }
//         }
//         taskYIELD();
//     }
// }

// void I2SInput::start(BufferSync *sample_source)
// {
//     m_sample_source = sample_source;
//     if (m_i2s_readerTaskHandle == NULL)
//     {
//         xTaskCreate(i2sReaderTask, "i2s Reader Task", 4096, this, 1, &m_i2s_readerTaskHandle);
//     }
//     vTaskDelay(1);
//     vTaskResume(m_i2s_readerTaskHandle);
//     xTaskToNotify = xTaskGetCurrentTaskHandle();
//     wanttoStopSampling = false;
// }

// void I2SInput::stop()
// {
//     wanttoStopSampling = true;
//     ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
// }