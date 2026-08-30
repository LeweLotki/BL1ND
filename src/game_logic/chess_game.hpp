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
    Move move;
};

class ChessGame {
public:
    ChessGame();
    explicit ChessGame(const ChessBoard& board);

    GameEvent handleKey(char key);
    GameEvent applyRemoteMove(Square from, Square to, char promotion);
    const ChessBoard& board() const;
    unsigned int moveNumber() const;
    void setOwnedColor(Color color);
    void setLinkAvailable(bool available);
    void clearOwnedColor();
    void adoptPosition(const ChessBoard& board, unsigned int move_number);
    void resetGame();

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
    bool has_owned_color_;
    Color owned_color_;
    bool link_available_;
};
