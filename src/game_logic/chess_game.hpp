#pragma once

#include "chess_board.hpp"

#include <cstddef>

enum class GameEventType {
    None,
    MoveAccepted,
    MoveRejected,
    Reset,
};

struct GameEvent {
    GameEventType type;
    MoveNotation notation;
    unsigned int move_number;
};

class ChessGame {
public:
    ChessGame();

    GameEvent handleKey(char key);
    const ChessBoard& board() const;

    static bool formatOutput(
        const GameEvent& event,
        char* output,
        size_t output_size
    );

private:
    GameEvent processMove();
    void reset();

    ChessBoard board_;
    char digits_[4];
    size_t digit_count_;
    unsigned int move_number_;
};
