cmake_minimum_required(VERSION 3.16)

project(bl1nd_host_tests LANGUAGES CXX)

enable_testing()

add_executable(
    chess_tests
    test_chess.cpp
    ../main/board_page.cpp
    ../main/chess_board.cpp
    ../main/chess_game.cpp
)

target_include_directories(chess_tests PRIVATE ../main)
target_compile_features(chess_tests PRIVATE cxx_std_17)
target_compile_options(chess_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)

add_test(NAME chess_tests COMMAND chess_tests)
