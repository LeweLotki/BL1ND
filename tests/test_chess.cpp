#include "board_page.hpp"
#include "chess_game.hpp"
#include "chess_notation.hpp"
#include "chess_rules.hpp"
#include "keypad_layout.hpp"
#include "piece.hpp"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            throw std::runtime_error(                                          \
                std::string(__func__) + ":" + std::to_string(__LINE__)        \
                + ": CHECK(" #condition ") failed"                            \
            );                                                                 \
        }                                                                      \
    } while (false)

Square square(char file, char rank)
{
    return {
        static_cast<uint8_t>(file - 'a'),
        static_cast<uint8_t>(rank - '1'),
    };
}

ChessBoard position(
    const char* r1,
    const char* r2,
    const char* r3,
    const char* r4,
    const char* r5,
    const char* r6,
    const char* r7,
    const char* r8,
    Color side = Color::White,
    bool wk = false,
    bool wq = false,
    bool bk = false,
    bool bq = false,
    int ep_file = -1
)
{
    const char* ranks[] = { r1, r2, r3, r4, r5, r6, r7, r8 };
    ChessBoard board;
    board.loadPosition(ranks, side, wk, wq, bk, bq, ep_file);
    return board;
}

GameEvent enter(ChessGame& game, const char* keys)
{
    GameEvent event = {};
    for (const char* key = keys; *key != '\0'; ++key) {
        event = game.handleKey(*key);
    }
    return event;
}

Move legalMove(
    const ChessBoard& board,
    const char* from,
    const char* to,
    char promotion = '\0'
)
{
    Move move = {};
    CHECK(
        ChessRules::validate(
            board,
            square(from[0], from[1]),
            square(to[0], to[1]),
            promotion,
            move
        ) == MoveError::None
    );
    return move;
}

MoveError errorFor(
    const ChessBoard& board,
    const char* from,
    const char* to,
    char promotion = '\0'
)
{
    Move move = {};
    return ChessRules::validate(
        board,
        square(from[0], from[1]),
        square(to[0], to[1]),
        promotion,
        move
    );
}

bool containsMove(const MoveList& moves, Square target)
{
    for (size_t index = 0; index < moves.size(); ++index) {
        if (moves[index].to == target) {
            return true;
        }
    }
    return false;
}

void copyBoard(const ChessBoard& board, char cells[64])
{
    for (uint8_t rank = 0; rank < 8; ++rank) {
        for (uint8_t file = 0; file < 8; ++file) {
            cells[rank * 8 + file] = board.pieceAt({ file, rank });
        }
    }
}

size_t countText(const std::string& text, const std::string& needle)
{
    size_t count = 0;
    size_t position_index = 0;
    while ((position_index = text.find(
                needle,
                position_index
            )) != std::string::npos) {
        ++count;
        position_index += needle.size();
    }
    return count;
}

void testKeypadAndInitialPosition()
{
    constexpr char expected[4][5] = {
        "123A",
        "456B",
        "789C",
        "*0#D",
    };
    for (unsigned int row = 0; row < 4; ++row) {
        for (unsigned int column = 0; column < 4; ++column) {
            CHECK(KeypadLayout::keyAt(row, column) == expected[row][column]);
        }
    }
    CHECK(KeypadLayout::keyAt(4, 0) == KeypadLayout::NO_KEY);

    ChessBoard board;
    CHECK(board.pieceAt(square('a', '1')) == 'R');
    CHECK(board.pieceAt(square('e', '1')) == 'K');
    CHECK(board.pieceAt(square('e', '8')) == 'k');
    CHECK(board.sideToMove() == Color::White);
    CHECK(board.hasCastlingRight(Color::White, MoveKind::CastleKingside));
    CHECK(!isOnBoard(board.enPassantTarget()));
}

void testKnightMovement()
{
    ChessBoard central_knight = position(
        "    K   ",
        "        ",
        "        ",
        "   N    ",
        "        ",
        "        ",
        "        ",
        "    k   "
    );
    MoveList moves;
    Piece::forSquare('N')->appendMoves(
        central_knight,
        square('d', '4'),
        moves
    );
    CHECK(moves.size() == 8);

    ChessBoard corner_knight = position(
        "N   K   ",
        "        ",
        "        ",
        "        ",
        "        ",
        "        ",
        "        ",
        "       k"
    );
    MoveList corner_moves;
    Piece::forSquare('N')->appendMoves(
        corner_knight,
        square('a', '1'),
        corner_moves
    );
    CHECK(corner_moves.size() == 2);
    CHECK(containsMove(corner_moves, square('b', '3')));
    CHECK(containsMove(corner_moves, square('c', '2')));

    ChessBoard surrounded_knight = position(
        "K       ",
        "        ",
        "  PPP   ",
        "  PNP   ",
        "  PPP   ",
        "        ",
        "        ",
        "       k"
    );
    MoveList surrounded_moves;
    Piece::forSquare('N')->appendMoves(
        surrounded_knight,
        square('d', '4'),
        surrounded_moves
    );
    CHECK(surrounded_moves.size() == 8);
}

