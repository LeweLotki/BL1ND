#pragma once

#include "chess_board.hpp"
#include "chess_rules.hpp"

#include <cstddef>

enum class GameEventType {
    None,
    MoveAccepted,
    MoveRejected,
    PromotionPending,
    Reset,
};

struct GameEvent {
    GameEventType type;
    MoveNotation notation;
    unsigned int move_number;
    MoveError error;
    PositionStatus status;
    char error_description[56];
};

class ChessGame {
public:
    ChessGame();
    explicit ChessGame(const ChessBoard& board);

    GameEvent handleKey(char key);
    const ChessBoard& board() const;

    static bool formatOutput(
        const GameEvent& event,
        char* output,
        size_t output_size
    );

private:
    GameEvent processMove();
    GameEvent finishMove(Move move);
    void reset();

    ChessBoard board_;
    char digits_[4];
    size_t digit_count_;
    unsigned int move_number_;
    bool promotion_pending_;
    Move pending_move_;
    PositionStatus game_result_;
};
