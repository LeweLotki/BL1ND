#pragma once

#include "move.hpp"

#include <cstdint>

struct MoveNotation {
    char coordinate[5];
    char algebraic[12];
};

class ChessBoard {
public:
    ChessBoard();

    void reset();

    char pieceAt(Square square) const;
    Color colorAt(Square square) const;
    bool isEmpty(Square square) const;
    Color sideToMove() const;
    Square enPassantTarget() const;
    Square kingSquare(Color color) const;
    bool isAttacked(Square target, Color by) const;
    bool hasCastlingRight(Color color, MoveKind side) const;
    void apply(const Move& move);

    void loadPosition(
        const char* const ranks[8],
        Color side_to_move,
        bool white_kingside = false,
        bool white_queenside = false,
        bool black_kingside = false,
        bool black_queenside = false,
        int en_passant_file = -1
    );

    static uint8_t digitToIndex(char digit);
    static void formatSquare(Square square, char text[3]);

private:
    void clearCastlingRightsTouched(Square square);

    static constexpr char EMPTY = ' ';
    static constexpr char INITIAL_POSITION[8][9] = {
        "RNBQKBNR",
        "PPPPPPPP",
        "        ",
        "        ",
        "        ",
        "        ",
        "pppppppp",
        "rnbqkbnr",
    };

    char board_[8][8];
    Color side_to_move_;
    bool white_kingside_;
    bool white_queenside_;
    bool black_kingside_;
    bool black_queenside_;
    int8_t en_passant_file_;
};