void testSliderMovement()
{
    ChessBoard bishop_board = position(
        "       K",
        " p      ",
        "        ",
        "   B    ",
        "        ",
        "     P  ",
        "        ",
        "       k"
    );
    MoveList bishop_moves;
    Piece::forSquare('B')->appendMoves(
        bishop_board,
        square('d', '4'),
        bishop_moves
    );
    CHECK(containsMove(bishop_moves, square('b', '2')));
    CHECK(!containsMove(bishop_moves, square('a', '1')));
    CHECK(!containsMove(bishop_moves, square('f', '6')));
    CHECK(!containsMove(bishop_moves, square('g', '7')));

    ChessBoard rook_board = position(
        "       K",
        "        ",
        "        ",
        " p R    ",
        "        ",
        "   P    ",
        "        ",
        "       k"
    );
    MoveList rook_moves;
    Piece::forSquare('R')->appendMoves(
        rook_board,
        square('d', '4'),
        rook_moves
    );
    CHECK(containsMove(rook_moves, square('b', '4')));
    CHECK(!containsMove(rook_moves, square('a', '4')));
    CHECK(!containsMove(rook_moves, square('d', '6')));
    CHECK(!containsMove(rook_moves, square('d', '7')));

    ChessBoard queen_board = position(
        "       K",
        "        ",
        "        ",
        "   Q P  ",
        "        ",
        "     p  ",
        "        ",
        "       k"
    );
    MoveList queen_moves;
    Piece::forSquare('Q')->appendMoves(
        queen_board,
        square('d', '4'),
        queen_moves
    );
    CHECK(!containsMove(queen_moves, square('f', '4')));
    CHECK(!containsMove(queen_moves, square('g', '4')));
    CHECK(containsMove(queen_moves, square('f', '6')));
    CHECK(!containsMove(queen_moves, square('g', '7')));
}

void testPawnAndKingMovement()
{
    ChessBoard pawn_board = position(
        "    K   ",
        "    P   ",
        "        ",
        "        ",
        "        ",
        "        ",
        "    p   ",
        "    k   "
    );
    CHECK(errorFor(pawn_board, "e2", "e4") == MoveError::None);
    CHECK(errorFor(pawn_board, "e2", "e3") == MoveError::None);
    CHECK(errorFor(pawn_board, "e2", "e5") == MoveError::Unreachable);
    CHECK(errorFor(pawn_board, "e2", "d3")
        == MoveError::EnPassantUnavailable);

    ChessBoard white_non_home = position(
        "K       ",
        "        ",
        "        ",
        "    P   ",
        "        ",
        "        ",
        "        ",
        "       k"
    );
    CHECK(errorFor(white_non_home, "e4", "e6") == MoveError::Unreachable);
    CHECK(errorFor(white_non_home, "e4", "e3") == MoveError::Unreachable);

    ChessBoard white_blocked = position(
        "K       ",
        "    P   ",
        "    P   ",
        "        ",
        "        ",
        "        ",
        "        ",
        "       k"
    );
    CHECK(errorFor(white_blocked, "e2", "e4") == MoveError::PathBlocked);

    ChessBoard white_capture = position(
        "K       ",
        "    P   ",
        "   ppp  ",
        "        ",
        "        ",
        "        ",
        "        ",
        "       k"
    );
    CHECK(errorFor(white_capture, "e2", "e3") == MoveError::Unreachable);
    CHECK(errorFor(white_capture, "e2", "d3") == MoveError::None);

    ChessBoard black_pawn = position(
        "K       ",
        "        ",
        "        ",
        "        ",
        "        ",
        "        ",
        "    p   ",
        "       k",
        Color::Black
    );
    CHECK(errorFor(black_pawn, "e7", "e6") == MoveError::None);
    CHECK(errorFor(black_pawn, "e7", "e5") == MoveError::None);
    CHECK(errorFor(black_pawn, "e7", "e8") == MoveError::Unreachable);
    CHECK(errorFor(black_pawn, "e7", "d6")
        == MoveError::EnPassantUnavailable);

    ChessBoard black_non_home = position(
        "K       ",
        "        ",
        "        ",
        "        ",
        "    p   ",
        "        ",
        "        ",
        "       k",
        Color::Black
    );
    CHECK(errorFor(black_non_home, "e5", "e3") == MoveError::Unreachable);
    CHECK(errorFor(black_non_home, "e5", "e6") == MoveError::Unreachable);

    ChessBoard black_blocked = position(
        "K       ",
        "        ",
        "        ",
        "        ",
        "        ",
        "    p   ",
        "    p   ",
        "       k",
        Color::Black
    );
    CHECK(errorFor(black_blocked, "e7", "e5") == MoveError::PathBlocked);

    ChessBoard black_capture = position(
        "K       ",
        "        ",
        "        ",
        "        ",
        "        ",
        "   PPP  ",
        "    p   ",
        "       k",
        Color::Black
    );
    CHECK(errorFor(black_capture, "e7", "e6") == MoveError::Unreachable);
    CHECK(errorFor(black_capture, "e7", "d6") == MoveError::None);

    CHECK(Piece::forSquare('P')->attacks(
        pawn_board,
        square('e', '2'),
        square('d', '3')
    ));
    CHECK(!Piece::forSquare('P')->attacks(
        pawn_board,
        square('e', '2'),
        square('e', '3')
    ));
    CHECK(Piece::forSquare('p')->attacks(
        black_pawn,
        square('e', '7'),
        square('f', '6')
    ));
    CHECK(!Piece::forSquare('p')->attacks(
        black_pawn,
        square('e', '7'),
        square('e', '6')
    ));
    CHECK(Piece::forSquare('K')->attacks(
        pawn_board,
        square('e', '1'),
        square('d', '2')
    ));
    constexpr Square KING_TARGETS[] = {
        { 2, 2 }, { 3, 2 }, { 4, 2 }, { 2, 3 },
        { 4, 3 }, { 2, 4 }, { 3, 4 }, { 4, 4 },
    };
    for (Square target : KING_TARGETS) {
        CHECK(Piece::forSquare('K')->attacks(
            pawn_board,
            square('d', '4'),
            target
        ));
    }
    CHECK(!Piece::forSquare('K')->attacks(
        pawn_board,
        square('e', '1'),
        square('g', '1')
    ));
}

