#include "led.hpp"

#include "numpad.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

Led::Led(NumPad& numpad)
    : numpad_(numpad)
{
}

void Led::initGpio()
{
    gpio_reset_pin(PIN_);
    gpio_set_direction(PIN_, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_, 0);
}

void Led::blink(int times)
{
    for (int i = 0; i < times; i++) {

        gpio_set_level(PIN_, 1);

        vTaskDelay(pdMS_TO_TICKS(250));

        gpio_set_level(PIN_, 0);

        vTaskDelay(pdMS_TO_TICKS(250));
    }
}

void Led::run()
{
    initGpio();

    int key_a = 0;
    int key_b = 0;
    bool is_addition_pressed = false;

    while (true) {

        numpad_.receiveKey(key_a, portMAX_DELAY);

        if (key_a == -1) {
            is_addition_pressed = true;
        }
        else if (is_addition_pressed && key_b > 0 && key_a > 0) {

            blink(key_a + key_b);

            key_a = 0;
            key_b = 0;
            is_addition_pressed = false;
        }
        else if (key_a > 0) {
            key_b = key_a;
            key_a = 0;
        }
    }
}
