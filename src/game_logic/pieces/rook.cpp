#include "rook.hpp"

namespace {

constexpr Offset OFFSETS[] = {
    { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 },
};

} // namespace

const Rook ROOK;

char Rook::letter() const { return 'R'; }
const char* Rook::name() const { return "rook"; }

void Rook::appendMoves(
    const ChessBoard& board,
    Square from,
    MoveList& moves
) const
{
    appendSlides(board, from, OFFSETS, 4, moves);
}

bool Rook::attacks(
    const ChessBoard& board,
    Square from,
    Square target
) const
{
    return slideAttacks(board, from, target, OFFSETS, 4);
}