void testEntryTurnOrderAndRejections()
{
    ChessGame first_move;
    const GameEvent black_first = enter(first_move, "5755");
    CHECK(black_first.type == GameEventType::MoveRejected);
    CHECK(black_first.error == MoveError::NotYourPiece);
    CHECK(std::strcmp(
        black_first.error_description,
        "it is White's turn"
    ) == 0);

    ChessGame game;
    char output[96];

    CHECK(game.handleKey('5').type == GameEventType::None);
    CHECK(game.handleKey('9').type == GameEventType::None);
    CHECK(game.handleKey('2').type == GameEventType::None);
    CHECK(game.handleKey('A').type == GameEventType::None);
    CHECK(game.handleKey('5').type == GameEventType::None);
    GameEvent event = game.handleKey('4');
    CHECK(event.type == GameEventType::MoveAccepted);
    CHECK(std::strcmp(event.notation.coordinate, "e2e4") == 0);
    CHECK(std::strcmp(event.notation.algebraic, "e4") == 0);
    CHECK(event.move_number == 1);
    CHECK(ChessGame::formatOutput(event, output, sizeof(output)));
    CHECK(std::strcmp(output, "e2e4 = 1. e4\n") == 0);

    event = enter(game, "7163");
    CHECK(event.type == GameEventType::MoveRejected);
    CHECK(event.error == MoveError::NotYourPiece);
    CHECK(std::strcmp(event.error_description, "it is Black's turn") == 0);

    event = enter(game, "5755");
    CHECK(event.type == GameEventType::MoveAccepted);
    CHECK(event.move_number == 2);
    CHECK(enter(game, "1115").error == MoveError::PathBlocked);
    CHECK(game.board().pieceAt(square('a', '1')) == 'R');

    event = enter(game, "5252");
    CHECK(event.error == MoveError::EmptyFromSquare);
    game.handleKey('*');
    event = enter(game, "5252");
    CHECK(event.error == MoveError::SameSquare);
    CHECK(enter(game, "1112").error == MoveError::FriendlyPiece);
    CHECK(enter(game, "4185").error == MoveError::PathBlocked);
}

void testKingSafetyAndPins()
{
    ChessBoard pinned = position(
        "    K   ",
        "    N   ",
        "        ",
        "        ",
        "        ",
        "        ",
        "        ",
        "k   r   "
    );
    CHECK(errorFor(pinned, "e2", "c3") == MoveError::KingLeftInCheck);

    ChessBoard pinned_rook = position(
        "    K   ",
        "    R   ",
        "        ",
        "        ",
        "        ",
        "        ",
        "        ",
        "k   r   "
    );
    CHECK(errorFor(pinned_rook, "e2", "e5") == MoveError::None);
    CHECK(errorFor(pinned_rook, "e2", "e8") == MoveError::None);

    ChessBoard kings = position(
        "    K   ",
        "        ",
        "    k   ",
        "        ",
        "        ",
        "        ",
        "        ",
        "        "
    );
    CHECK(errorFor(kings, "e1", "e2") == MoveError::KingLeftInCheck);

    ChessBoard checking_ray = position(
        "r   K   ",
        "        ",
        "        ",
        "        ",
        "        ",
        "        ",
        "        ",
        "       k"
    );
    CHECK(errorFor(checking_ray, "e1", "d1")
        == MoveError::KingLeftInCheck);

    ChessBoard unrelated = position(
        "    K   ",
        "N       ",
        "        ",
        "        ",
        "        ",
        "        ",
        "        ",
        "k   r   "
    );
    CHECK(errorFor(unrelated, "a2", "b4") == MoveError::KingLeftInCheck);

    ChessBoard block = position(
        "    K   ",
        "R       ",
        "        ",
        "        ",
        "        ",
        "        ",
        "        ",
        "k   r   "
    );
    CHECK(errorFor(block, "a2", "e2") == MoveError::None);

    ChessBoard capture_checker = position(
        "    K   ",
        "        ",
        "        ",
        "        ",
        " B      ",
        "        ",
        "        ",
        "k   r   "
    );
    CHECK(errorFor(capture_checker, "b5", "e8") == MoveError::None);
}

