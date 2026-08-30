#include "led.hpp"

#include "freertos/task.h"

Led::Led()
    : command_queue_(xQueueCreate(8, sizeof(uint8_t)))
{
}

void Led::initGpio()
{
    gpio_reset_pin(PIN_);
    gpio_set_direction(PIN_, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_, 0);
}

void Led::blink()
{
    gpio_set_level(PIN_, 1);
    vTaskDelay(pdMS_TO_TICKS(250));
    gpio_set_level(PIN_, 0);
    vTaskDelay(pdMS_TO_TICKS(250));
}

bool Led::blinkOnce()
{
    const uint8_t command = 1;
    return xQueueSend(command_queue_, &command, portMAX_DELAY) == pdTRUE;
}

void Led::run()
{
    initGpio();

    while (true) {
        uint8_t command = 0;
        if (xQueueReceive(command_queue_, &command, portMAX_DELAY) == pdTRUE) {
            blink();
        }
    }
}
