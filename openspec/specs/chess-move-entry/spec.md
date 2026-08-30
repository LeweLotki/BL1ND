# chess-move-entry Specification

## Purpose
TBD - created by archiving change create-chess-task. Update Purpose after archive.
## Requirements
### Requirement: Every physical key is distinguishable from no key

The keypad scanner SHALL report a distinct code for each of the 16 keys in the matrix, and SHALL use a sentinel value that no key can produce to report that nothing is pressed. In particular the key at row 3 / column 0 SHALL be reported with its own code.

#### Scenario: Reset key is reported

- **WHEN** the key at row 3, column 0 is pressed
- **THEN** the scanner reports the reset key code
- **AND** that code differs from the code reported when no key is pressed

#### Scenario: No key pressed

- **WHEN** no key in the matrix is pressed
- **THEN** the scanner reports the "nothing pressed" sentinel

#### Scenario: Every key has a unique code

- **WHEN** each of the 16 keys is pressed in turn
- **THEN** each press reports a code unique to that key

### Requirement: A move is entered as four digits

The system SHALL accumulate four keypresses in the range 1–8 as a coordinate move, in the order from-file, from-rank, to-file, to-rank. A digit `d` SHALL map to file or rank index `d - 1`, so 1 maps to file `a` or rank 1 and 8 maps to file `h` or rank 8. On the fourth digit the move SHALL be completed and processed; before that no move SHALL be processed.

#### Scenario: Four digits complete a move

- **WHEN** the player presses 5, 2, 5, 4 from the starting position
- **THEN** the move e2 to e4 is processed

#### Scenario: Partial entry does not act

- **WHEN** the player has pressed fewer than four digits
- **THEN** no move is processed, no output line is printed, and the board is unchanged

#### Scenario: Entry restarts after a completed move

- **WHEN** a move has just been completed
- **THEN** the next digit pressed begins a new four-digit move

### Requirement: Keys outside the digit range are ignored during entry

While a move is being entered, the system SHALL discard any key that is not a digit in the range 1–8 and is not the reset key, and SHALL leave the partially entered move intact.

#### Scenario: Off-board digit is discarded

- **WHEN** the player presses 5, 2, then 9
- **THEN** the 9 is discarded and the entry still holds the two digits 5 and 2
- **AND** pressing 5 then 4 completes the move e2 to e4

#### Scenario: Letter key is discarded

- **WHEN** the player presses a non-digit key such as `A` mid-entry
- **THEN** the key is discarded and the partial entry is unchanged

#### Scenario: Zero is discarded

- **WHEN** the player presses the 0 key mid-entry
- **THEN** the key is discarded and the partial entry is unchanged

### Requirement: Reset key restarts the game

Pressing the reset key SHALL restore the board to the standard starting position, discard any partially entered move, and set the move number back to 1. The reset SHALL take effect regardless of how many digits have been entered.

#### Scenario: Reset after moves have been played

- **WHEN** several moves have been played and the player presses the reset key
- **THEN** the board holds the standard starting position
- **AND** the next completed move is printed with move number 1

#### Scenario: Reset mid-entry

- **WHEN** the player has entered two digits and presses the reset key
- **THEN** the partial entry is discarded
- **AND** the next four digits are read as a fresh move

### Requirement: LED confirms an accepted move

The LED SHALL blink exactly once when a move is accepted. It SHALL NOT blink for individual keypresses, for a partially entered move, or for a move that was rejected.

#### Scenario: Blink on accepted move

- **WHEN** a four-digit move is accepted
- **THEN** the LED blinks once

#### Scenario: No blink on individual keypress

- **WHEN** the player presses the first, second, or third digit of a move
- **THEN** the LED does not blink

#### Scenario: No blink on rejected move

- **WHEN** a four-digit move is rejected because the from-square is empty
- **THEN** the LED does not blink

### Requirement: Keypresses in a move are not dropped

The path from the keypad scanner to the move-entry logic SHALL buffer keypresses in first-in-first-out order and SHALL NOT discard an earlier press because a later one arrived. All four digits of a move SHALL be delivered in the order they were pressed.

#### Scenario: Rapid entry preserves order

- **WHEN** the player presses four digits in quick succession
- **THEN** all four are received in the order pressed and the intended move is processed

### Requirement: Peripherals emit no key-level output

The keypad SHALL NOT print a line to standard output for each keypress. Standard output SHALL carry only move results and rejection messages.

#### Scenario: Pressing a digit prints nothing

- **WHEN** the player presses the first digit of a move
- **THEN** nothing is printed to standard output

