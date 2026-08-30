#include "bishop.hpp"

namespace {

constexpr Offset OFFSETS[] = {
    { -1, -1 }, { -1, 1 }, { 1, -1 }, { 1, 1 },
};

} // namespace

const Bishop BISHOP;

char Bishop::letter() const { return 'B'; }
const char* Bishop::name() const { return "bishop"; }

void Bishop::appendMoves(
    const ChessBoard& board,
    Square from,
    MoveList& moves
) const
{
    appendSlides(board, from, OFFSETS, 4, moves);
}

bool Bishop::attacks(
    const ChessBoard& board,
    Square from,
    Square target
) const
{
    return slideAttacks(board, from, target, OFFSETS, 4);
}
