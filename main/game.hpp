#pragma once

#include "chess_game.hpp"

class Led;
class NumPad;
class StandardOutput;

class Game {
public:
    Game(NumPad& numpad, Led& led, StandardOutput& output);

    void run();

private:
    void handleKey(char key);

    NumPad& numpad_;
    Led& led_;
    StandardOutput& output_;
    ChessGame chess_game_;
};
