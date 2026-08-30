## Context

The firmware today is three FreeRTOS tasks on an ESP32: `standard_output` (a queue-backed serial printer), `numpad` (a 4×4 matrix scanner on GPIO 13/12/14/27 × 26/25/33/32), and `led` (GPIO 2). The calculator behaviour lives inside `Led::run()`, which pulls keys straight off the numpad queue — so the LED task is simultaneously the input consumer, the application state machine, and the output device. `NumPad` also holds a `StandardOutput&` purely to emit a `Pressed: <n>` debug line.

Two properties of the current keypad scanner constrain this change:

1. `readKey()` returns `col + 3*row + 1` for the 3×3 digit block, `-1` for row 0 / column 3 (the `A` key), and `0` for everything else. `0` is also the "nothing pressed" sentinel, so the `*`, `0`, `#`, `B`, `C`, and `D` keys are literally invisible to the rest of the system. The reset key the product needs is row 3 / column 0 — one of the invisible ones.
2. The key queue is length 1 and written with `xQueueOverwrite`. A slow consumer silently loses keypresses. That was tolerable for a calculator with two operands; it is not tolerable when a move is four presses that must all land in order.

Beyond this change, the device is meant to pair two units over Bluetooth and drive an OLED instead of stdout, so the chess rules should not be entangled with either the keypad or the serial printer.

## Goals / Non-Goals

**Goals:**

- Delete the calculator and leave `Led` and `NumPad` as dumb peripherals with no application state.
- Put board state and move entry in one owner (a game component with its own task) that talks to peripherals through queues.
- Make every physical key distinguishable from "no key", so the reset key works and future keys are free to use.
- Keep the chess core (board, initial position, coordinate→SAN conversion) free of FreeRTOS and ESP-IDF calls so it can be reasoned about and, later, compiled and tested on a host.
- Never drop a keypress from a four-digit sequence.

**Non-Goals:**

- Move legality. No check that the piece can reach the square, no pins, no check/checkmate detection, no `+`/`#` suffixes.
- SAN disambiguation (`Nbd2`, `R1e2`). Deciding between two knights requires knowing which one can legally reach the square, which is legality logic.
- Black's moves. Only White moves; the board's black pieces stay on their starting squares unless captured.
- Bluetooth transport and the OLED screen. Output stays on `StandardOutput`.
- Undo, move history storage, PGN export, clocks.

## Decisions

### Keypad returns a character per physical key

`readKey()` returns a `char` from a 4×4 lookup table laid out like the physical keypad, with `'\0'` as the "nothing pressed" sentinel:

```
'1' '2' '3' 'A'
'4' '5' '6' 'B'
'7' '8' '9' 'C'
'*' '0' '#' 'D'
```

Row 3 / column 0 is `'*'`, which becomes the reset key. Every key now has a distinct code and no key collides with the sentinel.

*Alternative considered:* keep integers and add a single `KEY_RESET = -2` constant. Rejected because it leaves the other five keys mapping to `0` — the same latent bug, just deferred, and each future key needs another magic number. A character table is the same amount of code and reads like the hardware.

### Key queue becomes a FIFO

`xQueueOverwrite` on a length-1 queue is replaced by `xQueueSend` on a queue of 8 characters. A four-press move survives the game task being briefly descheduled; 8 gives headroom for fast typing without meaningful RAM cost.

*Trade-off:* a genuinely stuck consumer now accumulates stale keys rather than showing only the newest. With a 20 ms scan interval and a game task doing nothing but string formatting, the queue should never approach full, and dropping the middle of a move is the worse failure.

### Board as a FEN-style character grid

The board is `char board_[8][8]` indexed `[rank][file]`, both 0-based, where rank 0 is rank 1 and file 0 is file `a`. Pieces use FEN letters: `PNBRQK` for White, `pnbrqk` for Black, `' '` for empty. The starting position is a literal table that looks like a board:

```cpp
static constexpr char INITIAL_POSITION[8][9] = {
    "RNBQKBNR",  // rank 1
    "PPPPPPPP",  // rank 2
    "        ",  // rank 3
    "        ",  // rank 4
    "        ",  // rank 5
    "        ",  // rank 6
    "pppppppp",  // rank 7
    "rnbqkbnr",  // rank 8
};
```

Colour is `isupper()`, the SAN piece letter is the character itself, and reset is one `memcpy`. 

*Alternative considered:* an `enum class Piece` plus a `Color` field. More type-safe, but it needs a lookup table to get back to a SAN letter and turns the initial position into 32 assignments or a hard-to-scan initialiser list. On a board this small the char grid's readability wins.

### Digit-to-coordinate mapping

