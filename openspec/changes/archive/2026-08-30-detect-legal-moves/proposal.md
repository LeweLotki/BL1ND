## Why

The device is meant to replace a physical board for a blindfold player, and a
blindfold player's single greatest risk is that their mental board silently
drifts from the real one. Today the firmware accepts any move whose from-square
is occupied: a bishop can move like a rook, a pinned knight can walk away and
expose its king, and a player can move twice in a row without noticing. The one
mistake the device could catch for free — "that move is not possible in this
position" — is exactly the one it lets through, so the LED currently confirms
nothing more than "four keys were pressed".

Making the LED mean *legal* turns the device from a notation recorder into a
referee the player can trust, which is the difference between a practice toy
and something usable in a rated blindfold game.

## What Changes

- **BREAKING**: A move is accepted only if it is legal in the current position.
  Moves that break a piece's movement rules, ignore turn order, jump over
  pieces, capture a friendly piece, or leave the mover's own king in check are
  rejected, the board is unchanged, and the move number is not consumed.
- Give each piece kind its own class in its own file under
  `src/game_logic/pieces/`, each responsible for generating its own
  pseudo-legal moves, replacing today's "any occupied from-square is fine".
- Track side to move, alternating from White, so moving the wrong colour is
  rejected.
- Filter pseudo-legal moves through king safety, which handles absolute pins,
  moving a king into check, and the requirement to answer an existing check,
  without any of those needing a special case.
- Implement castling in full: rights are lost when the king or the relevant
  rook first moves or when the rook is captured, and the king may not castle
  out of, through, or into check, nor over occupied squares.
- Implement en passant, available only on the move immediately following the
  enemy pawn's two-square advance, including the rare case where the capture
  would expose the mover's own king along a rank.
- **BREAKING**: Promotion is chosen, not assumed. When four digits complete a
  pawn move to the last rank, entry stays pending until the player presses `A`
  (queen), `B` (rook), `C` (bishop), or `D` (knight). Black pawns reaching rank
  1 promote as well, which today they do not.
- **BREAKING**: The LED speaks two words instead of one. A legal move blinks it
  once; an illegal move blinks it three times, fast, so the two are
  distinguishable without sight.
- Print a reason for every rejection on the serial console, such as
  `e2e5 = illegal: pawn cannot reach e5`, so a disputed move can be settled at
  the bench.
- Produce real standard algebraic notation: file, rank, or full-square
  disambiguation when two identical pieces can reach a square, the chosen
  promotion piece after `=`, `+` for check, and `#` for checkmate.
- Detect checkmate and stalemate, print the result, and reject all further
  moves until the reset key starts a new game.

## Capabilities

### New Capabilities

- `chess-move-legality`: What makes a move legal — per-piece movement and
  blocking rules, turn order, friendly-fire, king safety and pins, castling
  rights and the squares castling requires, and en passant availability.
- `chess-game-end`: Detecting and reporting checkmate, stalemate, and the
  resulting refusal to accept further moves until reset.

### Modified Capabilities

- `chess-move-entry`: Promotion needs a fifth keypress, so `A`–`D` are no
  longer discarded unconditionally and a completed four-digit entry no longer
  always produces a move. The LED gains a distinct three-blink pattern for
  rejected moves, where today it stays dark.
- `chess-notation`: Legality is now verified rather than explicitly skipped, so
  the "move legality is not verified" requirement is replaced. Notation gains
  disambiguation, `+`/`#` suffixes, the chosen promotion piece, and Black
  promotion; rejection messages gain a reason.

## Impact

- New `src/game_logic/pieces/`: a `Piece` interface plus `Pawn`, `Knight`,
  `Bishop`, `Rook`, `Queen`, and `King`, one class per file.
- `src/game_logic/chess_board.*`: gains side to move, castling rights, the en
  passant target square, square-attack queries, king location, legality
  checking, and en passant / chosen-promotion handling in `applyMove`. The
  `pieceAt` accessor and the 64-character board layout the preview depends on
  are unchanged.
- `src/game_logic/chess_game.*`: gains the pending-promotion entry state, a
  rejection reason on the event, and the game-over state. `GameEventType` grows
  new values, which every consumer of the event must handle.
- `src/game_logic/game.cpp`: chooses the LED pattern from the event type.
- `src/peripherals/led.*`: gains a rapid three-blink command alongside the
  existing single blink; the command queue carries a pattern instead of a
  constant.
- `src/CMakeLists.txt` and `tests/CMakeLists.txt`: the new piece sources and
  the `game_logic/pieces` include directory.
- `tests/test_chess.cpp`: the existing `testLegalityIsNotChecked` asserts
  behaviour this change deliberately removes and must be replaced.
- `README.md`: the "Chess behavior and limitations" section describes the
  opposite of the new behaviour and needs rewriting, along with the move-entry
  and LED documentation.
- No change to the Wi-Fi preview, the keypad scanner, or the serial output
  plumbing.
- **Not addressed**: threefold repetition, the fifty-move rule, and insufficient
  material. The device will not declare those draws.
