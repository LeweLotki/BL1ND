#include "standard_output.hpp"

#include "freertos/task.h"

#include <cstdio>
#include <cstdarg>
#include <cstring>

StandardOutput::StandardOutput()
    : message_queue_(xQueueCreate(QUEUE_LENGTH, MESSAGE_SIZE))
{
}

void StandardOutput::print(const char* text)
{
    char message[MESSAGE_SIZE];
    strncpy(message, text, MESSAGE_SIZE - 1);
    message[MESSAGE_SIZE - 1] = '\0';

    xQueueSend(message_queue_, message, 0);
}

void StandardOutput::printf(const char* fmt, ...)
{
    char message[MESSAGE_SIZE];

    va_list args;
    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);

    print(message);
}

void StandardOutput::run()
{
    char message[MESSAGE_SIZE];

    while (true) {

        if (xQueueReceive(message_queue_, message, portMAX_DELAY) == pdTRUE) {
            ::printf("%s", message);
        }
    }
}