Digit `d` maps to index `d - 1` for both files and ranks, so `1`→`a`/rank 1 and `8`→`h`/rank 8. The four digits are, in order: from-file, from-rank, to-file, to-rank. `5254` is file 5 rank 2 → file 5 rank 4, i.e. `e2e4`. Digits `0` and `9` are off-board and are among the ignored keys.

### Input is a four-slot buffer, not a state machine

The game task keeps a `char digits[4]` buffer and a count. Each key is dispatched: `'*'` resets everything; `'1'`–`'8'` appends and, on reaching four, completes the move; anything else is discarded without touching the buffer. This keeps "ignore invalid keys" a one-line rule rather than a transition table.

### Empty from-square is the only rejection

Since legality is out of scope, the sole way a move can fail is that no piece stands on the from-square. In that case the game prints a message, leaves the board and the move counter untouched, and does not blink the LED. Everything else is accepted, including moving a black piece or a piece onto its own colour.

*Rationale:* without this check, SAN generation has no piece letter to work with. It is a data requirement, not a rules check.

### SAN generation rules

Given the from-piece and the board, notation is assembled as:

- **Castling** — a king moving two files from its home square: `O-O` when the destination file is `g`, `O-O-O` when it is `c`. The rook is moved as part of applying the move. Detected purely geometrically; the change does not verify castling rights or that the rook is actually there.
- **Pawn moves** — no piece letter. A quiet move is just the destination (`e4`). A capture is the from-file letter, `x`, then the destination (`exd5`).
- **Pawn promotion** — a White pawn reaching rank 8 becomes a queen and the notation gets `=Q` (`e8=Q`). Four digits leave no room to choose the promotion piece, and a queen is the choice in the overwhelming majority of games.
- **Piece moves** — the uppercase piece letter, then `x` if the destination is occupied, then the destination (`Nf3`, `Bxf7`).
- **Diagonal pawn move to an empty square** — still written as a capture (`exd5`). This is what en passant looks like, and the alternative (`d5` for a sideways pawn move) is notation that cannot be replayed.

No disambiguation and no check/checkmate suffix, per the non-goals.

### Move numbering and output format

A counter starts at 1, is printed with each accepted move, and increments afterwards; reset returns it to 1. The output line is:

```
e2e4 = 1. e4
```

Only White moves, so every move gets its own number and there is no `1...` black-move form.

### Module layout

| File | Contents | Depends on FreeRTOS? |
| --- | --- | --- |
| `chess_board.hpp/.cpp` | Board grid, initial position, `applyMove`, coordinate→SAN | No |
| `game.hpp/.cpp` | Task loop, digit buffer, move counter, peripheral wiring | Yes |
| `numpad.*` | Matrix scan, key queue | Yes |
| `led.*` | Blink command queue | Yes |
| `standard_output.*` | Unchanged | Yes |

`Game` holds references to `NumPad`, `Led`, and `StandardOutput`. `Led` gains a command queue and a `blinkOnce()` that posts to it, replacing its direct read of the numpad queue; `NumPad` drops its `StandardOutput&`. The dependency direction becomes peripherals ← game, with no cycles.

### Task priorities and stacks

The game task is registered in `app_main` alongside the existing three, at priority 5 with a 2048-word stack, matching the current tasks. It only formats short strings, so this is ample.

## Risks / Trade-offs

- **Ambiguous notation when two identical pieces can reach a square** → `Nf3` may be genuinely ambiguous with knights on `d2` and `g1`. Accepted: the coordinate form (`g1f3`) is printed alongside and is never ambiguous. Disambiguation lands with the legality work.
- **Geometric castling detection misfires** → a king slid two files sideways in the middlegame is written as castling. Accepted: it is an illegal move, and illegal moves are explicitly not this change's problem. Revisit with castling rights.
- **Auto-queening hides a real choice** → underpromotion is unreachable. Accepted for now; a promotion prompt needs an input design that four digits do not have. Listed as an open question.
- **A wrong digit can only be cleared by resetting the whole game** → there is no backspace, so a mistyped third digit forces either completing a junk move or pressing `*` and losing the position. Worth a dedicated undo/backspace key later; noted rather than solved here to keep the keymap minimal.
- **Membrane keypad bounce or ghosting injects digits** → the existing edge detection (act only on a `none → key` transition) plus the 20 ms scan interval is retained unchanged, which was adequate for the calculator.
- **Queue depth of 8 could still overflow** → only if the game task stalls for >160 ms. Nothing in its loop blocks other than the queue receive itself.

## Open Questions

- How should underpromotion be entered once the OLED and a richer keymap exist — a fifth keypress, or a modifier key?
- When Bluetooth lands, does the receiving device replay the coordinate move against its own board, or trust the sender's notation? The board model here assumes the former.
- Should the reset key require a confirmation press so a mid-game bump cannot wipe the position?