void testCastling()
{
    ChessBoard board = position(
        "R   K  R",
        "        ",
        "        ",
        "        ",
        "        ",
        "        ",
        "        ",
        "r   k  r",
        Color::White,
        true,
        true,
        true,
        true
    );
    Move move = legalMove(board, "e1", "g1");
    CHECK(move.kind == MoveKind::CastleKingside);
    board.apply(move);
    CHECK(board.pieceAt(square('g', '1')) == 'K');
    CHECK(board.pieceAt(square('f', '1')) == 'R');

    move = legalMove(board, "e8", "c8");
    board.apply(move);
    CHECK(board.pieceAt(square('c', '8')) == 'k');
    CHECK(board.pieceAt(square('d', '8')) == 'r');

    ChessBoard white_queenside = position(
        "R   K  R",
        "        ",
        "        ",
        "        ",
        "        ",
        "        ",
        "        ",
        "r   k  r",
        Color::White,
        true,
        true,
        true,
        true
    );
    move = legalMove(white_queenside, "e1", "c1");
    white_queenside.apply(move);
    CHECK(white_queenside.pieceAt(square('c', '1')) == 'K');
    CHECK(white_queenside.pieceAt(square('d', '1')) == 'R');

    ChessBoard black_kingside = position(
        "R   K  R",
        "        ",
        "        ",
        "        ",
        "        ",
        "        ",
        "        ",
        "r   k  r",
        Color::Black,
        true,
        true,
        true,
        true
    );
    move = legalMove(black_kingside, "e8", "g8");
    black_kingside.apply(move);
    CHECK(black_kingside.pieceAt(square('g', '8')) == 'k');
    CHECK(black_kingside.pieceAt(square('f', '8')) == 'r');

    ChessBoard blocked;
    CHECK(errorFor(blocked, "e1", "g1") == MoveError::CastlingPathBlocked);

    ChessBoard through_check = position(
        "R   K  R",
        "        ",
        "        ",
        "        ",
        "        ",
        "        ",
        "     r  ",
        "k       ",
        Color::White,
        true
    );
    CHECK(errorFor(through_check, "e1", "g1")
        == MoveError::CastlingThroughCheck);

    ChessBoard out_of_check = position(
        "R   K  R",
        "        ",
        "        ",
        "        ",
        "        ",
        "        ",
        "        ",
        "k   r   ",
        Color::White,
        true
    );
    CHECK(errorFor(out_of_check, "e1", "g1")
        == MoveError::CastlingThroughCheck);

    ChessBoard into_check = position(
        "R   K  R",
        "        ",
        "        ",
        "        ",
        "        ",
        "        ",
        "        ",
        "k     r ",
        Color::White,
        true
    );
    CHECK(errorFor(into_check, "e1", "g1")
        == MoveError::CastlingThroughCheck);

    ChessBoard rights = position(
        "R   K  R",
        "        ",
        "        ",
        "        ",
        "        ",
        "        ",
        "        ",
        "    k   "
    );
    CHECK(errorFor(rights, "e1", "g1") == MoveError::CastlingRightsLost);

    ChessBoard spent = position(
        "R   K  R",
        "        ",
        "        ",
        "        ",
        "        ",
        "        ",
        "        ",
        "r   k  r",
        Color::White,
        true,
        true,
        true,
        true
    );
    spent.apply({ square('e', '1'), square('f', '1'), MoveKind::Normal, '\0' });
    spent.apply({ square('e', '8'), square('f', '8'), MoveKind::Normal, '\0' });
    spent.apply({ square('f', '1'), square('e', '1'), MoveKind::Normal, '\0' });
    spent.apply({ square('f', '8'), square('e', '8'), MoveKind::Normal, '\0' });
    CHECK(errorFor(spent, "e1", "g1") == MoveError::CastlingRightsLost);
    CHECK(!spent.hasCastlingRight(Color::White, MoveKind::CastleQueenside));

    ChessBoard rook_returned = position(
        "R   K  R",
        "        ",
        "        ",
        "        ",
        "        ",
        "        ",
        "        ",
        "r   k  r",
        Color::White,
        true,
        true,
        true,
        true
    );
    rook_returned.apply({
        square('h', '1'),
        square('h', '2'),
        MoveKind::Normal,
        '\0',
    });
    rook_returned.apply({
        square('e', '8'),
        square('f', '8'),
        MoveKind::Normal,
        '\0',
    });
    rook_returned.apply({
        square('h', '2'),
        square('h', '1'),
        MoveKind::Normal,
        '\0',
    });
    rook_returned.apply({
        square('f', '8'),
        square('e', '8'),
        MoveKind::Normal,
        '\0',
    });
    CHECK(errorFor(rook_returned, "e1", "g1")
        == MoveError::CastlingRightsLost);
    CHECK(rook_returned.hasCastlingRight(
        Color::White,
        MoveKind::CastleQueenside
    ));

    ChessBoard captured = position(
        "R   K  R",
        "        ",
        "        ",
        "        ",
        "        ",
        "        ",
        "        ",
        "r   k  r",
        Color::Black,
        true,
        true,
        true,
        true
    );
    captured.apply({
        square('h', '8'),
        square('h', '1'),
        MoveKind::Normal,
        '\0',
    });
    CHECK(!captured.hasCastlingRight(
        Color::White,
        MoveKind::CastleKingside
    ));
    CHECK(errorFor(captured, "e1", "g1") == MoveError::CastlingRightsLost);

    ChessBoard b_attacked = position(
        "R   K   ",
        "        ",
        "        ",
        "        ",
        "        ",
        "        ",
        " r      ",
        "       k",
        Color::White,
        false,
        true
    );
    CHECK(errorFor(b_attacked, "e1", "c1") == MoveError::None);

    ChessBoard b_occupied = position(
        "RN  K   ",
        "        ",
        "        ",
        "        ",
        "        ",
        "        ",
        "        ",
        "       k",
        Color::White,
        false,
        true
    );
    CHECK(errorFor(b_occupied, "e1", "c1")
        == MoveError::CastlingPathBlocked);
}

