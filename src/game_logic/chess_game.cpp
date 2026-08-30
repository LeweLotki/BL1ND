#include "chess_game.hpp"

#include "chess_notation.hpp"

#include <cctype>
#include <cstdio>

namespace {

GameEvent emptyEvent(GameEventType type)
{
    return {
        type,
        {},
        0,
        MoveError::None,
        PositionStatus::Normal,
        {},
    };
}

void formatCoordinate(Square from, Square to, MoveNotation& notation)
{
    char from_text[3];
    char to_text[3];
    ChessBoard::formatSquare(from, from_text);
    ChessBoard::formatSquare(to, to_text);
    snprintf(
        notation.coordinate,
        sizeof(notation.coordinate),
        "%s%s",
        from_text,
        to_text
    );
}

} // namespace

ChessGame::ChessGame()
    : digit_count_(0)
    , move_number_(1)
    , promotion_pending_(false)
    , pending_move_({})
    , game_result_(PositionStatus::Normal)
{
}

ChessGame::ChessGame(const ChessBoard& board)
    : board_(board)
    , digit_count_(0)
    , move_number_(1)
    , promotion_pending_(false)
    , pending_move_({})
    , game_result_(PositionStatus::Normal)
{
}

GameEvent ChessGame::handleKey(char key)
{
    if (key == '*') {
        reset();
        return emptyEvent(GameEventType::Reset);
    }

    if (promotion_pending_) {
        char promotion = '\0';
        switch (key) {
        case 'A':
            promotion = 'Q';
            break;
        case 'B':
            promotion = 'R';
            break;
        case 'C':
            promotion = 'B';
            break;
        case 'D':
            promotion = 'N';
            break;
        default:
            return emptyEvent(GameEventType::None);
        }
        promotion_pending_ = false;
        pending_move_.promotion = promotion;
        return finishMove(pending_move_);
    }

    if (key < '1' || key > '8') {
        return emptyEvent(GameEventType::None);
    }

    digits_[digit_count_++] = key;
    if (digit_count_ < 4) {
        return emptyEvent(GameEventType::None);
    }

    digit_count_ = 0;
    return processMove();
}

const ChessBoard& ChessGame::board() const
{
    return board_;
}

bool ChessGame::formatOutput(
    const GameEvent& event,
    char* output,
    size_t output_size
)
{
    if (output_size == 0) {
        return false;
    }

    if (event.type == GameEventType::MoveAccepted) {
        if (event.status == PositionStatus::Checkmate) {
            snprintf(
                output,
                output_size,
                "%s = %u. %s\nCheckmate: %s wins\n",
                event.notation.coordinate,
                event.move_number,
                event.notation.algebraic,
                event.error_description
            );
        }
        else if (event.status == PositionStatus::Stalemate) {
            snprintf(
                output,
                output_size,
                "%s = %u. %s\nStalemate: draw\n",
                event.notation.coordinate,
                event.move_number,
                event.notation.algebraic
            );
        }
        else {
            snprintf(
                output,
                output_size,
                "%s = %u. %s\n",
                event.notation.coordinate,
                event.move_number,
                event.notation.algebraic
            );
        }
        return true;
    }

    if (event.type == GameEventType::MoveRejected) {
        snprintf(
            output,
            output_size,
            "%s = illegal: %s\n",
            event.notation.coordinate,
            event.error_description
        );
        return true;
    }

    if (event.type == GameEventType::PromotionPending) {
        snprintf(
            output,
            output_size,
            "%s = promotion: A=queen B=rook C=bishop D=knight\n",
            event.notation.coordinate
        );
        return true;
    }

    output[0] = '\0';
    return false;
}

GameEvent ChessGame::processMove()
{
    const Square from = {
        ChessBoard::digitToIndex(digits_[0]),
        ChessBoard::digitToIndex(digits_[1]),
    };
    const Square to = {
        ChessBoard::digitToIndex(digits_[2]),
        ChessBoard::digitToIndex(digits_[3]),
    };

    GameEvent event = {
        GameEventType::MoveAccepted,
        {},
        move_number_,
        MoveError::None,
        PositionStatus::Normal,
        {},
    };
    formatCoordinate(from, to, event.notation);

    if (game_result_ == PositionStatus::Checkmate
        || game_result_ == PositionStatus::Stalemate) {
        event.type = GameEventType::MoveRejected;
        event.error = MoveError::GameOver;
        ChessRules::describe(
            event.error,
            board_,
            from,
            to,
            event.error_description,
            sizeof(event.error_description)
        );
        return event;
    }

    const char moving = board_.pieceAt(from);
    const bool promotion_attempt = !board_.isEmpty(from)
        && std::toupper(static_cast<unsigned char>(moving)) == 'P'
        && (to.rank == 0 || to.rank == 7);
    Move resolved = {};
    event.error = ChessRules::validate(
        board_,
        from,
        to,
        promotion_attempt ? 'Q' : '\0',
        resolved
    );
    if (event.error != MoveError::None) {
        event.type = GameEventType::MoveRejected;
        ChessRules::describe(
            event.error,
            board_,
            from,
            to,
            event.error_description,
            sizeof(event.error_description)
        );
        return event;
    }

    if (resolved.promotion != '\0') {
        promotion_pending_ = true;
        pending_move_ = resolved;
        event.type = GameEventType::PromotionPending;
        return event;
    }

    return finishMove(resolved);
}

GameEvent ChessGame::finishMove(Move move)
{
    ChessBoard before = board_;
    board_.apply(move);

    GameEvent event = {
        GameEventType::MoveAccepted,
        {},
        move_number_,
        MoveError::None,
        ChessRules::status(board_),
        {},
    };
    ChessNotation::format(before, move, board_, event.notation);
    if (event.status == PositionStatus::Checkmate) {
        snprintf(
            event.error_description,
            sizeof(event.error_description),
            "%s",
            before.sideToMove() == Color::White ? "White" : "Black"
        );
        game_result_ = event.status;
    }
    else if (event.status == PositionStatus::Stalemate) {
        game_result_ = event.status;
    }
    ++move_number_;
    return event;
}

void ChessGame::reset()
{
    board_.reset();
    digit_count_ = 0;
    move_number_ = 1;
    promotion_pending_ = false;
    pending_move_ = {};
    game_result_ = PositionStatus::Normal;
}
