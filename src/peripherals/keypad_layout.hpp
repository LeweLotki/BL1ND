#pragma once

namespace KeypadLayout {

constexpr char NO_KEY = '\0';

constexpr char keyAt(unsigned int row, unsigned int column)
{
    return row >= 4 || column >= 4
        ? NO_KEY
        : row == 0 ? "123A"[column]
        : row == 1 ? "456B"[column]
        : row == 2 ? "789C"[column]
                   : "*0#D"[column];
}

} // namespace KeypadLayout
