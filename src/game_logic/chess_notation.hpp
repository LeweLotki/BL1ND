#pragma once

#include "chess_board.hpp"

namespace ChessNotation {

void format(
    const ChessBoard& before,
    const Move& move,
    const ChessBoard& after,
    MoveNotation& notation
);

} // namespace ChessNotation
