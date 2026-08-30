#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "driver/gpio.h"

class Led {
public:
    Led();

    void run();
    bool blinkOnce();

private:
    void initGpio();
    void blink();

    static constexpr gpio_num_t PIN_ = GPIO_NUM_2;

    QueueHandle_t command_queue_;
};
