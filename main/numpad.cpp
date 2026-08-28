#include "numpad.hpp"

#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_rom_sys.h"

#include <cstdio>

constexpr gpio_num_t ROWS[4] = {
    GPIO_NUM_13,
    GPIO_NUM_12,
    GPIO_NUM_14,
    GPIO_NUM_27,
};

constexpr gpio_num_t COLS[4] = {
    GPIO_NUM_26,
    GPIO_NUM_25,
    GPIO_NUM_33,
    GPIO_NUM_32,
};

// Kolejka ma tylko 1 element.
// Zawsze interesuje nas najnowszy naciśnięty klawisz.
static QueueHandle_t key_queue;

static int keypad_read()
{
    for (int row = 0; row < 4; row++) {

        // Wszystkie wiersze HIGH
        for (int i = 0; i < 4; i++) {
            gpio_set_level(ROWS[i], 1);
        }

        // Aktualnie testowany wiersz LOW
        gpio_set_level(ROWS[row], 0);

        // Krótki czas na ustabilizowanie sygnału
        esp_rom_delay_us(5);

        for (int col = 0; col < 4; col++) {

            if (gpio_get_level(COLS[col]) == 0) {

                // row-major:
                //
                // row 0: 1  2  3  4
                // row 1: 5  6  7  8
                // row 2: 9 10 11 12
                // row 3:13 14 15 16

                if (col < 3 && row < 3) {
                    return col + 3 * row + 1;
                }
                else if (col == 3 && row == 0) {
                    return -1; // additon operator
                }
                else {
                    return 0;
                }
            }
        }
    }

    return 0;
}

static void keypad_task(void* parameter)
{
    int last_key = 0;

    while (true) {

        int key = keypad_read();

        // Reagujemy tylko na nowe naciśnięcie.
        //
        // Dzięki temu przytrzymanie "5"
        // nie generuje 100 razy klawisza 5.
        if (key != 0 && last_key == 0) {

            printf("Pressed: %d\n", key);

            // Kolejka długości 1.
            // Jeśli już coś tam jest, zastępujemy najnowszą wartością.
            xQueueOverwrite(key_queue, &key);
        }

        last_key = key;

        // 20 ms działa również jako prosty debounce
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void keypad_init()
{
    // Wiersze jako OUTPUT
    for (int i = 0; i < 4; i++) {
        gpio_reset_pin(ROWS[i]);
        gpio_set_direction(ROWS[i], GPIO_MODE_OUTPUT);

        // Normalnie HIGH
        gpio_set_level(ROWS[i], 1);
    }

    // Kolumny jako INPUT + pull-up
    for (int i = 0; i < 4; i++) {
        gpio_reset_pin(COLS[i]);
        gpio_set_direction(COLS[i], GPIO_MODE_INPUT);
        gpio_set_pull_mode(COLS[i], GPIO_PULLUP_ONLY);
    }
}

void keypad_start()
{
    key_queue = xQueueCreate(1, sizeof(int));

    xTaskCreate(
        keypad_task,
        "keypad_task",
        2048,
        nullptr,
        5,
        nullptr
    );
}

bool numpad_receive_key(int& key, TickType_t timeout)
{
    return xQueueReceive(key_queue, &key, timeout) == pdTRUE;
}

bool wait_for_new_key(int milliseconds, int& new_key)
{
    return numpad_receive_key(new_key, pdMS_TO_TICKS(milliseconds));
}
