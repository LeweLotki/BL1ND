# bl1nd

`bl1nd` is ESP32 firmware for a blindfold-chess move-entry device. A player
enters moves on a 4×4 matrix keypad and receives an LED confirmation. The
firmware prints generated notation to the serial output and hosts a local Wi-Fi
board preview for an arbiter or spectator.

## Features

- Four-key coordinate move entry, plus a promotion choice when needed
- In-memory chessboard initialized to the standard starting position
- Legal-move validation with turn order, king safety, castling, and en passant
- Standard algebraic notation including disambiguation, check, and checkmate
- Numbered move output over the serial console
- Distinct LED confirmation and rejection patterns
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

After the fourth valid digit, a legal move is applied and printed:

```text
e2e4 = 1. e4
g1f3 = 2. Nf3
```

If a pawn legally reaches its last rank, the board waits for a fifth key:
`A` chooses a queen, `B` a rook, `C` a bishop, and `D` a knight. For example,
after entering the four digits for `b7a8`, press `D` to complete `bxa8=N`.
Other keys are ignored while the choice is pending; `*` still resets the game.

The LED blinks once slowly for an accepted move and three times rapidly for a
rejected move. A rejection does not change the board, consume a move number,
or discard an available en passant capture. The serial line names the reason:

```text
e4e5 = illegal: empty from-square
a1a5 = illegal: path to a5 is blocked
```

Checkmate and stalemate add a result line:

```text
d8h4 = 4. Qh4#
Checkmate: Black wins
Stalemate: draw
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

The firmware enforces normal piece movement, blocking, captures, turn order,
king safety, castling rights and attacked paths, en passant, and promotion for
both colours. It detects checkmate and stalemate, then refuses further moves
until reset. SAN output includes legal-move disambiguation and `+`/`#`.

It does not detect draws by threefold repetition, the fifty-move rule, or
insufficient material. It also has no position-entry mode; positions must be
reached by replaying legal moves from the start. The device is a practice aid,
not an arbiter of last resort.

The accepted/rejected LED signal reveals whether a proposed move is legal,
which is information a blindfold player is normally expected to retain.
Rejected attempts and their reasons are logged to the serial console so an
arbiter can review possible probing.

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
├── game_logic/           Board state, rules, SAN, and move entry
│   └── pieces/           One allocation-free movement class per piece kind
├── peripherals/          Keypad, LED, and queued serial output
└── server/               Wi-Fi AP, board snapshot, HTTP server, and HTML page
tests/                    Host-side unit tests
openspec/                 Behavior specifications and change documentation
```

The chess board and move-entry core are independent of ESP-IDF. FreeRTOS queues
connect the game task to the keypad, LED, serial output, and immutable board
snapshot used by the HTTP server.
