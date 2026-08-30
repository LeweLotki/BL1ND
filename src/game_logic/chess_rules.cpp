#include "chess_rules.hpp"

#include "piece.hpp"

#include <cctype>
#include <cstdio>
#include <cstdlib>

namespace {

bool isSlider(char piece)
{
    const char upper = static_cast<char>(
        std::toupper(static_cast<unsigned char>(piece))
    );
    return upper == 'B' || upper == 'R' || upper == 'Q';
}

bool sliderAligned(char piece, int file_delta, int rank_delta)
{
    const char upper = static_cast<char>(
        std::toupper(static_cast<unsigned char>(piece))
    );
    const bool diagonal = std::abs(file_delta) == std::abs(rank_delta);
    const bool straight = file_delta == 0 || rank_delta == 0;
    return (upper == 'B' && diagonal)
        || (upper == 'R' && straight)
        || (upper == 'Q' && (diagonal || straight));
}

int sign(int value)
{
    return (value > 0) - (value < 0);
}

bool pathBlocked(
    const ChessBoard& board,
    Square from,
    Square to,
    char piece
)
{
    const int file_delta = static_cast<int>(to.file) - from.file;
    const int rank_delta = static_cast<int>(to.rank) - from.rank;
    if (!isSlider(piece) || !sliderAligned(piece, file_delta, rank_delta)) {
        return false;
    }
    const int file_step = sign(file_delta);
    const int rank_step = sign(rank_delta);
    int file = static_cast<int>(from.file) + file_step;
    int rank = static_cast<int>(from.rank) + rank_step;
    while (file != to.file || rank != to.rank) {
        if (!board.isEmpty({
                static_cast<uint8_t>(file),
                static_cast<uint8_t>(rank),
            })) {
            return true;
        }
        file += file_step;
        rank += rank_step;
    }
    return false;
}

bool pawnDoubleBlocked(
    const ChessBoard& board,
    Square from,
    Square to,
    char piece
)
{
    if (std::toupper(static_cast<unsigned char>(piece)) != 'P'
        || from.file != to.file
        || std::abs(static_cast<int>(to.rank) - from.rank) != 2) {
        return false;
    }
    const Square middle = {
        from.file,
        static_cast<uint8_t>((from.rank + to.rank) / 2),
    };
    return !board.isEmpty(middle) || !board.isEmpty(to);
}

bool castlingAttempt(
    const ChessBoard& board,
    Square from,
    Square to,
    MoveError& error
)
{
    const char value = board.pieceAt(from);
    if (std::toupper(static_cast<unsigned char>(value)) != 'K'
        || from.rank != to.rank
        || std::abs(static_cast<int>(to.file) - from.file) != 2) {
        return false;
    }

    const Color color = board.colorAt(from);
    const uint8_t home_rank = color == Color::White ? 0 : 7;
    const MoveKind kind = to.file > from.file
        ? MoveKind::CastleKingside
        : MoveKind::CastleQueenside;
    if (from != Square { 4, home_rank }
        || to.file != (kind == MoveKind::CastleKingside ? 6 : 2)
        || !board.hasCastlingRight(color, kind)) {
        error = MoveError::CastlingRightsLost;
        return true;
    }

    const bool kingside = kind == MoveKind::CastleKingside;
    const uint8_t rook_file = kingside ? 7 : 0;
    const char rook = color == Color::White ? 'R' : 'r';
    if (board.pieceAt({ rook_file, home_rank }) != rook) {
        error = MoveError::CastlingRightsLost;
        return true;
    }
    const uint8_t first = kingside ? 5 : 1;
    const uint8_t last = kingside ? 6 : 3;
    for (uint8_t file = first; file <= last; ++file) {
        if (!board.isEmpty({ file, home_rank })) {
            error = MoveError::CastlingPathBlocked;
            return true;
        }
    }

    const Color enemy = opposite(color);
    if (board.isAttacked(from, enemy)
        || board.isAttacked(
            { static_cast<uint8_t>(kingside ? 5 : 3), home_rank },
            enemy
        )
        || board.isAttacked(to, enemy)) {
        error = MoveError::CastlingThroughCheck;
        return true;
    }
    error = MoveError::Unreachable;
    return true;
}

} // namespace

