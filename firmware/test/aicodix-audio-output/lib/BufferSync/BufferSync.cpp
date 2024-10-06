#include "BufferSync.h"

BufferSync::BufferSync(std::size_t maxMessages)
{
    m_queue = xQueueCreate(maxMessages, sizeof(BufferSyncMessage));
}

BufferSync::~BufferSync()
{
    vQueueDelete(m_queue);
}

/**
 * @brief Add a message to the queue.  This function is blocking.
 * 
 * @param data 
 * @param size 
 * @return true 
 * @return false 
 */
bool BufferSync::send(void *data, std::size_t size)
{
    BufferSyncMessage message;
    message.data = new uint8_t[size];
    if (message.data == nullptr)
    {
        return false;
    }
    memcpy(message.data, data, size);
    message.size = size;
    return xQueueSend(m_queue, &message, portMAX_DELAY) == pdTRUE;
}

/**
 * @brief Receive a message from the queue.  This function is blocking.  Free the data when done.
 * 
 * @param message 
 * @return true 
 * @return false 
 */
bool BufferSync::receive(BufferSyncMessage *message)
{
    return xQueueReceive(m_queue, message, portMAX_DELAY) == pdTRUE;
}