void testEnPassantAndStatePreservation()
{
    ChessGame game;
    CHECK(enter(game, "5254").type == GameEventType::MoveAccepted);
    CHECK(enter(game, "1716").type == GameEventType::MoveAccepted);
    CHECK(enter(game, "5455").type == GameEventType::MoveAccepted);
    CHECK(enter(game, "4745").type == GameEventType::MoveAccepted);
    CHECK(game.board().enPassantTarget() == square('d', '6'));

    char before_rejection[64];
    copyBoard(game.board(), before_rejection);
    const Color side_before = game.board().sideToMove();
    const Square ep_before = game.board().enPassantTarget();
    const bool wk_before = game.board().hasCastlingRight(
        Color::White,
        MoveKind::CastleKingside
    );
    const bool wq_before = game.board().hasCastlingRight(
        Color::White,
        MoveKind::CastleQueenside
    );
    const bool bk_before = game.board().hasCastlingRight(
        Color::Black,
        MoveKind::CastleKingside
    );
    const bool bq_before = game.board().hasCastlingRight(
        Color::Black,
        MoveKind::CastleQueenside
    );
    const GameEvent bad = enter(game, "1115");
    CHECK(bad.type == GameEventType::MoveRejected);
    CHECK(bad.move_number == 5);
    char after_rejection[64];
    copyBoard(game.board(), after_rejection);
    CHECK(std::memcmp(
        before_rejection,
        after_rejection,
        sizeof(before_rejection)
    ) == 0);
    CHECK(game.board().sideToMove() == side_before);
    CHECK(game.board().enPassantTarget() == ep_before);
    CHECK(game.board().hasCastlingRight(
        Color::White,
        MoveKind::CastleKingside
    ) == wk_before);
    CHECK(game.board().hasCastlingRight(
        Color::White,
        MoveKind::CastleQueenside
    ) == wq_before);
    CHECK(game.board().hasCastlingRight(
        Color::Black,
        MoveKind::CastleKingside
    ) == bk_before);
    CHECK(game.board().hasCastlingRight(
        Color::Black,
        MoveKind::CastleQueenside
    ) == bq_before);

    const GameEvent capture = enter(game, "5546");
    CHECK(capture.type == GameEventType::MoveAccepted);
    CHECK(capture.move_number == 5);
    CHECK(std::strcmp(capture.notation.algebraic, "exd6") == 0);
    CHECK(game.board().pieceAt(square('d', '5')) == ' ');
    CHECK(game.board().pieceAt(square('d', '6')) == 'P');

    ChessBoard exposed = position(
        "        ",
        "        ",
        "        ",
        "        ",
        "r    PpK",
        "        ",
        "        ",
        "k       ",
        Color::White,
        false,
        false,
        false,
        false,
        6
    );
    CHECK(errorFor(exposed, "f5", "g6") == MoveError::KingLeftInCheck);

    ChessGame expired;
    CHECK(enter(expired, "5254").type == GameEventType::MoveAccepted);
    CHECK(enter(expired, "1716").type == GameEventType::MoveAccepted);
    CHECK(enter(expired, "5455").type == GameEventType::MoveAccepted);
    CHECK(enter(expired, "4745").type == GameEventType::MoveAccepted);
    CHECK(enter(expired, "7163").type == GameEventType::MoveAccepted);
    CHECK(enter(expired, "1615").type == GameEventType::MoveAccepted);
    CHECK(enter(expired, "5546").error == MoveError::EnPassantUnavailable);

    ChessGame single_step;
    CHECK(enter(single_step, "5254").type == GameEventType::MoveAccepted);
    CHECK(enter(single_step, "4746").type == GameEventType::MoveAccepted);
    CHECK(enter(single_step, "5455").type == GameEventType::MoveAccepted);
    CHECK(enter(single_step, "4645").type == GameEventType::MoveAccepted);
    CHECK(enter(single_step, "5546").error == MoveError::EnPassantUnavailable);

    ChessGame black_capture;
    CHECK(enter(black_capture, "1213").type == GameEventType::MoveAccepted);
    CHECK(enter(black_capture, "4745").type == GameEventType::MoveAccepted);
    CHECK(enter(black_capture, "1314").type == GameEventType::MoveAccepted);
    CHECK(enter(black_capture, "4544").type == GameEventType::MoveAccepted);
    CHECK(enter(black_capture, "5254").type == GameEventType::MoveAccepted);
    const GameEvent black_ep = enter(black_capture, "4453");
    CHECK(black_ep.type == GameEventType::MoveAccepted);
    CHECK(std::strcmp(black_ep.notation.algebraic, "dxe3") == 0);
    CHECK(black_capture.board().pieceAt(square('d', '4')) == ' ');
    CHECK(black_capture.board().pieceAt(square('e', '4')) == ' ');
    CHECK(black_capture.board().pieceAt(square('e', '3')) == 'p');
}

GameEvent advanceToWhitePromotion(ChessGame& game)
{
    CHECK(enter(game, "1214").type == GameEventType::MoveAccepted);
    CHECK(enter(game, "8786").type == GameEventType::MoveAccepted);
    CHECK(enter(game, "1415").type == GameEventType::MoveAccepted);
    CHECK(enter(game, "8685").type == GameEventType::MoveAccepted);
    CHECK(enter(game, "1516").type == GameEventType::MoveAccepted);
    CHECK(enter(game, "8584").type == GameEventType::MoveAccepted);
    CHECK(enter(game, "1627").type == GameEventType::MoveAccepted);
    CHECK(enter(game, "8483").type == GameEventType::MoveAccepted);
    char before[64];
    copyBoard(game.board(), before);
    const GameEvent pending = enter(game, "2718");
    CHECK(pending.type == GameEventType::PromotionPending);
    char after[64];
    copyBoard(game.board(), after);
    CHECK(std::memcmp(before, after, sizeof(before)) == 0);
    return pending;
}

