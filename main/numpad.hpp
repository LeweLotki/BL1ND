#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "driver/gpio.h"

class NumPad {
public:
    NumPad();

    void run();

    bool receiveKey(int& key, TickType_t timeout);

    bool waitForNewKey(int milliseconds, int& new_key);

private:
    void initGpio();
    int readKey();

    static constexpr gpio_num_t ROWS[4] = {
        GPIO_NUM_13,
        GPIO_NUM_12,
        GPIO_NUM_14,
        GPIO_NUM_27,
    };

    static constexpr gpio_num_t COLS[4] = {
        GPIO_NUM_26,
        GPIO_NUM_25,
        GPIO_NUM_33,
        GPIO_NUM_32,
    };

    QueueHandle_t key_queue_;
};
