#include "chess_game.hpp"

#include <cstdio>

namespace {

GameEvent emptyEvent(GameEventType type)
{
    return { type, {}, 0 };
}

} // namespace

ChessGame::ChessGame()
    : digit_count_(0)
    , move_number_(1)
{
}

GameEvent ChessGame::handleKey(char key)
{
    if (key == '*') {
        reset();
        return emptyEvent(GameEventType::Reset);
    }

    if (key < '1' || key > '8') {
        return emptyEvent(GameEventType::None);
    }

    digits_[digit_count_++] = key;
    if (digit_count_ < 4) {
        return emptyEvent(GameEventType::None);
    }

    digit_count_ = 0;
    return processMove();
}

const ChessBoard& ChessGame::board() const
{
    return board_;
}

bool ChessGame::formatOutput(
    const GameEvent& event,
    char* output,
    size_t output_size
)
{
    if (output_size == 0) {
        return false;
    }

    if (event.type == GameEventType::MoveAccepted) {
        snprintf(
            output,
            output_size,
            "%s = %u. %s\n",
            event.notation.coordinate,
            event.move_number,
            event.notation.algebraic
        );
        return true;
    }

    if (event.type == GameEventType::MoveRejected) {
        snprintf(
            output,
            output_size,
            "%s = invalid: empty from-square\n",
            event.notation.coordinate
        );
        return true;
    }

    output[0] = '\0';
    return false;
}

GameEvent ChessGame::processMove()
{
    const Square from = {
        ChessBoard::digitToIndex(digits_[0]),
        ChessBoard::digitToIndex(digits_[1]),
    };
    const Square to = {
        ChessBoard::digitToIndex(digits_[2]),
        ChessBoard::digitToIndex(digits_[3]),
    };

    GameEvent event = {
        GameEventType::MoveAccepted,
        {},
        move_number_,
    };
    if (!board_.formatMove(from, to, event.notation)) {
        event.type = GameEventType::MoveRejected;
        return event;
    }

    board_.applyMove(from, to);
    ++move_number_;
    return event;
}

void ChessGame::reset()
{
    board_.reset();
    digit_count_ = 0;
    move_number_ = 1;
}