void testPromotionFlow()
{
    constexpr char KEYS[] = { 'A', 'B', 'C', 'D' };
    constexpr char PIECES[] = { 'Q', 'R', 'B', 'N' };
    for (size_t index = 0; index < 4; ++index) {
        ChessGame game;
        const GameEvent pending = advanceToWhitePromotion(game);
        CHECK(game.board().pieceAt(square('b', '7')) == 'P');
        CHECK(game.board().pieceAt(square('a', '8')) == 'r');
        char prompt[96];
        CHECK(ChessGame::formatOutput(pending, prompt, sizeof(prompt)));
        CHECK(std::strstr(prompt, "A=queen B=rook C=bishop D=knight")
            != nullptr);
        CHECK(game.handleKey('1').type == GameEventType::None);
        const GameEvent event = game.handleKey(KEYS[index]);
        CHECK(event.type == GameEventType::MoveAccepted);
        CHECK(game.board().pieceAt(square('a', '8')) == PIECES[index]);
        char expected[] = "bxa8=Q";
        expected[5] = PIECES[index];
        CHECK(std::strcmp(event.notation.algebraic, expected) == 0);
        CHECK(event.move_number == 9);
    }

    ChessGame reset_game;
    advanceToWhitePromotion(reset_game);
    reset_game.handleKey('*');
    CHECK(reset_game.board().pieceAt(square('a', '2')) == 'P');
    CHECK(enter(reset_game, "5254").move_number == 1);

    ChessBoard black = position(
        "       K",
        "    p   ",
        "        ",
        "        ",
        "        ",
        "        ",
        "        ",
        "k       ",
        Color::Black
    );
    Move promotion = legalMove(black, "e2", "e1", 'R');
    ChessBoard after = black;
    after.apply(promotion);
    MoveNotation notation = {};
    ChessNotation::format(black, promotion, after, notation);
    CHECK(after.pieceAt(square('e', '1')) == 'r');
    CHECK(std::strcmp(notation.algebraic, "e1=R+") == 0);

    ChessBoard blocked_promotion = position(
        "K       ",
        "        ",
        "        ",
        "        ",
        "        ",
        "        ",
        "    P   ",
        "    r  k"
    );
    ChessGame illegal(blocked_promotion);
    const GameEvent rejected = enter(illegal, "5758");
    CHECK(rejected.type == GameEventType::MoveRejected);
    CHECK(illegal.board().pieceAt(square('e', '7')) == 'P');
    CHECK(illegal.board().pieceAt(square('e', '8')) == 'r');
    CHECK(illegal.handleKey('A').type == GameEventType::None);
}

void testBasicNotation()
{
    ChessGame game;
    CHECK(std::strcmp(
        enter(game, "5254").notation.algebraic,
        "e4"
    ) == 0);
    CHECK(std::strcmp(
        enter(game, "7866").notation.algebraic,
        "Nf6"
    ) == 0);
    CHECK(std::strcmp(
        enter(game, "7163").notation.algebraic,
        "Nf3"
    ) == 0);

    ChessBoard bishop_capture = position(
        "K       ",
        "        ",
        "        ",
        "  B     ",
        "        ",
        "        ",
        "     p  ",
        "       k"
    );
    Move move = legalMove(bishop_capture, "c4", "f7");
    ChessBoard after = bishop_capture;
    after.apply(move);
    MoveNotation notation = {};
    ChessNotation::format(bishop_capture, move, after, notation);
    CHECK(std::strcmp(notation.algebraic, "Bxf7") == 0);

    ChessBoard pawn_capture = position(
        "K       ",
        "        ",
        "        ",
        "    P   ",
        "   p    ",
        "        ",
        "        ",
        "       k"
    );
    move = legalMove(pawn_capture, "e4", "d5");
    after = pawn_capture;
    after.apply(move);
    ChessNotation::format(pawn_capture, move, after, notation);
    CHECK(std::strcmp(notation.algebraic, "exd5") == 0);

    ChessBoard kingside = position(
        "    K  R",
        "        ",
        "        ",
        "        ",
        "        ",
        "        ",
        "        ",
        "k       ",
        Color::White,
        true
    );
    move = legalMove(kingside, "e1", "g1");
    after = kingside;
    after.apply(move);
    ChessNotation::format(kingside, move, after, notation);
    CHECK(std::strcmp(notation.algebraic, "O-O") == 0);

    ChessBoard queenside = position(
        "R   K   ",
        "        ",
        "        ",
        "        ",
        "        ",
        "        ",
        "        ",
        "       k",
        Color::White,
        false,
        true
    );
    move = legalMove(queenside, "e1", "c1");
    after = queenside;
    after.apply(move);
    ChessNotation::format(queenside, move, after, notation);
    CHECK(std::strcmp(notation.algebraic, "O-O-O") == 0);
}

