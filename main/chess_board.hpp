#pragma once

#include <cstdint>

struct Square {
    uint8_t file;
    uint8_t rank;
};

struct MoveNotation {
    char coordinate[5];
    char algebraic[8];
};

class ChessBoard {
public:
    ChessBoard();

    void reset();

    char pieceAt(Square square) const;
    bool formatMove(Square from, Square to, MoveNotation& notation) const;
    void applyMove(Square from, Square to);

    static uint8_t digitToIndex(char digit);
    static void formatSquare(Square square, char text[3]);

private:
    bool isCastling(Square from, Square to, char piece) const;

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
};
