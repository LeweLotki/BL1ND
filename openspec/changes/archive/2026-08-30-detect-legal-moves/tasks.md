## 1. Shared move types and build plumbing

- [x] 1.1 Create `src/game_logic/move.hpp` holding `Square`, `enum class Color { White, Black }`, `enum class MoveKind { Normal, DoublePawnPush, EnPassant, CastleKingside, CastleQueenside }`, a `Move` struct (`from`, `to`, `kind`, `promotion` char that is `'\0'` when absent), and a `MoveList` with a fixed capacity of 64, an `append` that is a no-op when full, `size()`, and indexed access
- [x] 1.2 Move the `Square` definition out of `chess_board.hpp` into `move.hpp` and have `chess_board.hpp` include `move.hpp`, so existing includes of `chess_board.hpp` keep compiling unchanged
- [x] 1.3 Add `colorOf(char piece)` and `isEmpty(char piece)` helpers to `move.hpp` so the case convention for colour lives in one place
- [x] 1.4 Widen `MoveNotation::algebraic` from 8 to 12 bytes and confirm every `snprintf` into it still bounds itself with `sizeof`
- [x] 1.5 Add the `game_logic/pieces` include directory to `tests/CMakeLists.txt` and to `INCLUDE_DIRS` in `src/CMakeLists.txt`, ahead of the source files existing
- [x] 1.6 Build and run the host tests to confirm the tree is still green before any behaviour changes

## 2. Piece interface and shared movement helpers

- [x] 2.1 Create `src/game_logic/pieces/piece.hpp` declaring an abstract `Piece` with `letter()`, `name()`, `appendMoves(const ChessBoard&, Square from, MoveList&) const`, and `attacks(const ChessBoard&, Square from, Square target) const`, forward-declaring `class ChessBoard` so no header cycle forms
- [x] 2.2 Add an `Offset` type (file and rank deltas) and declare the protected static helpers `appendSlides`, `slideAttacks`, `appendSteps`, and `stepAttacks` on `Piece`
- [x] 2.3 Implement the four helpers in `src/game_logic/pieces/piece.cpp`: sliding walks each direction until it leaves the board or hits a piece, appending empty squares and one enemy capture; stepping tries each offset once with the same friendly-occupancy rule
- [x] 2.4 Implement `Piece::forSquare(char piece)` returning a pointer to the matching singleton for either case, and `nullptr` for an empty square or an unrecognised character
- [x] 2.5 Confirm nothing in `pieces/` includes FreeRTOS or ESP-IDF headers

## 3. The six piece classes

- [x] 3.1 Create `src/game_logic/pieces/knight.hpp/.cpp` with the eight L-offsets, `appendMoves` and `attacks` both routed through the stepping helpers, and a file-scope `const Knight KNIGHT`
- [x] 3.2 Create `src/game_logic/pieces/bishop.hpp/.cpp` with the four diagonal directions routed through the sliding helpers
- [x] 3.3 Create `src/game_logic/pieces/rook.hpp/.cpp` with the four orthogonal directions routed through the sliding helpers
- [x] 3.4 Create `src/game_logic/pieces/queen.hpp/.cpp` with all eight directions routed through the sliding helpers
- [x] 3.5 Create `src/game_logic/pieces/king.hpp/.cpp` whose `appendMoves` emits the eight adjacent steps plus, when the board reports the right and the path is clear, the two castling destinations tagged `CastleKingside` / `CastleQueenside`, and whose `attacks` covers adjacency only and never a castling destination
- [x] 3.6 Create `src/game_logic/pieces/pawn.hpp/.cpp` whose `appendMoves` emits the single advance onto an empty square, the double advance from the home rank when both squares are empty tagged `DoublePawnPush`, the two diagonal captures onto enemy-occupied squares, and the diagonal onto the board's en passant target tagged `EnPassant`, with direction and home rank taken from the piece's colour
- [x] 3.7 In `Pawn::appendMoves`, expand every move that lands on the last rank into four moves carrying promotion pieces `Q`, `R`, `B`, and `N`
- [x] 3.8 Implement `Pawn::attacks` as the two forward diagonals only, independent of what stands on them, so an empty diagonal square still counts as attacked
- [x] 3.9 Register all six singletons in `Piece::forSquare` and add the six source files to `SRCS` in `src/CMakeLists.txt` and to the `chess_tests` target in `tests/CMakeLists.txt`

## 4. Piece movement tests

