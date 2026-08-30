#include "chess_board.hpp"

#include <cctype>
#include <cstdio>
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
}

char ChessBoard::pieceAt(Square square) const
{
    return board_[square.rank][square.file];
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

bool ChessBoard::isCastling(Square from, Square to, char piece) const
{
    const bool white_home = piece == 'K' && from.rank == 0;
    const bool black_home = piece == 'k' && from.rank == 7;

    return (white_home || black_home)
        && from.file == 4
        && to.rank == from.rank
        && (to.file == 2 || to.file == 6);
}

bool ChessBoard::formatMove(Square from, Square to, MoveNotation& notation) const
{
    char from_text[3];
    char to_text[3];
    formatSquare(from, from_text);
    formatSquare(to, to_text);
    snprintf(
        notation.coordinate,
        sizeof(notation.coordinate),
        "%s%s",
        from_text,
        to_text
    );

    const char piece = pieceAt(from);
    if (piece == EMPTY) {
        return false;
    }

    if (isCastling(from, to, piece)) {
        strcpy(notation.algebraic, to.file == 6 ? "O-O" : "O-O-O");
        return true;
    }

    const char upper_piece = static_cast<char>(
        std::toupper(static_cast<unsigned char>(piece))
    );
    const bool is_pawn = upper_piece == 'P';
    const bool is_capture = pieceAt(to) != EMPTY
        || (is_pawn && from.file != to.file);
    const bool is_promotion = piece == 'P' && to.rank == 7;

    if (is_pawn) {
        if (is_capture) {
            snprintf(
                notation.algebraic,
                sizeof(notation.algebraic),
                "%cx%s%s",
                from_text[0],
                to_text,
                is_promotion ? "=Q" : ""
            );
        }
        else {
            snprintf(
                notation.algebraic,
                sizeof(notation.algebraic),
                "%s%s",
                to_text,
                is_promotion ? "=Q" : ""
            );
        }
    }
    else {
        snprintf(
            notation.algebraic,
            sizeof(notation.algebraic),
            "%c%s%s",
            upper_piece,
            is_capture ? "x" : "",
            to_text
        );
    }

    return true;
}

void ChessBoard::applyMove(Square from, Square to)
{
    const char piece = pieceAt(from);

    if (isCastling(from, to, piece)) {
        const Square rook_from = {
            static_cast<uint8_t>(to.file == 6 ? 7 : 0),
            from.rank,
        };
        const Square rook_to = {
            static_cast<uint8_t>(to.file == 6 ? 5 : 3),
            from.rank,
        };
        board_[rook_to.rank][rook_to.file] = board_[rook_from.rank][rook_from.file];
        board_[rook_from.rank][rook_from.file] = EMPTY;
    }

    board_[to.rank][to.file] = piece == 'P' && to.rank == 7 ? 'Q' : piece;
    board_[from.rank][from.file] = EMPTY;
}
