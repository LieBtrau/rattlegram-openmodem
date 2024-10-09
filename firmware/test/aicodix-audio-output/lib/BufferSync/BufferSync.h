// /**
//  * @file BufferSync.h
//  * @author Christoph Tack
//  * @brief This class provides a way for passing messages of varying size between two or more tasks.
//  * Copying byte per byte into a queue is too slow for large messages.
//  * FreeRTOS provides StreamBuffers and MessageBuffers, but they are not available in the ESP32 Arduino environment.
//  * https://forums.freertos.org/t/queue-tactics-for-large-memory-objects/15233/10 suggests to use an array of pointers to the data and two queues.
//  * 
//  * I think that the array of pointers is not necessary.  And a single queue is sufficient.
//  * 
//  * The approach here is to create an object that holds a pointer to the data and the size of the data.  
//  * These objects are then passed between tasks using a queue.  The number of objects is limited by the size of the queue.
//  * The sending task copies data into a buffer and a pointer to the buffer and its size is pushed to the queue.
//  * The receiving task then pops the pointer to the buffer from the queue and processes the data.  When the data is no longer needed, the buffer is freed.
//  * The queue has an empty place now, and the next data can be sent.
//  * @version 0.1
//  * @date 2024-10-05
//  * 
//  * @copyright Copyright (c) 2024
//  * 
//  */
// #pragma once

// #include <Arduino.h>

// struct BufferSyncMessage
// {
//     uint8_t *data;
//     std::size_t size;
// };

// class BufferSync
// {
// private:
//     xQueueHandle m_queue;
// public:
//     BufferSync(std::size_t maxMessages);
//     ~BufferSync();
//     bool send(void *data, std::size_t size);
//     bool receive(BufferSyncMessage *message);
// };
