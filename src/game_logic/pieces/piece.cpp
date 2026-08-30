#include "piece.hpp"

#include "bishop.hpp"
#include "chess_board.hpp"
#include "king.hpp"
#include "knight.hpp"
#include "pawn.hpp"
#include "queen.hpp"
#include "rook.hpp"

#include <cctype>

namespace {

bool offsetSquare(Square from, Offset offset, Square& result)
{
    const int file = static_cast<int>(from.file) + offset.file;
    const int rank = static_cast<int>(from.rank) + offset.rank;
    if (file < 0 || file > 7 || rank < 0 || rank > 7) {
        return false;
    }
    result = {
        static_cast<uint8_t>(file),
        static_cast<uint8_t>(rank),
    };
    return true;
}

} // namespace

void Piece::appendSlides(
    const ChessBoard& board,
    Square from,
    const Offset* offsets,
    size_t count,
    MoveList& moves
)
{
    const Color moving_color = board.colorAt(from);
    for (size_t index = 0; index < count; ++index) {
        Square current = from;
        while (offsetSquare(current, offsets[index], current)) {
            if (board.isEmpty(current)) {
                moves.append({ from, current, MoveKind::Normal, '\0' });
                continue;
            }
            if (board.colorAt(current) != moving_color) {
                moves.append({ from, current, MoveKind::Normal, '\0' });
            }
            break;
        }
    }
}

bool Piece::slideAttacks(
    const ChessBoard& board,
    Square from,
    Square target,
    const Offset* offsets,
    size_t count
)
{
    for (size_t index = 0; index < count; ++index) {
        Square current = from;
        while (offsetSquare(current, offsets[index], current)) {
            if (current == target) {
                return true;
            }
            if (!board.isEmpty(current)) {
                break;
            }
        }
    }
    return false;
}

void Piece::appendSteps(
    const ChessBoard& board,
    Square from,
    const Offset* offsets,
    size_t count,
    MoveList& moves
)
{
    const Color moving_color = board.colorAt(from);
    for (size_t index = 0; index < count; ++index) {
        Square target;
        if (offsetSquare(from, offsets[index], target)
            && (board.isEmpty(target) || board.colorAt(target) != moving_color)) {
            moves.append({ from, target, MoveKind::Normal, '\0' });
        }
    }
}

bool Piece::stepAttacks(
    Square from,
    Square target,
    const Offset* offsets,
    size_t count
)
{
    for (size_t index = 0; index < count; ++index) {
        Square candidate;
        if (offsetSquare(from, offsets[index], candidate)
            && candidate == target) {
            return true;
        }
    }
    return false;
}

const Piece* Piece::forSquare(char piece)
{
    switch (std::toupper(static_cast<unsigned char>(piece))) {
    case 'P':
        return &PAWN;
    case 'N':
        return &KNIGHT;
    case 'B':
        return &BISHOP;
    case 'R':
        return &ROOK;
    case 'Q':
        return &QUEEN;
    case 'K':
        return &KING;
    default:
        return nullptr;
    }
}
