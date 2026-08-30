#pragma once

#include "piece.hpp"

class Knight final : public Piece {
public:
    char letter() const override;
    const char* name() const override;
    void appendMoves(
        const ChessBoard& board,
        Square from,
        MoveList& moves
    ) const override;
    bool attacks(
        const ChessBoard& board,
        Square from,
        Square target
    ) const override;
};

extern const Knight KNIGHT;