void testNotationDisambiguationAndStatus()
{
    ChessBoard before = position(
        " N  K   ",
        "        ",
        "     N  ",
        "        ",
        "        ",
        "        ",
        "        ",
        "    k   "
    );
    Move move = legalMove(before, "b1", "d2");
    ChessBoard after = before;
    after.apply(move);
    MoveNotation notation = {};
    ChessNotation::format(before, move, after, notation);
    CHECK(std::strcmp(notation.algebraic, "Nbd2") == 0);

    ChessBoard capture_disambiguation = position(
        " N  K   ",
        "   p    ",
        "     N  ",
        "        ",
        "        ",
        "        ",
        "        ",
        "    k   "
    );
    move = legalMove(capture_disambiguation, "b1", "d2");
    after = capture_disambiguation;
    after.apply(move);
    ChessNotation::format(capture_disambiguation, move, after, notation);
    CHECK(std::strcmp(notation.algebraic, "Nbxd2") == 0);

    ChessBoard rank_disambiguation = position(
        "    R  K",
        "        ",
        "        ",
        "        ",
        "    R   ",
        "        ",
        "        ",
        "       k"
    );
    move = legalMove(rank_disambiguation, "e1", "e3");
    after = rank_disambiguation;
    after.apply(move);
    ChessNotation::format(rank_disambiguation, move, after, notation);
    CHECK(std::strcmp(notation.algebraic, "R1e3") == 0);

    ChessBoard full_disambiguation = position(
        " K      ",
        "        ",
        "        ",
        "Q      Q",
        "        ",
        "        ",
        "      k ",
        "Q       "
    );
    move = legalMove(full_disambiguation, "a4", "e4");
    after = full_disambiguation;
    after.apply(move);
    ChessNotation::format(full_disambiguation, move, after, notation);
    CHECK(std::strcmp(notation.algebraic, "Qa4e4") == 0);

    ChessBoard pinned_twin = position(
        "    K   ",
        "  N N   ",
        "        ",
        "        ",
        "        ",
        "        ",
        "        ",
        "k   r   "
    );
    move = legalMove(pinned_twin, "c2", "d4");
    after = pinned_twin;
    after.apply(move);
    ChessNotation::format(pinned_twin, move, after, notation);
    CHECK(std::strcmp(notation.algebraic, "Nd4") == 0);

    ChessBoard check = position(
        "K       ",
        "        ",
        "        ",
        "        ",
        "        ",
        "        ",
        "    R   ",
        "       k"
    );
    move = legalMove(check, "e7", "e8");
    after = check;
    after.apply(move);
    ChessNotation::format(check, move, after, notation);
    CHECK(std::strcmp(notation.algebraic, "Re8+") == 0);

    ChessBoard checking_castle = position(
        "    K  R",
        "        ",
        "        ",
        "        ",
        "        ",
        "        ",
        "        ",
        "     k  ",
        Color::White,
        true
    );
    move = legalMove(checking_castle, "e1", "g1");
    after = checking_castle;
    after.apply(move);
    ChessNotation::format(checking_castle, move, after, notation);
    CHECK(std::strcmp(notation.algebraic, "O-O+") == 0);

    ChessBoard before_stalemate = position(
        "        ",
        "        ",
        "        ",
        "        ",
        "        ",
        "  K     ",
        "  Q     ",
        "k       ",
        Color::White
    );
    move = legalMove(before_stalemate, "c7", "b6");
    after = before_stalemate;
    after.apply(move);
    ChessNotation::format(before_stalemate, move, after, notation);
    CHECK(ChessRules::status(after) == PositionStatus::Stalemate);
    CHECK(std::strcmp(notation.algebraic, "Qb6") == 0);

    ChessBoard longest = position(
        " K      ",
        "    k   ",
        "        ",
        "Q   r  Q",
        "        ",
        "        ",
        "        ",
        "Q       "
    );
    move = legalMove(longest, "a4", "e4");
    after = longest;
    after.apply(move);
    struct GuardedNotation {
        MoveNotation notation;
        char guard[4];
    } guarded = { {}, { 'S', 'A', 'F', 'E' } };
    ChessNotation::format(longest, move, after, guarded.notation);
    CHECK(std::strcmp(guarded.notation.algebraic, "Qa4xe4+") == 0);
    CHECK(std::strlen(guarded.notation.algebraic) + 1
        <= sizeof(guarded.notation.algebraic));
    CHECK(std::memcmp(guarded.guard, "SAFE", sizeof(guarded.guard)) == 0);
}

void testCheckmateLatchAndReset()
{
    ChessGame game;
    CHECK(enter(game, "6263").type == GameEventType::MoveAccepted);
    CHECK(enter(game, "5755").type == GameEventType::MoveAccepted);
    CHECK(enter(game, "7274").type == GameEventType::MoveAccepted);
    const GameEvent mate = enter(game, "4884");
    CHECK(mate.type == GameEventType::MoveAccepted);
    CHECK(mate.status == PositionStatus::Checkmate);
    CHECK(std::strcmp(mate.notation.algebraic, "Qh4#") == 0);

    char output[96];
    CHECK(ChessGame::formatOutput(mate, output, sizeof(output)));
    CHECK(std::strstr(output, "Checkmate: Black wins") != nullptr);
    const GameEvent rejected = enter(game, "5254");
    CHECK(rejected.error == MoveError::GameOver);
    CHECK(std::strcmp(
        rejected.error_description,
        "game is over, press reset"
    ) == 0);

    game.handleKey('*');
    const GameEvent fresh = enter(game, "5254");
    CHECK(fresh.type == GameEventType::MoveAccepted);
    CHECK(fresh.move_number == 1);
}

