#pragma once

#include "keypad_layout.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "driver/gpio.h"

class NumPad {
public:
    static constexpr UBaseType_t QUEUE_LENGTH = 8;

    NumPad();

    void run();

    bool receiveKey(char& key, TickType_t timeout);
    QueueHandle_t queue() const;

    bool waitForNewKey(int milliseconds, char& new_key);

private:
    void initGpio();
    char readKey();

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
