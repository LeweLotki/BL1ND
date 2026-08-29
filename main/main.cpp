#include "led.hpp"
#include "numpad.hpp"
#include "standard_output.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static StandardOutput output;
static NumPad numpad(output);
static Led led(numpad);

static void standard_output_task(void* parameter)
{
    static_cast<StandardOutput*>(parameter)->run();
}

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
        standard_output_task,
        "standard_output_task",
        2048,
        &output,
        5,
        nullptr
    );

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
