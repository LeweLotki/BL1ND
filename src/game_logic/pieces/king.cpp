#include "king.hpp"

#include "chess_board.hpp"

namespace {

constexpr Offset OFFSETS[] = {
    { -1, -1 }, { -1, 0 }, { -1, 1 }, { 0, -1 },
    { 0, 1 }, { 1, -1 }, { 1, 0 }, { 1, 1 },
};

bool canCastle(
    const ChessBoard& board,
    Square from,
    MoveKind kind
)
{
    const Color color = board.colorAt(from);
    const uint8_t home_rank = color == Color::White ? 0 : 7;
    if (from != Square { 4, home_rank }
        || !board.hasCastlingRight(color, kind)) {
        return false;
    }

    const bool kingside = kind == MoveKind::CastleKingside;
    const Square rook = {
        static_cast<uint8_t>(kingside ? 7 : 0),
        home_rank,
    };
    const char expected_rook = color == Color::White ? 'R' : 'r';
    if (board.pieceAt(rook) != expected_rook) {
        return false;
    }

    const uint8_t first = kingside ? 5 : 1;
    const uint8_t last = kingside ? 6 : 3;
    for (uint8_t file = first; file <= last; ++file) {
        if (!board.isEmpty({ file, home_rank })) {
            return false;
        }
    }

    const Color enemy = opposite(color);
    const Square through = {
        static_cast<uint8_t>(kingside ? 5 : 3),
        home_rank,
    };
    const Square destination = {
        static_cast<uint8_t>(kingside ? 6 : 2),
        home_rank,
    };
    return !board.isAttacked(from, enemy)
        && !board.isAttacked(through, enemy)
        && !board.isAttacked(destination, enemy);
}

} // namespace

const King KING;

char King::letter() const { return 'K'; }
const char* King::name() const { return "king"; }

void King::appendMoves(
    const ChessBoard& board,
    Square from,
    MoveList& moves
) const
{
    appendSteps(board, from, OFFSETS, 8, moves);
    if (canCastle(board, from, MoveKind::CastleKingside)) {
        moves.append({
            from,
            { 6, from.rank },
            MoveKind::CastleKingside,
            '\0',
        });
    }
    if (canCastle(board, from, MoveKind::CastleQueenside)) {
        moves.append({
            from,
            { 2, from.rank },
            MoveKind::CastleQueenside,
            '\0',
        });
    }
}

bool King::attacks(
    const ChessBoard&,
    Square from,
    Square target
) const
{
    return stepAttacks(from, target, OFFSETS, 8);
}
