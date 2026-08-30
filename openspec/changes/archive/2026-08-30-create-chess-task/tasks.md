## 1. Strip the calculator

- [x] 1.1 Remove the accumulate-and-add loop from `Led::run()`, along with the `key_a`/`key_b`/`is_addition_pressed` state and the `NumPad&` member and constructor parameter in `led.hpp`/`led.cpp`
- [x] 1.2 Remove the `Pressed: %d` line and the `StandardOutput&` member and constructor parameter from `numpad.hpp`/`numpad.cpp`
- [x] 1.3 Confirm the project still builds with `Led` and `NumPad` reduced to peripherals

## 2. Keypad reports every key

- [x] 2.1 Replace the arithmetic key mapping in `NumPad::readKey()` with a `static constexpr char KEYS[4][4]` table laid out as `1 2 3 A` / `4 5 6 B` / `7 8 9 C` / `* 0 # D`, returning `'\0'` when nothing is pressed
- [x] 2.2 Change the key queue and the `receiveKey` / `waitForNewKey` signatures from `int` to `char`, and update the edge detection in `NumPad::run()` to compare against `'\0'`
- [x] 2.3 Change the key queue to hold 8 entries and publish with `xQueueSend` instead of `xQueueOverwrite`
- [x] 2.4 Flash and confirm each of the 16 keys is distinguishable, especially `*` at row 3 / column 0

## 3. LED becomes command-driven

- [x] 3.1 Give `Led` a command queue and a `blinkOnce()` method that posts to it
- [x] 3.2 Rewrite `Led::run()` to init the GPIO then block on the command queue, blinking once per command received
- [x] 3.3 Verify no game state remains in `led.cpp`

## 4. Board model

- [x] 4.1 Create `main/chess_board.hpp` and `main/chess_board.cpp` with a `ChessBoard` class holding `char board_[8][8]` indexed `[rank][file]`, using FEN letters (`PNBRQK` White, `pnbrqk` Black, `' '` empty)
- [x] 4.2 Add the `INITIAL_POSITION` table and a `reset()` that restores it
- [x] 4.3 Add a square accessor and a helper that maps a digit 1–8 to index 0–7, plus square-to-text (`e2`) formatting
- [x] 4.4 Add `applyMove(from, to)` that moves the piece, clears the from-square, and overwrites any piece on the to-square

## 5. Notation conversion

- [x] 5.1 Add a conversion entry point that takes a from-square and to-square and produces the coordinate string (`e2e4`) plus the algebraic string, reporting failure when the from-square is empty
- [x] 5.2 Handle castling: a king moving two files from its home square renders `O-O` (destination file `g`) or `O-O-O` (destination file `c`), and moves the rook h1→f1 or a1→d1 as part of applying the move
- [x] 5.3 Handle quiet pawn moves as the destination square alone
- [x] 5.4 Handle captures: `x` before the destination, after the piece letter for non-pawns, and the from-file letter for pawns; treat a diagonal pawn move to an empty square as a capture
- [x] 5.5 Handle non-pawn moves as the uppercase piece letter followed by the destination
- [x] 5.6 Handle promotion: a White pawn reaching rank 8 becomes a queen on the board and the notation gains a `=Q` suffix
- [x] 5.7 Walk the scenarios in `specs/chess-notation/spec.md` by hand against the implementation, confirming `e4`, `Nf3`, `Bxf7`, `exd5`, `O-O`, `O-O-O`, and `e8=Q`

## 6. Game task

- [x] 6.1 Create `main/game.hpp` and `main/game.cpp` with a `Game` class holding references to `NumPad`, `Led`, and `StandardOutput`, owning a `ChessBoard`, a four-slot digit buffer with a count, and a move counter starting at 1
- [x] 6.2 Implement the run loop: block on a key, dispatch `'*'` to reset, `'1'`–`'8'` to append, and discard everything else without disturbing the buffer
- [x] 6.3 On the fourth digit, convert the buffer to from/to squares, run the conversion, and clear the buffer
- [x] 6.4 On an accepted move, print `%s = %d. %s` through `StandardOutput`, apply the move to the board, increment the move counter, and call `Led::blinkOnce()`
- [x] 6.5 On a rejected move (empty from-square), print a rejection message and leave the board, the counter, and the LED untouched
- [x] 6.6 On reset, restore the starting position, clear the digit buffer, and set the move counter back to 1

## 7. Wiring

- [x] 7.1 Add `chess_board.cpp` and `game.cpp` to `SRCS` in `main/CMakeLists.txt`
- [x] 7.2 Update the object construction in `main.cpp` for the new constructor signatures and add the `Game` instance
- [x] 7.3 Register a `game_task` in `app_main` with a 2048-word stack at priority 5, alongside the existing three tasks

## 8. Verification on device

- [x] 8.1 Build and flash, then enter 5254 and confirm stdout shows `e2e4 = 1. e4` and the LED blinks once
- [x] 8.2 Enter 7163 and confirm `g1f3 = 2. Nf3`
- [x] 8.3 Confirm no output appears for the first three digits of a move
- [x] 8.4 Press 9, 0, and a letter key mid-entry and confirm they are ignored and the move still completes correctly
- [x] 8.5 Enter 5454 from the starting position and confirm the rejection message, no LED blink, and an unchanged move number
- [x] 8.6 Press `*` and confirm the next move is numbered 1 and resolves against the starting position
- [x] 8.7 Play a sequence that reaches a capture and confirm the `x` form, then a sequence that castles and confirm `O-O`
