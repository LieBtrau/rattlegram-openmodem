#include <Arduino.h>
#include "I2SInput.h"
#include "driver/i2s.h"
#include "I2SAudio.h"

static bool wanttoStopSampling = false;
static TaskHandle_t xTaskToNotify = NULL;

void i2sReaderTask(void *param)
{
    I2SInput *sampler = (I2SInput *)param;
    size_t bytesRead = 0;
    uint8_t *i2sData;
    for (;;)
    {
        if (wanttoStopSampling)
        {
            xTaskNotifyGive(xTaskToNotify);
            vTaskSuspend(NULL);
        }
        else
        {
            size_t bytecount = sampler->m_sampleCount * 2;
            i2sData = new uint8_t[bytecount];
            if (i2sData!=nullptr && i2s_read(sampler->m_i2sPort, i2sData, bytecount, &bytesRead, portMAX_DELAY) == ESP_OK)
            {
                BufferSyncMessage message;
                message.data = i2sData;
                message.size = bytesRead;
                xQueueSend(sampler->m_sample_source, &message, portMAX_DELAY);
            }
        }
        taskYIELD();
    }
}

void I2SInput::start(xQueueHandle sample_source, size_t maxSamples)
{
    m_sample_source = sample_source;
    m_sampleCount = maxSamples;
    if (m_i2s_readerTaskHandle == NULL)
    {
        xTaskCreate(i2sReaderTask, "i2s Reader Task", 4096, this, 1, &m_i2s_readerTaskHandle);
    }
    vTaskDelay(1);
    vTaskResume(m_i2s_readerTaskHandle);
    xTaskToNotify = xTaskGetCurrentTaskHandle();
    wanttoStopSampling = false;
}

void I2SInput::stop()
{
    wanttoStopSampling = true;
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
}
