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
    unsigned int cycles = 1;
    unsigned int on_ms = 250;
    unsigned int off_ms = 250;
    switch (pattern) {
    case BlinkPattern::Single:
        break;
    case BlinkPattern::Error:
        cycles = 3;
        on_ms = 80;
        off_ms = 80;
        break;
    case BlinkPattern::Linked:
        cycles = 5;
        on_ms = 120;
        off_ms = 120;
        break;
    case BlinkPattern::ColorWhite:
        on_ms = 700;
        off_ms = 400;
        break;
    case BlinkPattern::ColorBlack:
        cycles = 2;
        on_ms = 700;
        off_ms = 400;
        break;
    }
    for (unsigned int cycle = 0; cycle < cycles; ++cycle) {
        gpio_set_level(PIN_, 1);
        vTaskDelay(pdMS_TO_TICKS(on_ms));
        gpio_set_level(PIN_, 0);
        vTaskDelay(pdMS_TO_TICKS(off_ms));
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

bool Led::blinkLinked()
{
    const BlinkPattern command = BlinkPattern::Linked;
    return xQueueSend(command_queue_, &command, portMAX_DELAY) == pdTRUE;
}

bool Led::announceColor(Color color)
{
    const BlinkPattern command = color == Color::White
        ? BlinkPattern::ColorWhite
        : BlinkPattern::ColorBlack;
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
