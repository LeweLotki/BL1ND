#include "board_page.hpp"
#include "chess_game.hpp"
#include "keypad_layout.hpp"

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

GameEvent enter(ChessGame& game, const char* keys)
{
    GameEvent event = { GameEventType::None, {}, 0 };
    for (const char* key = keys; *key != '\0'; ++key) {
        event = game.handleKey(*key);
    }
    return event;
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
    size_t position = 0;
    while ((position = text.find(needle, position)) != std::string::npos) {
        ++count;
        position += needle.size();
    }
    return count;
}

void expectEvent(
    const GameEvent& event,
    GameEventType type,
    const char* coordinate,
    const char* algebraic,
    unsigned int move_number
)
{
    CHECK(event.type == type);
    CHECK(std::strcmp(event.notation.coordinate, coordinate) == 0);
    CHECK(std::strcmp(event.notation.algebraic, algebraic) == 0);
    CHECK(event.move_number == move_number);
}

void testKeypadLayout()
{
    constexpr char expected[4][5] = {
        "123A",
        "456B",
        "789C",
        "*0#D",
    };
    bool seen[128] = {};

    for (unsigned int row = 0; row < 4; ++row) {
        for (unsigned int column = 0; column < 4; ++column) {
            const char key = KeypadLayout::keyAt(row, column);
            CHECK(key == expected[row][column]);
            CHECK(key != KeypadLayout::NO_KEY);
            CHECK(!seen[static_cast<unsigned char>(key)]);
            seen[static_cast<unsigned char>(key)] = true;
        }
    }

    CHECK(KeypadLayout::keyAt(4, 0) == KeypadLayout::NO_KEY);
    CHECK(KeypadLayout::keyAt(0, 4) == KeypadLayout::NO_KEY);
}

void testInitialPositionAndCoordinates()
{
    ChessGame game;
    const ChessBoard& board = game.board();

    CHECK(board.pieceAt(square('a', '1')) == 'R');
    CHECK(board.pieceAt(square('b', '1')) == 'N');
    CHECK(board.pieceAt(square('d', '1')) == 'Q');
    CHECK(board.pieceAt(square('e', '1')) == 'K');
    CHECK(board.pieceAt(square('e', '2')) == 'P');
    CHECK(board.pieceAt(square('e', '4')) == ' ');
    CHECK(board.pieceAt(square('e', '7')) == 'p');
    CHECK(board.pieceAt(square('e', '8')) == 'k');
    CHECK(board.pieceAt(square('g', '8')) == 'n');

    CHECK(ChessBoard::digitToIndex('1') == 0);
    CHECK(ChessBoard::digitToIndex('8') == 7);

    char text[3];
    ChessBoard::formatSquare(square('e', '2'), text);
    CHECK(std::strcmp(text, "e2") == 0);
}

void testFourDigitEntryAndOutput()
{
    ChessGame game;
    char output[64];

    CHECK(game.handleKey('5').type == GameEventType::None);
    CHECK(game.handleKey('9').type == GameEventType::None);
    CHECK(game.handleKey('2').type == GameEventType::None);
    CHECK(game.handleKey('0').type == GameEventType::None);
    CHECK(game.handleKey('A').type == GameEventType::None);
    CHECK(game.handleKey('5').type == GameEventType::None);

    const GameEvent pawn = game.handleKey('4');
    expectEvent(pawn, GameEventType::MoveAccepted, "e2e4", "e4", 1);
    CHECK(ChessGame::formatOutput(pawn, output, sizeof(output)));
    CHECK(std::strcmp(output, "e2e4 = 1. e4\n") == 0);
    CHECK(game.board().pieceAt(square('e', '2')) == ' ');
    CHECK(game.board().pieceAt(square('e', '4')) == 'P');

    const GameEvent knight = enter(game, "7163");
    expectEvent(knight, GameEventType::MoveAccepted, "g1f3", "Nf3", 2);
    CHECK(ChessGame::formatOutput(knight, output, sizeof(output)));
    CHECK(std::strcmp(output, "g1f3 = 2. Nf3\n") == 0);

    const GameEvent none = game.handleKey('#');
    CHECK(!ChessGame::formatOutput(none, output, sizeof(output)));
    CHECK(output[0] == '\0');
}

