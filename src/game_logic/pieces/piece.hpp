#pragma once

#include "move.hpp"

class ChessBoard;

struct Offset {
    int8_t file;
    int8_t rank;
};

class Piece {
public:
    virtual ~Piece() = default;
    virtual char letter() const = 0;
    virtual const char* name() const = 0;
    virtual void appendMoves(
        const ChessBoard& board,
        Square from,
        MoveList& moves
    ) const = 0;
    virtual bool attacks(
        const ChessBoard& board,
        Square from,
        Square target
    ) const = 0;

    static const Piece* forSquare(char piece);

protected:
    static void appendSlides(
        const ChessBoard& board,
        Square from,
        const Offset* offsets,
        size_t count,
        MoveList& moves
    );
    static bool slideAttacks(
        const ChessBoard& board,
        Square from,
        Square target,
        const Offset* offsets,
        size_t count
    );
    static void appendSteps(
        const ChessBoard& board,
        Square from,
        const Offset* offsets,
        size_t count,
        MoveList& moves
    );
    static bool stepAttacks(
        Square from,
        Square target,
        const Offset* offsets,
        size_t count
    );
};
