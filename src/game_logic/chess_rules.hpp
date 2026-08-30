#pragma once

#include "chess_board.hpp"

#include <cstddef>

enum class MoveError {
    None,
    EmptyFromSquare,
    NotYourPiece,
    SameSquare,
    FriendlyPiece,
    Unreachable,
    PathBlocked,
    KingLeftInCheck,
    CastlingRightsLost,
    CastlingPathBlocked,
    CastlingThroughCheck,
    EnPassantUnavailable,
    GameOver,
};

enum class PositionStatus {
    Normal,
    Check,
    Checkmate,
    Stalemate,
};

namespace ChessRules {

bool leavesKingInCheck(const ChessBoard& board, const Move& move);
MoveError validate(
    const ChessBoard& board,
    Square from,
    Square to,
    char promotion,
    Move& resolved
);
bool hasLegalMove(const ChessBoard& board);
PositionStatus status(const ChessBoard& board);
const char* describe(
    MoveError error,
    const ChessBoard& board,
    Square from,
    Square to,
    char* out,
    size_t out_size
);

} // namespace ChessRules
