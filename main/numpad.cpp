#include "numpad.hpp"

#include "standard_output.hpp"

#include "freertos/task.h"

#include "esp_rom_sys.h"

NumPad::NumPad(StandardOutput& output)
    : output_(output)
    , key_queue_(xQueueCreate(1, sizeof(int)))
{
}

void NumPad::initGpio()
{

    for (int i = 0; i < 4; i++) {
        gpio_reset_pin(ROWS[i]);
        gpio_set_direction(ROWS[i], GPIO_MODE_OUTPUT);

        gpio_set_level(ROWS[i], 1);
    }

    for (int i = 0; i < 4; i++) {
        gpio_reset_pin(COLS[i]);
        gpio_set_direction(COLS[i], GPIO_MODE_INPUT);
        gpio_set_pull_mode(COLS[i], GPIO_PULLUP_ONLY);
    }
}

int NumPad::readKey()
{
    for (int row = 0; row < 4; row++) {

        for (int i = 0; i < 4; i++) {
            gpio_set_level(ROWS[i], 1);
        }

        gpio_set_level(ROWS[row], 0);

        esp_rom_delay_us(5);

        for (int col = 0; col < 4; col++) {

            if (gpio_get_level(COLS[col]) == 0) {

                if (col < 3 && row < 3) {
                    return col + 3 * row + 1;
                }
                else if (col == 3 && row == 0) {
                    return -1;
                }
                else {
                    return 0;
                }
            }
        }
    }

    return 0;
}

void NumPad::run()
{
    initGpio();

    int last_key = 0;

    while (true) {

        int key = readKey();

        if (key != 0 && last_key == 0) {

            output_.printf("Pressed: %d\n", key);

            xQueueOverwrite(key_queue_, &key);
        }

        last_key = key;

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

bool NumPad::receiveKey(int& key, TickType_t timeout)
{
    return xQueueReceive(key_queue_, &key, timeout) == pdTRUE;
}

bool NumPad::waitForNewKey(int milliseconds, int& new_key)
{
    return receiveKey(new_key, pdMS_TO_TICKS(milliseconds));
}
