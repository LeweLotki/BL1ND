#include "game.hpp"

#include "board_snapshot.hpp"
#include "led.hpp"
#include "numpad.hpp"
#include "standard_output.hpp"

#include "freertos/FreeRTOS.h"

Game::Game(
    NumPad& numpad,
    Led& led,
    StandardOutput& output,
    BoardSnapshot& board_snapshot
)
    : numpad_(numpad)
    , led_(led)
    , output_(output)
    , board_snapshot_(board_snapshot)
{
}

void Game::run()
{
    board_snapshot_.publish(chess_game_.board());

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
    char message[96];
    if (ChessGame::formatOutput(event, message, sizeof(message))) {
        output_.print(message);
    }

    switch (event.type) {
    case GameEventType::MoveAccepted:
        led_.blinkOnce();
        board_snapshot_.publish(chess_game_.board());
        break;
    case GameEventType::MoveRejected:
        led_.blinkError();
        break;
    case GameEventType::Reset:
        board_snapshot_.publish(chess_game_.board());
        break;
    case GameEventType::None:
    case GameEventType::PromotionPending:
        break;
    }
}
