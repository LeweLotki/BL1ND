#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

class StandardOutput {
public:
    StandardOutput();

    void run();

    void print(const char* text);
    void printf(const char* fmt, ...);

private:
    static constexpr size_t MESSAGE_SIZE = 128;
    static constexpr size_t QUEUE_LENGTH = 8;

    QueueHandle_t message_queue_;
};
