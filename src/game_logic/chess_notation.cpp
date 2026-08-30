#include "chess_notation.hpp"

#include "chess_rules.hpp"
#include "piece.hpp"

#include <cctype>
#include <cstdio>
#include <cstring>

namespace {

void appendChar(char* text, size_t capacity, char value)
{
    const size_t length = strlen(text);
    if (length + 1 < capacity) {
        text[length] = value;
        text[length + 1] = '\0';
    }
}

void appendText(char* text, size_t capacity, const char* suffix)
{
    const size_t length = strlen(text);
    if (length < capacity) {
        snprintf(text + length, capacity - length, "%s", suffix);
    }
}

void appendDisambiguation(
    const ChessBoard& before,
    const Move& move,
    char* output,
    size_t output_size
)
{
    const char moving = before.pieceAt(move.from);
    const char upper = static_cast<char>(
        std::toupper(static_cast<unsigned char>(moving))
    );
    if (upper == 'P' || upper == 'K') {
        return;
    }

    bool alternative = false;
    bool same_file = false;
    bool same_rank = false;
    for (uint8_t rank = 0; rank < 8; ++rank) {
        for (uint8_t file = 0; file < 8; ++file) {
            const Square from = { file, rank };
            const char candidate = before.pieceAt(from);
            if (from == move.from
                || candidate != moving) {
                continue;
            }
            MoveList moves;
            Piece::forSquare(candidate)->appendMoves(before, from, moves);
            for (size_t index = 0; index < moves.size(); ++index) {
                if (moves[index].to == move.to
                    && !ChessRules::leavesKingInCheck(before, moves[index])) {
                    alternative = true;
                    same_file = same_file || from.file == move.from.file;
                    same_rank = same_rank || from.rank == move.from.rank;
                    break;
                }
            }
        }
    }
    if (!alternative) {
        return;
    }
    if (!same_file) {
        appendChar(
            output,
            output_size,
            static_cast<char>('a' + move.from.file)
        );
    }
    else if (!same_rank) {
        appendChar(
            output,
            output_size,
            static_cast<char>('1' + move.from.rank)
        );
    }
    else {
        appendChar(
            output,
            output_size,
            static_cast<char>('a' + move.from.file)
        );
        appendChar(
            output,
            output_size,
            static_cast<char>('1' + move.from.rank)
        );
    }
}

} // namespace

namespace ChessNotation {

void format(
    const ChessBoard& before,
    const Move& move,
    const ChessBoard& after,
    MoveNotation& notation
)
{
    char from_text[3];
    char to_text[3];
    ChessBoard::formatSquare(move.from, from_text);
    ChessBoard::formatSquare(move.to, to_text);
    snprintf(
        notation.coordinate,
        sizeof(notation.coordinate),
        "%s%s",
        from_text,
        to_text
    );
    notation.algebraic[0] = '\0';

    if (move.kind == MoveKind::CastleKingside) {
        snprintf(
            notation.algebraic,
            sizeof(notation.algebraic),
            "O-O"
        );
    }
    else if (move.kind == MoveKind::CastleQueenside) {
        snprintf(
            notation.algebraic,
            sizeof(notation.algebraic),
            "O-O-O"
        );
    }
    else {
        const char moving = before.pieceAt(move.from);
        const char upper = static_cast<char>(
            std::toupper(static_cast<unsigned char>(moving))
        );
        const bool pawn = upper == 'P';
        const bool capture = move.kind == MoveKind::EnPassant
            || !before.isEmpty(move.to);
        if (pawn) {
            if (capture) {
                appendChar(
                    notation.algebraic,
                    sizeof(notation.algebraic),
                    from_text[0]
                );
            }
        }
        else {
            appendChar(
                notation.algebraic,
                sizeof(notation.algebraic),
                upper
            );
            appendDisambiguation(
                before,
                move,
                notation.algebraic,
                sizeof(notation.algebraic)
            );
        }
        if (capture) {
            appendChar(
                notation.algebraic,
                sizeof(notation.algebraic),
                'x'
            );
        }
        appendText(
            notation.algebraic,
            sizeof(notation.algebraic),
            to_text
        );
        if (move.promotion != '\0') {
            appendChar(
                notation.algebraic,
                sizeof(notation.algebraic),
                '='
            );
            appendChar(
                notation.algebraic,
                sizeof(notation.algebraic),
                move.promotion
            );
        }
    }

    const PositionStatus position_status = ChessRules::status(after);
    if (position_status == PositionStatus::Check) {
        appendChar(
            notation.algebraic,
            sizeof(notation.algebraic),
            '+'
        );
    }
    else if (position_status == PositionStatus::Checkmate) {
        appendChar(
            notation.algebraic,
            sizeof(notation.algebraic),
            '#'
        );
    }
}

} // namespace ChessNotation