- [x] 4.1 Add a host test helper that builds a position from an eight-string board literal plus a side to move, so tests can state the position they are testing
- [x] 4.2 Test the knight's eight destinations from a central square and its truncated set from a corner, and that it is unaffected by surrounding pieces
- [x] 4.3 Test that each slider stops at the first occupied square, captures an enemy there, and does not enter a friendly square or continue past either
- [x] 4.4 Test the pawn's single advance, double advance only from the home rank, double advance blocked by a piece on the intermediate square, refusal to capture straight ahead, refusal to move diagonally onto an empty square, and refusal to move backward, for both colours
- [x] 4.5 Test that `Pawn::attacks` reports the diagonals as attacked even when empty, and does not report the square straight ahead
- [x] 4.6 Test that `King::attacks` reports the eight adjacent squares and not a castling destination

## 5. Position state on ChessBoard

- [x] 5.1 Add `side_to_move_`, four castling-right flags, and an en passant file with a sentinel for "none" to `ChessBoard`, with accessors for each and `reset()` restoring White to move, all four rights, and no en passant target
- [x] 5.2 Add `Color colorAt(Square) const`, `bool isEmpty(Square) const`, and `Square kingSquare(Color) const`
- [x] 5.3 Implement `bool isAttacked(Square target, Color by) const` by scanning all 64 squares and asking each piece of colour `by` whether it `attacks` the target, returning on the first hit
- [x] 5.4 Replace `applyMove(Square, Square)` with `void apply(const Move&)` that handles a normal move, a capture, the rook leg of both castles, removing the en passant victim from its own square, and placing the promotion piece with the mover's case
- [x] 5.5 In `apply`, set the en passant file only for a `DoublePawnPush` and clear it for every other move, then flip the side to move
- [x] 5.6 In `apply`, clear any castling right whose king square or rook square equals the move's from-square or to-square, so a rook that moves, a rook that is captured on its home square, and a king that moves all lose the right, and a returning rook does not regain it
- [x] 5.7 Add `bool hasCastlingRight(Color, MoveKind) const` for `King::appendMoves` to consult
- [x] 5.8 Confirm `pieceAt` and the `[rank][file]` layout are unchanged so `BoardSnapshot` and the preview page keep working untouched

## 6. Legality rules

- [x] 6.1 Create `src/game_logic/chess_rules.hpp/.cpp` with a `MoveError` enum covering `None`, `EmptyFromSquare`, `NotYourPiece`, `SameSquare`, `FriendlyPiece`, `Unreachable`, `PathBlocked`, `KingLeftInCheck`, `CastlingRightsLost`, `CastlingPathBlocked`, `CastlingThroughCheck`, `EnPassantUnavailable`, and `GameOver`
- [x] 6.2 Implement `bool leavesKingInCheck(const ChessBoard&, const Move&)` by copying the board, applying the move to the copy, and asking whether the mover's king is attacked on the copy
- [x] 6.3 Implement `MoveError validate(const ChessBoard&, Square from, Square to, char promotion, Move& resolved)`: reject an empty from-square, a piece of the wrong colour, a from-square equal to the to-square, and a friendly piece on the to-square, in that order
- [x] 6.4 Continue `validate` by generating the from-square piece's pseudo-legal moves and searching for one matching the to-square and the requested promotion, filling `resolved` on a hit
- [x] 6.5 On a miss, distinguish `PathBlocked` from `Unreachable` by testing whether the destination lies on one of the piece's lines of travel with an occupied square in between, so a rook aimed down a blocked file reports the blockage rather than an unreachable square
- [x] 6.6 On a miss for a king moving two files, report `CastlingRightsLost`, `CastlingPathBlocked`, or `CastlingThroughCheck` by testing the three conditions in that order, and for a pawn moving diagonally onto an empty square report `EnPassantUnavailable`
- [x] 6.7 Finish `validate` by returning `KingLeftInCheck` when `leavesKingInCheck` is true for the resolved move
- [x] 6.8 In `King::appendMoves`, emit a castling destination only when the right is held, the squares between king and rook are empty, the king is not currently attacked, and neither the square passed over nor the destination is attacked, allowing b1/b8 to be attacked but requiring it empty for the queenside
- [x] 6.9 Implement `bool hasLegalMove(const ChessBoard&)` scanning the squares of the side to move, generating each piece's moves and returning true on the first that survives `leavesKingInCheck`
- [x] 6.10 Implement `PositionStatus status(const ChessBoard&)` returning `Normal`, `Check`, `Checkmate`, or `Stalemate` from `isAttacked` on the king of the side to move and `hasLegalMove`
- [x] 6.11 Implement `const char* describe(MoveError, const ChessBoard&, Square from, Square to, char* out, size_t out_size)` producing the message text from the design's table, including the piece name and square for `Unreachable` and the square for `PathBlocked` and `FriendlyPiece`
- [x] 6.12 Add `chess_rules.cpp` to `SRCS` in `src/CMakeLists.txt` and to the `chess_tests` target

