## Why

The firmware currently runs a throwaway calculator (press digits, press `A`, LED blinks the sum) that was only ever a bring-up exercise for the keypad and LED peripherals. The actual product is a blindfold-chess device: a calculator-shaped box where the player types a move as four digits and gets back proper chess notation. Replacing the calculator with real move entry turns the existing peripheral drivers into the first vertical slice of that product and gives us the board model that Bluetooth pairing and the OLED screen will both need later.

## What Changes

- **BREAKING**: Remove the calculator logic entirely — the accumulate-two-operands-and-add behaviour in `Led::run()`, the `A` key acting as a "plus" operator, and the `Pressed: <n>` debug line emitted by the numpad scanner.
- Add a chess game component that owns board state and move entry, running as its own FreeRTOS task. `Led` and `NumPad` become thin peripherals with no game logic in them.
- Accept a move as four consecutive keypresses in the digit range 1–8: from-file, from-rank, to-file, to-rank. `5254` means e2→e4.
- Maintain an in-memory 8×8 board seeded with the standard chess starting position, and apply each completed move to it so later moves see the updated placement.
- Convert the completed coordinate move to standard algebraic notation using the piece standing on the from-square, including piece letters (`N`, `B`, `R`, `Q`, `K`), capture markers (`x`), and castling (`O-O`, `O-O-O`).
- Print one line per move to stdout in the form `e2e4 = 1. e4`, with a move counter that increments per move and resets with the board.
- Blink the LED once when a move is accepted.
- Map the keypad's row 3 / column 0 key to a distinct reset code so it can restore the starting position, clear the move counter, and discard any partially typed move. Today that key is indistinguishable from "no key pressed".
- Ignore keys outside 1–8 while a move is being typed, leaving the partial move intact.
- Do not check move legality: any four digits are accepted as long as a piece stands on the from-square.

## Capabilities

### New Capabilities

- `chess-move-entry`: Turning keypad presses into a completed four-digit coordinate move — digit accumulation, handling of out-of-range keys, the reset key, and LED confirmation feedback.
- `chess-notation`: The board model — the standard starting position, applying a move to the board, and rendering a coordinate move as algebraic notation with a move number for stdout.

### Modified Capabilities

None. `openspec/specs/` is empty; this change introduces the project's first specs.

## Impact

- `main/led.cpp` / `main/led.hpp`: calculator loop removed; `Led` exposes a blink triggered by the game rather than reading the keypad itself.
- `main/numpad.cpp` / `main/numpad.hpp`: row 3 / column 0 gains a distinct key code; the `Pressed: <n>` output and the `StandardOutput` dependency are dropped.
- `main/main.cpp`: new game task wired in alongside the existing `standard_output`, `numpad`, and `led` tasks.
- `main/CMakeLists.txt`: new source files registered.
- New files for the board model, notation conversion, and the game task.
- No new dependencies; `standard_output` remains the only output sink until the OLED screen replaces it.
