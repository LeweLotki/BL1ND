#include "led.hpp"

#include "freertos/task.h"

Led::Led()
    : command_queue_(xQueueCreate(8, sizeof(BlinkPattern)))
{
}

void Led::initGpio()
{
    gpio_reset_pin(PIN_);
    gpio_set_direction(PIN_, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_, 0);
}

void Led::blink(BlinkPattern pattern)
{
    const unsigned int cycles = pattern == BlinkPattern::Single ? 1 : 3;
    const TickType_t delay = pdMS_TO_TICKS(
        pattern == BlinkPattern::Single ? 250 : 80
    );
    for (unsigned int cycle = 0; cycle < cycles; ++cycle) {
        gpio_set_level(PIN_, 1);
        vTaskDelay(delay);
        gpio_set_level(PIN_, 0);
        vTaskDelay(delay);
    }
}

bool Led::blinkOnce()
{
    const BlinkPattern command = BlinkPattern::Single;
    return xQueueSend(command_queue_, &command, portMAX_DELAY) == pdTRUE;
}

bool Led::blinkError()
{
    const BlinkPattern command = BlinkPattern::Error;
    return xQueueSend(command_queue_, &command, portMAX_DELAY) == pdTRUE;
}

void Led::run()
{
    initGpio();

    while (true) {
        BlinkPattern command = BlinkPattern::Single;
        if (xQueueReceive(command_queue_, &command, portMAX_DELAY) == pdTRUE) {
            blink(command);
        }
    }
}
