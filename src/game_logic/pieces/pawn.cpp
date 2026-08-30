#include "pawn.hpp"

#include "chess_board.hpp"

namespace {

void appendPawnMove(
    MoveList& moves,
    Square from,
    Square to,
    MoveKind kind
)
{
    if (to.rank == 0 || to.rank == 7) {
        constexpr char PROMOTIONS[] = { 'Q', 'R', 'B', 'N' };
        for (char promotion : PROMOTIONS) {
            moves.append({ from, to, kind, promotion });
        }
        return;
    }
    moves.append({ from, to, kind, '\0' });
}

} // namespace

const Pawn PAWN;

char Pawn::letter() const { return 'P'; }
const char* Pawn::name() const { return "pawn"; }

void Pawn::appendMoves(
    const ChessBoard& board,
    Square from,
    MoveList& moves
) const
{
    const Color color = board.colorAt(from);
    const int direction = color == Color::White ? 1 : -1;
    const uint8_t home_rank = color == Color::White ? 1 : 6;
    const int one_rank = static_cast<int>(from.rank) + direction;
    if (one_rank < 0 || one_rank > 7) {
        return;
    }

    const Square one = {
        from.file,
        static_cast<uint8_t>(one_rank),
    };
    if (board.isEmpty(one)) {
        appendPawnMove(moves, from, one, MoveKind::Normal);
        const int two_rank = static_cast<int>(from.rank) + 2 * direction;
        const Square two = {
            from.file,
            static_cast<uint8_t>(two_rank),
        };
        if (from.rank == home_rank && board.isEmpty(two)) {
            moves.append({ from, two, MoveKind::DoublePawnPush, '\0' });
        }
    }

    const Square en_passant = board.enPassantTarget();
    constexpr int FILE_DELTAS[] = { -1, 1 };
    for (int file_delta : FILE_DELTAS) {
        const int target_file = static_cast<int>(from.file) + file_delta;
        if (target_file < 0 || target_file > 7) {
            continue;
        }
        const Square target = {
            static_cast<uint8_t>(target_file),
            static_cast<uint8_t>(one_rank),
        };
        if (!board.isEmpty(target) && board.colorAt(target) != color) {
            appendPawnMove(moves, from, target, MoveKind::Normal);
        }
        else if (target == en_passant) {
            const Square victim = { target.file, from.rank };
            const char expected = color == Color::White ? 'p' : 'P';
            if (board.pieceAt(victim) == expected) {
                appendPawnMove(moves, from, target, MoveKind::EnPassant);
            }
        }
    }
}

bool Pawn::attacks(
    const ChessBoard& board,
    Square from,
    Square target
) const
{
    const int direction = board.colorAt(from) == Color::White ? 1 : -1;
    const int file_delta = static_cast<int>(target.file)
        - static_cast<int>(from.file);
    const int rank_delta = static_cast<int>(target.rank)
        - static_cast<int>(from.rank);
    return (file_delta == -1 || file_delta == 1)
        && rank_delta == direction;
}
