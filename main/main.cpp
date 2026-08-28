#include "numpad.hpp"

#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

constexpr gpio_num_t LED_PIN = GPIO_NUM_2;

void led_init()
{
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_PIN, 0);
}

void led_task(void* parameter)
{
    int key_a = 0;
    int key_b = 0;
    bool is_addition_pressed = false;

    while (true) {

        // Czekamy, aż użytkownik coś naciśnie
        numpad_receive_key(key_a, portMAX_DELAY);

        if (key_a == -1) {
            is_addition_pressed = true;
        }
        else if (is_addition_pressed && key_b > 0 && key_a > 0) {

            int sum = key_a + key_b;

            // blink led
            for (int i = 0; i < sum; i++) {

                gpio_set_level(LED_PIN, 1);

                vTaskDelay(pdMS_TO_TICKS(250));

                gpio_set_level(LED_PIN, 0);

                vTaskDelay(pdMS_TO_TICKS(250));
            }
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

extern "C" void app_main()
{
    led_init();
    keypad_init();
    keypad_start();

    xTaskCreate(
        led_task,
        "led_task",
        2048,
        nullptr,
        4,
        nullptr
    );
}