void testRejectionDoesNotConsumeMoveNumber()
{
    ChessGame game;
    char output[64];

    const GameEvent rejected = enter(game, "5455");
    CHECK(rejected.type == GameEventType::MoveRejected);
    CHECK(std::strcmp(rejected.notation.coordinate, "e4e5") == 0);
    CHECK(game.board().pieceAt(square('e', '4')) == ' ');
    CHECK(ChessGame::formatOutput(rejected, output, sizeof(output)));
    CHECK(
        std::strcmp(output, "e4e5 = invalid: empty from-square\n") == 0
    );

    const GameEvent accepted = enter(game, "5254");
    expectEvent(accepted, GameEventType::MoveAccepted, "e2e4", "e4", 1);
}

void testResetRestoresEverything()
{
    ChessGame game;
    enter(game, "5254");
    CHECK(game.handleKey('7').type == GameEventType::None);
    CHECK(game.handleKey('1').type == GameEventType::None);

    const GameEvent reset = game.handleKey('*');
    CHECK(reset.type == GameEventType::Reset);
    CHECK(game.board().pieceAt(square('e', '2')) == 'P');
    CHECK(game.board().pieceAt(square('e', '4')) == ' ');

    const GameEvent first_after_reset = enter(game, "5254");
    expectEvent(
        first_after_reset,
        GameEventType::MoveAccepted,
        "e2e4",
        "e4",
        1
    );
}

void testPawnAndPieceNotation()
{
    ChessGame game;

    expectEvent(
        enter(game, "4243"),
        GameEventType::MoveAccepted,
        "d2d3",
        "d3",
        1
    );
    expectEvent(
        enter(game, "7163"),
        GameEventType::MoveAccepted,
        "g1f3",
        "Nf3",
        2
    );
    expectEvent(
        enter(game, "5182"),
        GameEventType::MoveAccepted,
        "e1h2",
        "Kxh2",
        3
    );

    game.handleKey('*');
    enter(game, "3134");
    expectEvent(
        enter(game, "3467"),
        GameEventType::MoveAccepted,
        "c4f7",
        "Bxf7",
        2
    );

    game.handleKey('*');
    enter(game, "5254");
    expectEvent(
        enter(game, "5445"),
        GameEventType::MoveAccepted,
        "e4d5",
        "exd5",
        2
    );
}

void testCastlingMovesTheRook()
{
    ChessGame game;

    expectEvent(
        enter(game, "5171"),
        GameEventType::MoveAccepted,
        "e1g1",
        "O-O",
        1
    );
    CHECK(game.board().pieceAt(square('g', '1')) == 'K');
    CHECK(game.board().pieceAt(square('f', '1')) == 'R');
    CHECK(game.board().pieceAt(square('e', '1')) == ' ');
    CHECK(game.board().pieceAt(square('h', '1')) == ' ');

    game.handleKey('*');
    expectEvent(
        enter(game, "5131"),
        GameEventType::MoveAccepted,
        "e1c1",
        "O-O-O",
        1
    );
    CHECK(game.board().pieceAt(square('c', '1')) == 'K');
    CHECK(game.board().pieceAt(square('d', '1')) == 'R');
    CHECK(game.board().pieceAt(square('e', '1')) == ' ');
    CHECK(game.board().pieceAt(square('a', '1')) == ' ');
}

