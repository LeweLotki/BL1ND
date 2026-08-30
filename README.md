# bl1nd

`bl1nd` is ESP32 firmware for a blindfold-chess move-entry device. A player
enters moves on a 4×4 matrix keypad and receives an LED confirmation. The
firmware prints generated notation to the serial output and hosts a local Wi-Fi
board preview for an arbiter or spectator.

## Features

- Four-key coordinate move entry, such as `5254` for `e2e4`
- In-memory chessboard initialized to the standard starting position
- Algebraic-style notation for piece moves, captures, castling, and promotion
- Numbered move output over the serial console
- LED confirmation for accepted moves
- One-key game reset
- Self-refreshing, phone-sized board preview over an ESP32 Wi-Fi access point
- Host-side tests for the platform-independent chess and HTML-rendering code

## Hardware

The current firmware targets an ESP32 and uses:

- 4×4 keypad row pins: GPIO 13, 12, 14, and 27
- 4×4 keypad column pins: GPIO 26, 25, 33, and 32
- Confirmation LED: GPIO 2

The keypad is expected to use this layout:

```text
1 2 3 A
4 5 6 B
7 8 9 C
* 0 # D
```

The column inputs use the ESP32's internal pull-ups. The LED output is
active-high.

## Entering moves

A move consists of four digits in this order:

```text
from-file, from-rank, to-file, to-rank
```

Digits `1` through `8` map directly to files `a` through `h` and ranks `1`
through `8`.

Examples:

```text
5254 -> e2e4
7163 -> g1f3
```

After the fourth valid digit, the move is applied and printed:

```text
e2e4 = 1. e4
g1f3 = 2. Nf3
```

The LED blinks once for an accepted move. A move from an empty square is
rejected without changing the board or consuming a move number:

```text
e4e5 = invalid: empty from-square
```

Press `*` at any time to restore the starting position, reset move numbering
to 1, and discard a partially entered move. Keys other than `1`–`8` and `*`
are ignored without clearing the partial entry.

## Board preview

At startup, the ESP32 creates an open Wi-Fi network:

```text
SSID: esp_chessboard
Password: none
URL: http://192.168.4.1/
```

Join the network from a phone and open the URL to view the current position.
The page refreshes every two seconds, requires no internet connection, and
supports up to four connected clients. It changes only after accepted moves
or a reset.

The access point is intentionally open. Anyone in range can view the board, so
access control must be handled through tournament procedure or physical
supervision.

If Wi-Fi or the HTTP server fails to start, the error is printed to the serial
console and keypad play continues.

## Chess behavior and limitations

This project tracks piece placement and formats moves; it is not a chess rules
engine.

- Any move with a non-empty source square is accepted.
- Piece movement, turn order, check, checkmate, pins, and castling rights are
  not validated.
- Moving onto an occupied square removes the existing piece, regardless of
  color.
- Algebraic output does not include disambiguation, `+`, or `#`.
- A king moving two files from its home square is treated as castling and also
  moves the corresponding rook.
- A White pawn reaching rank 8 is automatically promoted to a queen.

## Building and flashing

The project is built with ESP-IDF 6.0.2 and CMake 3.16 or newer.

Activate an ESP-IDF 6.0.2 environment. With the managed installation used for
this project:

```sh
source ~/.espressif/tools/activate_idf_v6.0.2.sh
```

Then build the firmware:

```sh
idf.py set-target esp32
idf.py build
```

Flash the ESP32 and open its serial monitor, replacing the port when needed:

```sh
idf.py -p /dev/ttyUSB0 flash monitor
```

Exit the monitor with `Ctrl-]`.

## Running host tests

The host tests require a C++17 compiler and CMake:

```sh
cmake -S tests -B build-host
cmake --build build-host
ctest --test-dir build-host --output-on-failure
```

## Project structure

```text
src/
├── main.cpp              Application wiring and FreeRTOS task startup
├── main.hpp
├── game_logic/           Board state, move entry, and notation
├── peripherals/          Keypad, LED, and queued serial output
└── server/               Wi-Fi AP, board snapshot, HTTP server, and HTML page
tests/                    Host-side unit tests
openspec/                 Behavior specifications and change documentation
```

The chess board and move-entry core are independent of ESP-IDF. FreeRTOS queues
connect the game task to the keypad, LED, serial output, and immutable board
snapshot used by the HTTP server.
