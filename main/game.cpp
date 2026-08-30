#include "game.hpp"

#include "led.hpp"
#include "numpad.hpp"
#include "standard_output.hpp"

#include "freertos/FreeRTOS.h"

Game::Game(NumPad& numpad, Led& led, StandardOutput& output)
    : numpad_(numpad)
    , led_(led)
    , output_(output)
{
}

void Game::run()
{
    while (true) {
        char key = '\0';
        if (numpad_.receiveKey(key, portMAX_DELAY)) {
            handleKey(key);
        }
    }
}

void Game::handleKey(char key)
{
    const GameEvent event = chess_game_.handleKey(key);
    char message[64];
    if (ChessGame::formatOutput(event, message, sizeof(message))) {
        output_.print(message);
    }

    if (event.type == GameEventType::MoveAccepted) {
        led_.blinkOnce();
    }
}
