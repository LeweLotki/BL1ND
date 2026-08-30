idf_component_register(
    SRCS
        "main.cpp"
        "chess_board.cpp"
        "chess_game.cpp"
        "game.cpp"
        "numpad.cpp"
        "led.cpp"
        "standard_output.cpp"
    INCLUDE_DIRS "."
    REQUIRES esp_driver_gpio
)
