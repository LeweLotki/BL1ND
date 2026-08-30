cmake_minimum_required(VERSION 3.16)

project(bl1nd_host_tests LANGUAGES CXX)

enable_testing()

add_executable(
    chess_tests
    test_chess.cpp
    ../src/server/board_page.cpp
    ../src/game_logic/chess_board.cpp
    ../src/game_logic/chess_game.cpp
    ../src/game_logic/chess_notation.cpp
    ../src/game_logic/chess_rules.cpp
    ../src/game_logic/pieces/piece.cpp
    ../src/game_logic/pieces/pawn.cpp
    ../src/game_logic/pieces/knight.cpp
    ../src/game_logic/pieces/bishop.cpp
    ../src/game_logic/pieces/rook.cpp
    ../src/game_logic/pieces/queen.cpp
    ../src/game_logic/pieces/king.cpp
    ../src/link/link_protocol.cpp
    ../src/link/link_session.cpp
)

target_include_directories(
    chess_tests
    PRIVATE
        ../src/game_logic
        ../src/game_logic/pieces
        ../src/link
        ../src/peripherals
        ../src/server
)
target_compile_features(chess_tests PRIVATE cxx_std_17)
target_compile_options(chess_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)

add_test(NAME chess_tests COMMAND chess_tests)
