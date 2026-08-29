#include "led.hpp"
#include "numpad.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static NumPad numpad;
static Led led(numpad);

static void numpad_task(void* parameter)
{
    static_cast<NumPad*>(parameter)->run();
}

static void led_task(void* parameter)
{
    static_cast<Led*>(parameter)->run();
}

extern "C" void app_main()
{
    xTaskCreate(
        numpad_task,
        "numpad_task",
        2048,
        &numpad,
        5,
        nullptr
    );

    xTaskCreate(
        led_task,
        "led_task",
        2048,
        &led,
        4,
        nullptr
    );
}
