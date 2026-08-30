#include "queen.hpp"

namespace {

constexpr Offset OFFSETS[] = {
    { -1, -1 }, { -1, 0 }, { -1, 1 }, { 0, -1 },
    { 0, 1 }, { 1, -1 }, { 1, 0 }, { 1, 1 },
};

} // namespace

const Queen QUEEN;

char Queen::letter() const { return 'Q'; }
const char* Queen::name() const { return "queen"; }

void Queen::appendMoves(
    const ChessBoard& board,
    Square from,
    MoveList& moves
) const
{
    appendSlides(board, from, OFFSETS, 8, moves);
}

bool Queen::attacks(
    const ChessBoard& board,
    Square from,
    Square target
) const
{
    return slideAttacks(board, from, target, OFFSETS, 8);
}
