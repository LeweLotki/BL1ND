#pragma once

#include <cstddef>
#include <cstdint>

struct Square {
    uint8_t file;
    uint8_t rank;
};

inline bool operator==(Square left, Square right)
{
    return left.file == right.file && left.rank == right.rank;
}

inline bool operator!=(Square left, Square right)
{
    return !(left == right);
}

inline bool isOnBoard(Square square)
{
    return square.file < 8 && square.rank < 8;
}

enum class Color {
    White,
    Black,
};

inline Color opposite(Color color)
{
    return color == Color::White ? Color::Black : Color::White;
}

inline bool isEmpty(char piece)
{
    return piece == ' ';
}

inline Color colorOf(char piece)
{
    return piece >= 'A' && piece <= 'Z' ? Color::White : Color::Black;
}

enum class MoveKind {
    Normal,
    DoublePawnPush,
    EnPassant,
    CastleKingside,
    CastleQueenside,
};

struct Move {
    Square from;
    Square to;
    MoveKind kind;
    char promotion;
};

class MoveList {
public:
    void append(const Move& move)
    {
        if (size_ < CAPACITY) {
            moves_[size_++] = move;
        }
    }

    size_t size() const { return size_; }
    const Move& operator[](size_t index) const { return moves_[index]; }

private:
    static constexpr size_t CAPACITY = 64;
    Move moves_[CAPACITY] = {};
    size_t size_ = 0;
};
