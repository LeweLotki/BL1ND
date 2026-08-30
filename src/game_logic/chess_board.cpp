#include "chess_board.hpp"

#include "piece.hpp"

#include <cstring>

ChessBoard::ChessBoard()
{
    reset();
}

void ChessBoard::reset()
{
    for (int rank = 0; rank < 8; ++rank) {
        memcpy(board_[rank], INITIAL_POSITION[rank], 8);
    }
    side_to_move_ = Color::White;
    white_kingside_ = true;
    white_queenside_ = true;
    black_kingside_ = true;
    black_queenside_ = true;
    en_passant_file_ = -1;
}

char ChessBoard::pieceAt(Square square) const
{
    return board_[square.rank][square.file];
}

Color ChessBoard::colorAt(Square square) const
{
    return colorOf(pieceAt(square));
}

bool ChessBoard::isEmpty(Square square) const
{
    return ::isEmpty(pieceAt(square));
}

Color ChessBoard::sideToMove() const
{
    return side_to_move_;
}

Square ChessBoard::enPassantTarget() const
{
    if (en_passant_file_ < 0) {
        return { 8, 8 };
    }
    return {
        static_cast<uint8_t>(en_passant_file_),
        static_cast<uint8_t>(side_to_move_ == Color::White ? 5 : 2),
    };
}

Square ChessBoard::kingSquare(Color color) const
{
    const char king = color == Color::White ? 'K' : 'k';
    for (uint8_t rank = 0; rank < 8; ++rank) {
        for (uint8_t file = 0; file < 8; ++file) {
            if (board_[rank][file] == king) {
                return { file, rank };
            }
        }
    }
    return { 8, 8 };
}

bool ChessBoard::isAttacked(Square target, Color by) const
{
    for (uint8_t rank = 0; rank < 8; ++rank) {
        for (uint8_t file = 0; file < 8; ++file) {
            const Square from = { file, rank };
            const char value = pieceAt(from);
            const Piece* piece = Piece::forSquare(value);
            if (piece != nullptr && colorOf(value) == by
                && piece->attacks(*this, from, target)) {
                return true;
            }
        }
    }
    return false;
}

bool ChessBoard::hasCastlingRight(Color color, MoveKind side) const
{
    if (color == Color::White) {
        return side == MoveKind::CastleKingside
            ? white_kingside_
            : side == MoveKind::CastleQueenside && white_queenside_;
    }
    return side == MoveKind::CastleKingside
        ? black_kingside_
        : side == MoveKind::CastleQueenside && black_queenside_;
}

uint8_t ChessBoard::digitToIndex(char digit)
{
    return static_cast<uint8_t>(digit - '1');
}

void ChessBoard::formatSquare(Square square, char text[3])
{
    text[0] = static_cast<char>('a' + square.file);
    text[1] = static_cast<char>('1' + square.rank);
    text[2] = '\0';
}

void ChessBoard::clearCastlingRightsTouched(Square square)
{
    if (square == Square { 4, 0 }) {
        white_kingside_ = false;
        white_queenside_ = false;
    }
    if (square == Square { 0, 0 }) {
        white_queenside_ = false;
    }
    if (square == Square { 7, 0 }) {
        white_kingside_ = false;
    }
    if (square == Square { 4, 7 }) {
        black_kingside_ = false;
        black_queenside_ = false;
    }
    if (square == Square { 0, 7 }) {
        black_queenside_ = false;
    }
    if (square == Square { 7, 7 }) {
        black_kingside_ = false;
    }
}

void ChessBoard::apply(const Move& move)
{
    const char moving_piece = pieceAt(move.from);
    clearCastlingRightsTouched(move.from);
    clearCastlingRightsTouched(move.to);

    if (move.kind == MoveKind::CastleKingside
        || move.kind == MoveKind::CastleQueenside) {
        const Square rook_from = {
            static_cast<uint8_t>(
                move.kind == MoveKind::CastleKingside ? 7 : 0
            ),
            move.from.rank,
        };
        const Square rook_to = {
            static_cast<uint8_t>(
                move.kind == MoveKind::CastleKingside ? 5 : 3
            ),
            move.from.rank,
        };
        board_[rook_to.rank][rook_to.file] = board_[rook_from.rank][rook_from.file];
        board_[rook_from.rank][rook_from.file] = EMPTY;
    }

    if (move.kind == MoveKind::EnPassant) {
        board_[move.from.rank][move.to.file] = EMPTY;
    }

    char placed_piece = moving_piece;
    if (move.promotion != '\0') {
        placed_piece = colorOf(moving_piece) == Color::White
            ? move.promotion
            : static_cast<char>(move.promotion - 'A' + 'a');
    }
    board_[move.to.rank][move.to.file] = placed_piece;
    board_[move.from.rank][move.from.file] = EMPTY;

    en_passant_file_ = move.kind == MoveKind::DoublePawnPush
        ? static_cast<int8_t>(move.from.file)
        : -1;
    side_to_move_ = opposite(side_to_move_);
}

void ChessBoard::loadPosition(
    const char* const ranks[8],
    Color side_to_move,
    bool white_kingside,
    bool white_queenside,
    bool black_kingside,
    bool black_queenside,
    int en_passant_file
)
{
    for (uint8_t rank = 0; rank < 8; ++rank) {
        memcpy(board_[rank], ranks[rank], 8);
    }
    side_to_move_ = side_to_move;
    white_kingside_ = white_kingside;
    white_queenside_ = white_queenside;
    black_kingside_ = black_kingside;
    black_queenside_ = black_queenside;
    en_passant_file_ = static_cast<int8_t>(en_passant_file);
}
