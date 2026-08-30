#pragma once

#include "bluetooth_link.hpp"
#include "chess_game.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

class BoardSnapshot;
class Led;
class NumPad;
class StandardOutput;

class Game {
public:
    Game(
        NumPad& numpad,
        Led& led,
        StandardOutput& output,
        BoardSnapshot& board_snapshot
    );

    bool startLink();
    void run();

private:
    void handleKey(char key);
    void handleLinkOutput(const LinkOutput& output);
    void drainLinkOutputs();

    NumPad& numpad_;
    Led& led_;
    StandardOutput& output_;
    BoardSnapshot& board_snapshot_;
    ChessGame chess_game_;
    BluetoothLink bluetooth_link_;
    QueueSetHandle_t input_set_;
};
