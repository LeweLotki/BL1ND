#include "main.hpp"

#include "board_server.hpp"
#include "board_snapshot.hpp"
#include "game.hpp"
#include "led.hpp"
#include "numpad.hpp"
#include "standard_output.hpp"
#include "wifi_access_point.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_system.h"

static StandardOutput output;
static BoardSnapshot board_snapshot;
static WifiAccessPoint wifi_access_point(output);
static BoardServer board_server(board_snapshot, output);
static NumPad numpad;
static Led led;
static Game game(numpad, led, output, board_snapshot);

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

static void game_task(void* parameter)
{
    static_cast<Game*>(parameter)->run();
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

    if (wifi_access_point.start()) {
        board_server.start();
        output.printf(
            "Preview: free heap after startup: %lu bytes\n",
            static_cast<unsigned long>(esp_get_free_heap_size())
        );
    }

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

    xTaskCreate(
        game_task,
        "game_task",
        2048,
        &game,
        5,
        nullptr
    );
}