void testPromotion()
{
    ChessGame game;

    enter(game, "5856");
    enter(game, "5257");
    expectEvent(
        enter(game, "5758"),
        GameEventType::MoveAccepted,
        "e7e8",
        "e8=Q",
        3
    );
    CHECK(game.board().pieceAt(square('e', '8')) == 'Q');

    game.handleKey('*');
    enter(game, "4846");
    enter(game, "5257");
    expectEvent(
        enter(game, "5748"),
        GameEventType::MoveAccepted,
        "e7d8",
        "exd8=Q",
        3
    );
    CHECK(game.board().pieceAt(square('d', '8')) == 'Q');
}

void testLegalityIsNotChecked()
{
    ChessGame game;

    expectEvent(
        enter(game, "1115"),
        GameEventType::MoveAccepted,
        "a1a5",
        "Ra5",
        1
    );
    CHECK(game.board().pieceAt(square('a', '1')) == ' ');
    CHECK(game.board().pieceAt(square('a', '5')) == 'R');

    game.handleKey('*');
    expectEvent(
        enter(game, "4185"),
        GameEventType::MoveAccepted,
        "d1h5",
        "Qh5",
        1
    );
}

void testBoardPageStartingPosition()
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
    CHECK(output_size < 3800);
    CHECK(output[output_size] == '\0');

    const std::string page(output, output_size);
    CHECK(page.find("<!DOCTYPE html>") == 0);
    CHECK(page.find("http-equiv=\"refresh\" content=\"2\"") != std::string::npos);
    CHECK(page.find(".w{color:#fff") != std::string::npos);
    CHECK(page.find(".b{color:#000") != std::string::npos);
    CHECK(countText(page, "<td class=") == 64);

    CHECK(
        page.find(
            "id=e1><b class=w>K</b>"
        ) != std::string::npos
    );
    CHECK(
        page.find(
            "id=d1><b class=w>Q</b>"
        ) != std::string::npos
    );
    CHECK(
        page.find(
            "id=g8><b class=b>N</b>"
        ) != std::string::npos
    );
    CHECK(
        page.find(
            "id=e7><b class=b>P</b>"
        ) != std::string::npos
    );
}

void testBoardPageEmptyAndMovedPosition()
{
    char cells[64];
    std::fill(std::begin(cells), std::end(cells), ' ');

    char output[4096];
    size_t output_size = renderBoardPage(cells, output, sizeof(output));
    CHECK(output_size > 0);

    std::string page(output, output_size);
    CHECK(countText(page, "<td class=") == 64);
    CHECK(countText(page, "<b class=") == 0);

    cells[3 * 8 + 4] = 'P';
    output_size = renderBoardPage(cells, output, sizeof(output));
    CHECK(output_size > 0);
    page.assign(output, output_size);
    CHECK(countText(page, "<b class=") == 1);
    CHECK(
        page.find(
            "id=e4><b class=w>P</b>"
        ) != std::string::npos
    );
}

void testBoardPageRejectsSmallBuffer()
{
    char cells[64];
    std::fill(std::begin(cells), std::end(cells), ' ');

    char output[64];
    CHECK(renderBoardPage(cells, output, sizeof(output)) == 0);
    CHECK(output[0] == '\0');
}

} // namespace

int main()
{
    const struct {
        const char* name;
        void (*run)();
    } tests[] = {
        { "keypad layout", testKeypadLayout },
        { "initial position and coordinates", testInitialPositionAndCoordinates },
        { "four-digit entry and output", testFourDigitEntryAndOutput },
        { "empty-square rejection", testRejectionDoesNotConsumeMoveNumber },
        { "reset", testResetRestoresEverything },
        { "pawn and piece notation", testPawnAndPieceNotation },
        { "castling", testCastlingMovesTheRook },
        { "promotion", testPromotion },
        { "legality omitted", testLegalityIsNotChecked },
        { "board page starting position", testBoardPageStartingPosition },
        { "board page empty and moved", testBoardPageEmptyAndMovedPosition },
        { "board page small buffer", testBoardPageRejectsSmallBuffer },
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