namespace ChessRules {

bool leavesKingInCheck(const ChessBoard& board, const Move& move)
{
    const Color mover = board.colorAt(move.from);
    ChessBoard copy = board;
    copy.apply(move);
    const Square king = copy.kingSquare(mover);
    return isOnBoard(king) && copy.isAttacked(king, opposite(mover));
}

MoveError validate(
    const ChessBoard& board,
    Square from,
    Square to,
    char promotion,
    Move& resolved
)
{
    if (board.isEmpty(from)) {
        return MoveError::EmptyFromSquare;
    }
    if (board.colorAt(from) != board.sideToMove()) {
        return MoveError::NotYourPiece;
    }
    if (from == to) {
        return MoveError::SameSquare;
    }
    const char moving_value = board.pieceAt(from);
    const bool castle_shape = std::toupper(
        static_cast<unsigned char>(moving_value)
    ) == 'K'
        && from.rank == to.rank
        && std::abs(static_cast<int>(to.file) - from.file) == 2;
    if (!castle_shape && !board.isEmpty(to)
        && board.colorAt(to) == board.colorAt(from)) {
        return MoveError::FriendlyPiece;
    }

    const Piece* piece = Piece::forSquare(board.pieceAt(from));
    MoveList moves;
    piece->appendMoves(board, from, moves);
    const char requested = promotion == '\0'
        ? '\0'
        : static_cast<char>(
            std::toupper(static_cast<unsigned char>(promotion))
        );
    bool found = false;
    for (size_t index = 0; index < moves.size(); ++index) {
        if (moves[index].to == to && moves[index].promotion == requested) {
            resolved = moves[index];
            found = true;
            break;
        }
    }

    if (!found) {
        MoveError castling_error = MoveError::None;
        if (castlingAttempt(board, from, to, castling_error)) {
            return castling_error;
        }
        const char value = board.pieceAt(from);
        if (pathBlocked(board, from, to, value)
            || pawnDoubleBlocked(board, from, to, value)) {
            return MoveError::PathBlocked;
        }
        const bool pawn = std::toupper(
            static_cast<unsigned char>(value)
        ) == 'P';
        const int pawn_direction = board.colorAt(from) == Color::White ? 1 : -1;
        if (pawn && board.isEmpty(to)
            && std::abs(static_cast<int>(to.file) - from.file) == 1
            && static_cast<int>(to.rank) - from.rank == pawn_direction) {
            return MoveError::EnPassantUnavailable;
        }
        return MoveError::Unreachable;
    }

    return leavesKingInCheck(board, resolved)
        ? MoveError::KingLeftInCheck
        : MoveError::None;
}

bool hasLegalMove(const ChessBoard& board)
{
    for (uint8_t rank = 0; rank < 8; ++rank) {
        for (uint8_t file = 0; file < 8; ++file) {
            const Square from = { file, rank };
            if (board.isEmpty(from)
                || board.colorAt(from) != board.sideToMove()) {
                continue;
            }
            MoveList moves;
            Piece::forSquare(board.pieceAt(from))->appendMoves(
                board,
                from,
                moves
            );
            for (size_t index = 0; index < moves.size(); ++index) {
                if (!leavesKingInCheck(board, moves[index])) {
                    return true;
                }
            }
        }
    }
    return false;
}

PositionStatus status(const ChessBoard& board)
{
    const Square king = board.kingSquare(board.sideToMove());
    const bool checked = isOnBoard(king)
        && board.isAttacked(king, opposite(board.sideToMove()));
    if (hasLegalMove(board)) {
        return checked ? PositionStatus::Check : PositionStatus::Normal;
    }
    return checked ? PositionStatus::Checkmate : PositionStatus::Stalemate;
}

const char* describe(
    MoveError error,
    const ChessBoard& board,
    Square from,
    Square to,
    char* out,
    size_t out_size
)
{
    if (out_size == 0) {
        return out;
    }
    char to_text[3];
    ChessBoard::formatSquare(to, to_text);
    switch (error) {
    case MoveError::None:
        snprintf(out, out_size, "legal");
        break;
    case MoveError::EmptyFromSquare:
        snprintf(out, out_size, "empty from-square");
        break;
    case MoveError::NotYourPiece:
        snprintf(
            out,
            out_size,
            "it is %s's turn",
            board.sideToMove() == Color::White ? "White" : "Black"
        );
        break;
    case MoveError::SameSquare:
        snprintf(out, out_size, "from and to are the same square");
        break;
    case MoveError::FriendlyPiece:
        snprintf(out, out_size, "own piece on %s", to_text);
        break;
    case MoveError::Unreachable: {
        const Piece* piece = Piece::forSquare(board.pieceAt(from));
        snprintf(
            out,
            out_size,
            "%s cannot reach %s",
            piece == nullptr ? "piece" : piece->name(),
            to_text
        );
        break;
    }
    case MoveError::PathBlocked:
        snprintf(out, out_size, "path to %s is blocked", to_text);
        break;
    case MoveError::KingLeftInCheck:
        snprintf(out, out_size, "king would be left in check");
        break;
    case MoveError::CastlingRightsLost:
        snprintf(out, out_size, "castling rights lost");
        break;
    case MoveError::CastlingPathBlocked:
        snprintf(out, out_size, "castling path is blocked");
        break;
    case MoveError::CastlingThroughCheck:
        snprintf(
            out,
            out_size,
            "cannot castle out of, through, or into check"
        );
        break;
    case MoveError::EnPassantUnavailable:
        snprintf(out, out_size, "en passant no longer available");
        break;
    case MoveError::GameOver:
        snprintf(out, out_size, "game is over, press reset");
        break;
    case MoveError::NotYourSide:
        snprintf(out, out_size, "it is your opponent's turn");
        break;
    case MoveError::NotLinked:
        snprintf(out, out_size, "link is down");
        break;
    }
    return out;
}

} // namespace ChessRules
