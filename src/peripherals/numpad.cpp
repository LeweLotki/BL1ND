#include "numpad.hpp"

#include "freertos/task.h"

#include "esp_rom_sys.h"

NumPad::NumPad()
    : key_queue_(xQueueCreate(8, sizeof(char)))
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

char NumPad::readKey()
{
    for (int row = 0; row < 4; row++) {

        for (int i = 0; i < 4; i++) {
            gpio_set_level(ROWS[i], 1);
        }

        gpio_set_level(ROWS[row], 0);

        esp_rom_delay_us(5);

        for (int col = 0; col < 4; col++) {

            if (gpio_get_level(COLS[col]) == 0) {
                return KeypadLayout::keyAt(row, col);
            }
        }
    }

    return KeypadLayout::NO_KEY;
}

void NumPad::run()
{
    initGpio();

    char last_key = KeypadLayout::NO_KEY;

    while (true) {

        const char key = readKey();

        if (key != KeypadLayout::NO_KEY
            && last_key == KeypadLayout::NO_KEY) {
            xQueueSend(key_queue_, &key, portMAX_DELAY);
        }

        last_key = key;

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

bool NumPad::receiveKey(char& key, TickType_t timeout)
{
    return xQueueReceive(key_queue_, &key, timeout) == pdTRUE;
}

bool NumPad::waitForNewKey(int milliseconds, char& new_key)
{
    return receiveKey(new_key, pdMS_TO_TICKS(milliseconds));
}
