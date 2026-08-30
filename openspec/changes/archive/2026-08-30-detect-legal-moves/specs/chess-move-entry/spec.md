## MODIFIED Requirements

### Requirement: A move is entered as four digits

The system SHALL accumulate four keypresses in the range 1–8 as a coordinate move, in the order from-file, from-rank, to-file, to-rank. A digit `d` SHALL map to file or rank index `d - 1`, so 1 maps to file `a` or rank 1 and 8 maps to file `h` or rank 8. On the fourth digit the move SHALL be judged; before that no move SHALL be judged. A move that is not a pawn move to the last rank SHALL be completed on the fourth digit, either accepted or rejected. A legal pawn move to the last rank SHALL instead leave the entry awaiting a promotion choice.

#### Scenario: Four digits complete a move

- **WHEN** the player presses 5, 2, 5, 4 from the starting position
- **THEN** the move e2 to e4 is judged and accepted

#### Scenario: Partial entry does not act

- **WHEN** the player has pressed fewer than four digits
- **THEN** no move is judged, no output line is printed, the LED does not signal, and the board is unchanged

#### Scenario: Entry restarts after a completed move

- **WHEN** a move has just been completed, whether accepted or rejected
- **THEN** the next digit pressed begins a new four-digit move

#### Scenario: Fourth digit of a promotion does not complete the move

- **WHEN** the four digits describe a legal pawn move to the last rank
- **THEN** the move is not yet applied and the entry awaits a promotion choice

### Requirement: Keys outside the digit range are ignored during entry

While the four digits of a move are being entered, the system SHALL discard any key that is not a digit in the range 1–8 and is not the reset key, and SHALL leave the partially entered move intact. The keys `A`, `B`, `C`, and `D` SHALL carry meaning only while a promotion choice is awaited.

#### Scenario: Off-board digit is discarded

- **WHEN** the player presses 5, 2, then 9
- **THEN** the 9 is discarded and the entry still holds the two digits 5 and 2
- **AND** pressing 5 then 4 completes the move e2 to e4

#### Scenario: Letter key is discarded mid-entry

- **WHEN** the player presses a non-digit key such as `A` while entering the four digits and no promotion is awaited
- **THEN** the key is discarded and the partial entry is unchanged

#### Scenario: Zero is discarded

- **WHEN** the player presses the 0 key mid-entry
- **THEN** the key is discarded and the partial entry is unchanged

#### Scenario: Discarded keys do not signal

- **WHEN** a discarded key is pressed
- **THEN** the LED does not signal and nothing is printed

### Requirement: Reset key restarts the game

Pressing the reset key SHALL restore the board to the standard starting position, discard any partially entered move, discard an awaited promotion choice, clear any checkmate or stalemate result, return the side to move to White, restore full castling rights, clear any en passant target, and set the move number back to 1. The reset SHALL take effect regardless of how many digits have been entered and regardless of whether the game has ended.

#### Scenario: Reset after moves have been played

- **WHEN** several moves have been played and the player presses the reset key
- **THEN** the board holds the standard starting position
- **AND** the next completed move is printed with move number 1

#### Scenario: Reset mid-entry

- **WHEN** the player has entered two digits and presses the reset key
- **THEN** the partial entry is discarded
- **AND** the next four digits are read as a fresh move

#### Scenario: Reset cancels an awaited promotion

- **WHEN** the player has entered a promotion move and presses the reset key before choosing a piece
- **THEN** the promotion is abandoned, the pawn stays where it started the game, and the board holds the starting position

#### Scenario: Reset after the game has ended

- **WHEN** the player presses the reset key after checkmate or stalemate
- **THEN** moves are accepted again from the starting position

### Requirement: LED confirms an accepted move

The LED SHALL signal the outcome of every completed move with one of two distinguishable patterns. A move that is accepted SHALL produce a single blink. A move that is rejected SHALL produce three rapid blinks. The LED SHALL NOT signal for individual keypresses, for a partially entered move, for a discarded key, for a reset, or while a promotion choice is awaited.

#### Scenario: Single blink on accepted move

- **WHEN** a legal move is accepted
- **THEN** the LED blinks once

#### Scenario: Three rapid blinks on rejected move

- **WHEN** a completed move entry is rejected as illegal
- **THEN** the LED blinks three times in rapid succession
- **AND** the pattern is distinguishable from the single blink by its duration and rhythm

#### Scenario: No signal on individual keypress

- **WHEN** the player presses the first, second, or third digit of a move
- **THEN** the LED does not signal

#### Scenario: No signal while awaiting a promotion choice

- **WHEN** the fourth digit completes a legal pawn move to the last rank
- **THEN** the LED does not signal until the promotion piece is chosen

#### Scenario: Promotion blinks once when completed

- **WHEN** the player chooses a promotion piece and the move is applied
- **THEN** the LED blinks once

#### Scenario: No signal on reset

- **WHEN** the player presses the reset key
- **THEN** the LED does not signal

## ADDED Requirements

### Requirement: A promotion is completed by a letter key

While a promotion choice is awaited, the system SHALL accept `A` for a queen, `B` for a rook, `C` for a bishop, and `D` for a knight, and SHALL then apply the move with the chosen piece. Until one of those four keys is pressed, the system SHALL ignore every other key except the reset key, SHALL keep the awaited move pending, and SHALL NOT change the board.

#### Scenario: Choosing a queen

- **WHEN** a promotion is awaited and the player presses `A`
- **THEN** the move is applied with a queen on the promotion square

#### Scenario: Choosing a rook

- **WHEN** a promotion is awaited and the player presses `B`
- **THEN** the move is applied with a rook on the promotion square

#### Scenario: Choosing a bishop

- **WHEN** a promotion is awaited and the player presses `C`
- **THEN** the move is applied with a bishop on the promotion square

#### Scenario: Choosing a knight

- **WHEN** a promotion is awaited and the player presses `D`
- **THEN** the move is applied with a knight on the promotion square

#### Scenario: Digits are ignored while a promotion is awaited

- **WHEN** a promotion is awaited and the player presses digits
- **THEN** the digits are ignored, no new move begins, and the promotion is still awaited

#### Scenario: Entry resumes after the choice

- **WHEN** a promotion has been completed
- **THEN** the next digit pressed begins a new four-digit move

#### Scenario: The promotion square is announced

- **WHEN** a promotion choice becomes awaited
- **THEN** a line is printed to standard output naming the pending move and the four available choices
