#pragma once

#include "driver/gpio.h"

class NumPad;

class Led {
public:
    Led(NumPad& numpad);

    void run();

private:
    void initGpio();
    void blink(int times);

    NumPad& numpad_;

    static constexpr gpio_num_t PIN_ = GPIO_NUM_2;
};
