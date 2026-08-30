#include "knight.hpp"

namespace {

constexpr Offset OFFSETS[] = {
    { -2, -1 }, { -2, 1 }, { -1, -2 }, { -1, 2 },
    { 1, -2 }, { 1, 2 }, { 2, -1 }, { 2, 1 },
};

} // namespace

const Knight KNIGHT;

char Knight::letter() const { return 'N'; }
const char* Knight::name() const { return "knight"; }

void Knight::appendMoves(
    const ChessBoard& board,
    Square from,
    MoveList& moves
) const
{
    appendSteps(board, from, OFFSETS, 8, moves);
}

bool Knight::attacks(
    const ChessBoard&,
    Square from,
    Square target
) const
{
    return stepAttacks(from, target, OFFSETS, 8);
}
