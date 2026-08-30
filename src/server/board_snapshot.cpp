#include "board_snapshot.hpp"

#include "chess_board.hpp"

#include <cstring>

BoardSnapshot::BoardSnapshot()
    : queue_(xQueueCreate(1, sizeof(Cells)))
{
}

bool BoardSnapshot::publish(const ChessBoard& board)
{
    if (queue_ == nullptr) {
        return false;
    }

    Cells snapshot;
    for (uint8_t rank = 0; rank < 8; ++rank) {
        for (uint8_t file = 0; file < 8; ++file) {
            snapshot.values[rank * 8 + file] = board.pieceAt({
                file,
                rank,
            });
        }
    }

    return xQueueOverwrite(queue_, &snapshot) == pdTRUE;
}

bool BoardSnapshot::read(char cells[64]) const
{
    if (queue_ == nullptr || cells == nullptr) {
        return false;
    }

    Cells snapshot;
    if (xQueuePeek(queue_, &snapshot, 0) != pdTRUE) {
        return false;
    }

    memcpy(cells, snapshot.values, sizeof(snapshot.values));
    return true;
}
