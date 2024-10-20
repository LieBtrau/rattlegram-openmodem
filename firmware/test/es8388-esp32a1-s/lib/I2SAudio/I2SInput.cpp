#include <Arduino.h>
#include "I2SInput.h"
#include "driver/i2s.h"
#include "I2SAudio.h"

static bool wanttoStopSampling = false;
static TaskHandle_t xTaskToNotify = NULL;

void i2sReaderTask(void *param)
{
    I2SInput *sampler = (I2SInput *)param;
    for (;;)
    {
        if (wanttoStopSampling)
        {
            xTaskNotifyGive(xTaskToNotify);
            vTaskSuspend(NULL);
        }
        else
        {
            BufferSyncMessage message;
            ESP_ERROR_CHECK(i2s_read(sampler->m_i2sPort, message.data, sizeof(message.data), &message.size, portMAX_DELAY));
            xQueueSend(sampler->m_sample_source, &message, portMAX_DELAY);
        }
        taskYIELD();
    }
}

void I2SInput::start(xQueueHandle sample_source)
{
    m_sample_source = sample_source;
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
