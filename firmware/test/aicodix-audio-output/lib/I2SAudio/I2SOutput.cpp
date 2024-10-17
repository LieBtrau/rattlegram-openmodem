#include <Arduino.h>
#include "driver/i2s.h"
#include "I2SOutput.h"
#include "I2SAudio.h"

static bool wanttoStopEncoding = false;
static TaskHandle_t xTaskToNotify = NULL;

void i2sWriterTask(void *param)
{
    I2SOutput *output = (I2SOutput *)param;
    size_t bytesWritten = 0;
    for (;;)
    {
        if (wanttoStopEncoding)
        {
            xTaskNotifyGive(xTaskToNotify);
            vTaskSuspend(NULL);
        }
        else
        {
            BufferSyncMessage message;
            // Check if there is a message in the queue to be sent to the I2S peripheral
            if(xQueueReceive(output->m_sample_sink, &message, portMAX_DELAY) == pdTRUE);
            {
                // write data to the i2s peripheral
                //Wait portMAX_DELAY for the I2S peripheral to become available, so bytes written will always be equal to message.size
                ESP_ERROR_CHECK(i2s_write(output->m_i2sPort, message.data, message.size, &bytesWritten, portMAX_DELAY));
            }
        }
        taskYIELD();
    }
}

void I2SOutput::start(xQueueHandle sample_sink)
{
    m_sample_sink = sample_sink;
    if (m_i2s_writerTaskHandle == NULL)
    {
        xTaskCreate(i2sWriterTask, "i2s Writer Task", 4096, this, 1, &m_i2s_writerTaskHandle);
    }
    vTaskDelay(1);
    vTaskResume(m_i2s_writerTaskHandle);
    xTaskToNotify = xTaskGetCurrentTaskHandle();
    wanttoStopEncoding = false;
}

void I2SOutput::stop()
{
    wanttoStopEncoding = true;
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
}