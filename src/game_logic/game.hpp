#pragma once

#include "chess_game.hpp"

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

    void run();

private:
    void handleKey(char key);

    NumPad& numpad_;
    Led& led_;
    StandardOutput& output_;
    BoardSnapshot& board_snapshot_;
    ChessGame chess_game_;
};