void testStalemateLatchAndReset()
{
    ChessBoard setup = position(
        "        ",
        "        ",
        "        ",
        "        ",
        "        ",
        "  K     ",
        "  Q     ",
        "k       ",
        Color::White
    );
    ChessGame game(setup);
    const GameEvent stalemate = enter(game, "3726");
    CHECK(stalemate.type == GameEventType::MoveAccepted);
    CHECK(stalemate.status == PositionStatus::Stalemate);
    CHECK(std::strcmp(stalemate.notation.algebraic, "Qb6") == 0);

    char output[96];
    CHECK(ChessGame::formatOutput(stalemate, output, sizeof(output)));
    CHECK(std::strcmp(
        output,
        "c7b6 = 1. Qb6\nStalemate: draw\n"
    ) == 0);
    const GameEvent rejected = enter(game, "1817");
    CHECK(rejected.type == GameEventType::MoveRejected);
    CHECK(rejected.error == MoveError::GameOver);
    CHECK(game.board().pieceAt(square('a', '8')) == 'k');

    game.handleKey('*');
    const GameEvent fresh = enter(game, "5254");
    CHECK(fresh.type == GameEventType::MoveAccepted);
    CHECK(fresh.move_number == 1);
}

void testRejectionMessages()
{
    ChessBoard board;
    struct Case {
        MoveError error;
        const char* expected;
    };
    const Case cases[] = {
        { MoveError::EmptyFromSquare, "empty from-square" },
        { MoveError::NotYourPiece, "it is White's turn" },
        { MoveError::SameSquare, "from and to are the same square" },
        { MoveError::FriendlyPiece, "own piece on a2" },
        { MoveError::Unreachable, "knight cannot reach g3" },
        { MoveError::PathBlocked, "path to a5 is blocked" },
        { MoveError::KingLeftInCheck, "king would be left in check" },
        { MoveError::CastlingRightsLost, "castling rights lost" },
        { MoveError::CastlingPathBlocked, "castling path is blocked" },
        {
            MoveError::CastlingThroughCheck,
            "cannot castle out of, through, or into check",
        },
        {
            MoveError::EnPassantUnavailable,
            "en passant no longer available",
        },
        { MoveError::GameOver, "game is over, press reset" },
    };
    char text[64];
    for (const Case& item : cases) {
        ChessRules::describe(
            item.error,
            board,
            square('g', '1'),
            item.error == MoveError::FriendlyPiece
                ? square('a', '2')
                : item.error == MoveError::PathBlocked
                    ? square('a', '5')
                    : square('g', '3'),
            text,
            sizeof(text)
        );
        CHECK(std::strcmp(text, item.expected) == 0);
    }
}

void testBoardPage()
{
    ChessGame game;
    char cells[64];
    copyBoard(game.board(), cells);
    char output[4096];
    const size_t output_size = renderBoardPage(
        cells,
        output,
        sizeof(output)
    );
    CHECK(output_size > 0);
    const std::string page(output, output_size);
    CHECK(page.find("<!DOCTYPE html>") == 0);
    CHECK(countText(page, "<td class=") == 64);
    CHECK(page.find("id=e1><b class=w>K</b>") != std::string::npos);

    std::fill(std::begin(cells), std::end(cells), ' ');
    CHECK(renderBoardPage(cells, output, sizeof(output)) > 0);
    CHECK(std::string(output).find("<b class=") == std::string::npos);
    char small[64];
    CHECK(renderBoardPage(cells, small, sizeof(small)) == 0);
}

uint64_t perft(const ChessBoard& board, unsigned int depth)
{
    if (depth == 0) {
        return 1;
    }

    uint64_t nodes = 0;
    for (uint8_t rank = 0; rank < 8; ++rank) {
        for (uint8_t file = 0; file < 8; ++file) {
            const Square from = { file, rank };
            if (board.isEmpty(from)
                || board.colorAt(from) != board.sideToMove()) {
                continue;
            }

            MoveList moves;
            Piece::forSquare(board.pieceAt(from))->appendMoves(
                board,
                from,
                moves
            );
            for (size_t index = 0; index < moves.size(); ++index) {
                if (ChessRules::leavesKingInCheck(board, moves[index])) {
                    continue;
                }
                ChessBoard after = board;
                after.apply(moves[index]);
                nodes += perft(after, depth - 1);
            }
        }
    }
    return nodes;
}

void testStartingPositionPerft()
{
    const ChessBoard board;
    CHECK(perft(board, 1) == 20);
    CHECK(perft(board, 2) == 400);
    CHECK(perft(board, 3) == 8902);
    CHECK(perft(board, 4) == 197281);
}

} // namespace

int main()
{
    const struct {
        const char* name;
        void (*run)();
    } tests[] = {
        { "keypad and initial position", testKeypadAndInitialPosition },
        { "knight movement", testKnightMovement },
        { "slider movement", testSliderMovement },
        { "pawn and king movement", testPawnAndKingMovement },
        { "entry, turns, and rejections", testEntryTurnOrderAndRejections },
        { "king safety and pins", testKingSafetyAndPins },
        { "castling", testCastling },
        { "en passant and state", testEnPassantAndStatePreservation },
        { "promotion flow", testPromotionFlow },
        { "basic notation", testBasicNotation },
        { "notation and status", testNotationDisambiguationAndStatus },
        { "checkmate latch", testCheckmateLatchAndReset },
        { "stalemate latch", testStalemateLatchAndReset },
        { "rejection messages", testRejectionMessages },
        { "board page", testBoardPage },
        { "starting position perft", testStartingPositionPerft },
    };

    for (const auto& test : tests) {
        try {
            test.run();
            std::cout << "PASS: " << test.name << '\n';
        }
        catch (const std::exception& error) {
            std::cerr << "FAIL: " << test.name << ": " << error.what() << '\n';
            return 1;
        }
    }
    return 0;
}