## 7. Legality tests

- [x] 7.1 Test turn order: a Black first move is refused, two White moves in a row are refused, a rejection does not pass the turn, and reset returns the move to White
- [x] 7.2 Test that a rook cannot jump its own pawn and a queen cannot pass through one, replacing the existing `testLegalityIsNotChecked` with the opposite assertions
- [x] 7.3 Test the absolute pin: a knight between its king and an enemy rook cannot move away, a rook in the same position can slide along the pinning line, and a pinned piece can capture the pinner
- [x] 7.4 Test that the king cannot move to an attacked square, cannot slide along the checking ray to another attacked square, and cannot step next to the enemy king
- [x] 7.5 Test answering a check: an unrelated move is refused, blocking is accepted, and capturing the checker is accepted
- [x] 7.6 Test all four castling refusals — pieces in the way, right lost after the king moved, right lost after the rook moved and returned, right lost after the rook was captured on its home square — plus refusal when the king is in check, passes through an attacked square, or lands on one
- [x] 7.7 Test that queenside castling is allowed over an attacked but empty b-file square and refused when b1 is occupied
- [x] 7.8 Test a legal castle for both colours and both sides, asserting the king and rook end on the right squares
- [x] 7.9 Test en passant: accepted immediately after the double advance, refused after any intervening move, refused after a single-square advance, and available to Black as well as White
- [x] 7.10 Test the en passant capture that would expose its own king along a rank, and assert it is refused with `king would be left in check`
- [x] 7.11 Test that every rejection leaves all 64 squares, the side to move, the castling rights, the en passant target, and the move number unchanged, including that a rejected entry does not consume an en passant opportunity
- [x] 7.12 Assert the message text of each `MoveError` so the reasons are pinned by test rather than by eye

## 8. Notation

- [x] 8.1 Create `src/game_logic/chess_notation.hpp/.cpp` and move the existing `formatMove` rendering there as a function taking the position before the move, the resolved `Move`, and the position after it
- [x] 8.2 Render castling from the move kind rather than by inferring it from a two-file king move
- [x] 8.3 Render the promotion suffix from the resolved move's promotion character rather than assuming a queen, for both colours
- [x] 8.4 Render an en passant capture as a normal pawn capture, using the from-square's file and the destination square
- [x] 8.5 Implement disambiguation: collect the squares of same-kind, same-colour pieces that can legally reach the destination, and add the file letter, the rank digit, or the whole from-square by that precedence, skipping pawns and kings
- [x] 8.6 Append `+` or `#` from the status of the position after the move, and nothing for stalemate
- [x] 8.7 Add `chess_notation.cpp` to `SRCS` in `src/CMakeLists.txt` and to the `chess_tests` target, and delete the now-unused notation code from `chess_board.cpp`

## 9. Notation tests

- [x] 9.1 Keep the existing pawn, piece, capture, and castling notation assertions passing, adjusting the setup moves that are no longer legal so the intent of each test is preserved
- [x] 9.2 Test file, rank, and full-square disambiguation, and disambiguation combined with a capture
- [x] 9.3 Test that a pinned twin does not force disambiguation
- [x] 9.4 Test `+` on a checking move, `#` on a mating move, no suffix on stalemate, and `O-O+` for a checking castle
- [x] 9.5 Test all four promotion choices for White and at least one for Black, including a promotion capture and a promotion giving check
- [x] 9.6 Assert the longest realistic notation still fits the widened buffer with its terminator

## 10. Entry state machine and game end

- [x] 10.1 Add `PromotionPending` to `GameEventType` and a `MoveError` plus a `PositionStatus` to `GameEvent`
- [x] 10.2 Add an entry state to `ChessGame` distinguishing collecting digits from awaiting a promotion choice, holding the pending from-square and to-square
- [x] 10.3 On the fourth digit, validate with a queen placeholder; reject immediately on error, enter the awaiting state when the resolved move carries a promotion, and otherwise apply the move
- [x] 10.4 While awaiting, map `A`, `B`, `C`, and `D` to `Q`, `R`, `B`, and `N`, apply the move with that piece, and ignore every other key except reset
- [x] 10.5 Emit `PromotionPending` when the awaiting state is entered, and have `formatOutput` print a line naming the pending move and the four choices
- [x] 10.6 Compute the position status after each accepted move, store the result on the game, and have `formatOutput` append the checkmate or stalemate result line after the move line
- [x] 10.7 Reject every completed entry with `GameOver` once a result is stored, and clear the result on reset along with the awaiting state
- [x] 10.8 Change the rejection line to `<coordinates> = illegal: <reason>` and widen the message buffer in `Game::handleKey` from 64 to 96 bytes
- [x] 10.9 Confirm the move number is consumed only by accepted moves and that a promotion consumes exactly one

