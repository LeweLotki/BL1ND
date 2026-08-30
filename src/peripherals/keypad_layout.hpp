#pragma once

namespace KeypadLayout {

constexpr char NO_KEY = '\0';
constexpr char HOLD_B = '\x01';
constexpr unsigned int HOLD_SCANS = 150;

constexpr char keyAt(unsigned int row, unsigned int column)
{
    return row >= 4 || column >= 4
        ? NO_KEY
        : row == 0 ? "123A"[column]
        : row == 1 ? "456B"[column]
        : row == 2 ? "789C"[column]
                   : "*0#D"[column];
}

class InputTracker {
public:
    constexpr InputTracker()
        : current_(NO_KEY)
        , scans_(0)
        , hold_reported_(false)
    {
    }

    constexpr char update(char key)
    {
        if (key == current_) {
            if (key == 'B' && !hold_reported_) {
                ++scans_;
                if (scans_ >= HOLD_SCANS) {
                    hold_reported_ = true;
                    return HOLD_B;
                }
            }
            return NO_KEY;
        }

        if (current_ == 'B' && key == NO_KEY && !hold_reported_) {
            current_ = NO_KEY;
            scans_ = 0;
            return 'B';
        }

        current_ = key;
        scans_ = key == NO_KEY ? 0U : 1U;
        hold_reported_ = false;
        return key == 'B' ? NO_KEY : key;
    }

private:
    char current_;
    unsigned int scans_;
    bool hold_reported_;
};

} // namespace KeypadLayout
