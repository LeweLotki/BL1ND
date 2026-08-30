#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "driver/gpio.h"

class Led {
public:
    enum class BlinkPattern : uint8_t {
        Single,
        Error,
    };

    Led();

    void run();
    bool blinkOnce();
    bool blinkError();

private:
    void initGpio();
    void blink(BlinkPattern pattern);

    static constexpr gpio_num_t PIN_ = GPIO_NUM_2;

    QueueHandle_t command_queue_;
};