## 11. Entry and game-end tests

- [x] 11.1 Test that the fourth digit of a legal promotion produces `PromotionPending` and does not change the board
- [x] 11.2 Test each of `A`, `B`, `C`, and `D` completing the promotion with the right piece, and that digits pressed while awaiting are ignored
- [x] 11.3 Test that an illegal promotion move is rejected on the fourth digit without asking for a piece
- [x] 11.4 Test that reset cancels an awaited promotion and that entry resumes normally afterwards
- [x] 11.5 Test checkmate from a short mating sequence, asserting the `#` suffix, the result line, and that the following move is refused with `game is over, press reset`
- [x] 11.6 Test a stalemate position, asserting no suffix, the draw result line, and the same refusal afterwards
- [x] 11.7 Test that reset after a result starts a new game numbered from 1
- [x] 11.8 Run the full host suite and confirm every test passes with `-Wall -Wextra -Wpedantic -Werror`

## 12. LED patterns

- [x] 12.1 Add a blink-pattern enum to `Led` and change the command queue item from a constant to that pattern
- [x] 12.2 Keep `blinkOnce()` as the single 250 ms on / 250 ms off pulse and add `blinkError()` queuing three cycles of 80 ms on / 80 ms off
- [x] 12.3 Drive the pattern from the received command in `Led::run` so the timing lives in one place
- [x] 12.4 In `Game::handleKey`, switch exhaustively on the event type: single blink for an accepted move, error blink for a rejected move, and no blink for `None`, `PromotionPending`, or `Reset`

## 13. Wiring and firmware build

- [x] 13.1 Confirm `src/CMakeLists.txt` lists `chess_rules.cpp`, `chess_notation.cpp`, and the seven files under `pieces/`, and that `INCLUDE_DIRS` covers `game_logic/pieces`
- [x] 13.2 Raise the game task's stack in `main.cpp` from 2048 to 4096 bytes
- [x] 13.3 Confirm the board snapshot is still published after accepted moves and resets and not after rejections or a pending promotion
- [x] 13.4 Build the firmware with `source /home/arturstef/.espressif/tools/activate_idf_v6.0.2.sh && idf.py build` and confirm no new warnings
- [x] 13.5 Note the reported binary size increase and the free heap after startup, so the rule engine's cost is on record

## 14. Documentation

- [x] 14.1 Rewrite the README's "Chess behavior and limitations" section: legality is now enforced, and the remaining limitations are threefold repetition, the fifty-move rule, insufficient material, and the absence of a position-entry mode
- [x] 14.2 Document the promotion keys `A`/`B`/`C`/`D` and the fifth keypress in the "Entering moves" section, with an example
- [x] 14.3 Document the two LED patterns and what each means
- [x] 14.4 Document the rejection line format with a couple of real examples, and the checkmate and stalemate result lines
- [x] 14.5 Add a short note that the LED now reveals whether a move is legal, which is information a blindfold player is meant to hold themselves, and that rejected attempts are logged to the console for an arbiter to review
- [x] 14.6 Update the project structure listing with `game_logic/pieces/` and the new modules


## 15. On-device verification

- [x] 15.1 Flash the device and play a short legal opening, confirming a single blink and the expected notation for each move
- [x] 15.2 Enter an illegal move and confirm three rapid blinks, an unchanged preview, and a console line naming the reason
- [x] 15.3 Confirm the two blink patterns are distinguishable without looking directly at the LED
- [x] 15.4 Attempt a move for the wrong side and confirm it is refused with the turn message
- [x] 15.5 Castle for both colours, then in a fresh game move the king and back and confirm castling is refused
- [x] 15.6 Play into an en passant opportunity, take it, then in a fresh game let it expire and confirm the refusal
- [x] 15.7 Promote a pawn with each of the four letter keys across separate games and confirm the piece on the board and in the notation
- [x] 15.8 Enter a promotion and press digits without choosing, confirming the device waits and that reset escapes
- [x] 15.9 Play a short mate, confirm the `#` suffix, the result line, and that further moves are refused until reset
- [x] 15.10 Confirm the Wi-Fi preview still updates after accepted moves and never after rejected ones
- [x] 15.11 Enter moves rapidly and confirm no digit is dropped while the LED is mid-pattern
- [x] 15.12 Play a long game to confirm no reboot from stack exhaustion, checking the game task's high-water mark if convenient
