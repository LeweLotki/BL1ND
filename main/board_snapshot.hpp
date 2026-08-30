#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

class ChessBoard;

class BoardSnapshot {
public:
    BoardSnapshot();

    bool publish(const ChessBoard& board);
    bool read(char cells[64]) const;

private:
    struct Cells {
        char values[64];
    };

    QueueHandle_t queue_;
};
