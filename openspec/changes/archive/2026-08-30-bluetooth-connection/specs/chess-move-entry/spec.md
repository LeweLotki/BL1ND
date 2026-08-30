## MODIFIED Requirements

### Requirement: Keys outside the digit range are ignored during entry

While the four digits of a move are being entered, the system SHALL discard any key that is not a digit in the range 1–8, is not the reset key, and is not a three-second hold of `B`, and SHALL leave the partially entered move intact. The keys `A`, `B`, `C`, and `D` SHALL carry meaning as promotion choices only while a promotion choice is awaited. A three-second hold of `B` SHALL carry meaning at any time and SHALL leave a partial entry intact.

#### Scenario: Off-board digit is discarded

- **WHEN** the player presses 5, 2, then 9
- **THEN** the 9 is discarded and the entry still holds the two digits 5 and 2
- **AND** pressing 5 then 4 completes the move e2 to e4

#### Scenario: Letter key is discarded mid-entry

- **WHEN** the player presses and releases a non-digit key such as `A` while entering the four digits and no promotion is awaited
- **THEN** the key is discarded and the partial entry is unchanged

#### Scenario: Zero is discarded

- **WHEN** the player presses the 0 key mid-entry
- **THEN** the key is discarded and the partial entry is unchanged

#### Scenario: Discarded keys do not signal

- **WHEN** a discarded key is pressed
- **THEN** the LED does not signal and nothing is printed

#### Scenario: Holding B mid-entry does not disturb the entry

- **WHEN** the player has entered two digits and holds `B` for three seconds
- **THEN** the pairing or unlinking action is taken and the two entered digits are still held

### Requirement: Reset key restarts the game

Pressing the reset key SHALL restore the board to the standard starting position, discard any partially entered move, discard an awaited promotion choice, clear any checkmate or stalemate result, return the side to move to White, restore full castling rights, clear any en passant target, and set the move number back to 1. The reset SHALL take effect regardless of how many digits have been entered and regardless of whether the game has ended. When the device is linked to a peer, the same reset SHALL also be applied on the peer and the two boards SHALL be assigned new colours.

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

#### Scenario: Reset while linked resets both boards

- **WHEN** the player presses the reset key on a linked board
- **THEN** both that board and its peer hold the standard starting position with the move number back to 1

#### Scenario: Reset while unlinked stays local

- **WHEN** the player presses the reset key on a board with no link
- **THEN** the board resets and nothing is transmitted

### Requirement: LED confirms an accepted move

The LED SHALL signal the outcome of every completed move with one of two distinguishable patterns. A move that is accepted SHALL produce a single blink. A move that is rejected SHALL produce three rapid blinks. The LED SHALL NOT signal for individual keypresses, for a partially entered move, for a discarded key, or while a promotion choice is awaited. The LED SHALL NOT signal for a reset that is purely local, but SHALL announce the newly assigned colour after a reset that is shared with a peer. The LED SHALL additionally signal link events with patterns distinguishable from both move patterns: five blinks when a link is established, and one or two long blinks to announce the assigned colour.

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

#### Scenario: No signal on a local reset

- **WHEN** the player presses the reset key on an unlinked board
- **THEN** the LED does not signal

#### Scenario: Colour announced on a linked reset

- **WHEN** the player presses the reset key on a linked board
- **THEN** the LED announces the colour assigned for the new game

#### Scenario: Link patterns are distinguishable from move patterns

- **WHEN** the five-blink link pattern or a colour announcement is shown
- **THEN** it is distinguishable from the single accepted-move blink and from the three rapid rejected-move blinks

#### Scenario: A move arriving from a peer blinks once

- **WHEN** a move received over the link is applied
- **THEN** the LED blinks once, as it would for a move entered on the keypad

### Requirement: Peripherals emit no key-level output

The keypad SHALL NOT print a line to standard output for each keypress. Standard output SHALL carry move results, rejection messages, and link status — pairing, connection, colour assignment, disconnection, and position mismatches — and nothing else.

#### Scenario: Pressing a digit prints nothing

- **WHEN** the player presses the first digit of a move
- **THEN** nothing is printed to standard output

#### Scenario: Link events are printed

- **WHEN** the device enters pairing mode, links to a peer, is assigned a colour, loses the link, or detects a position mismatch
- **THEN** a line describing that event is printed to standard output

#### Scenario: Holding B prints nothing until the hold completes

- **WHEN** the player presses `B` and has held it for less than three seconds
- **THEN** nothing is printed to standard output

### Requirement: A promotion is completed by a letter key

While a promotion choice is awaited, the system SHALL accept `A` for a queen, `B` for a rook, `C` for a bishop, and `D` for a knight, and SHALL then apply the move with the chosen piece. The choices carried by `A`, `C`, and `D` SHALL take effect when the key is pressed. The choice carried by `B` SHALL take effect when the key is released, because a press of `B` may still become a three-second hold; a `B` held for three seconds SHALL be a hold and SHALL NOT also choose a rook. Until one of those four keys completes its choice, the system SHALL ignore every other key except the reset key and the `B` hold, SHALL keep the awaited move pending, and SHALL NOT change the board.

#### Scenario: Choosing a queen

- **WHEN** a promotion is awaited and the player presses `A`
- **THEN** the move is applied with a queen on the promotion square

#### Scenario: Choosing a rook

- **WHEN** a promotion is awaited and the player presses `B` and releases it within three seconds
- **THEN** the move is applied with a rook on the promotion square

#### Scenario: Choosing a bishop

- **WHEN** a promotion is awaited and the player presses `C`
- **THEN** the move is applied with a bishop on the promotion square

#### Scenario: Choosing a knight

- **WHEN** a promotion is awaited and the player presses `D`
- **THEN** the move is applied with a knight on the promotion square

#### Scenario: Holding B does not promote

- **WHEN** a promotion is awaited and the player holds `B` for three seconds
- **THEN** the hold is taken as a pairing or unlinking action, no rook is placed, and the promotion is still awaited

#### Scenario: Digits are ignored while a promotion is awaited

- **WHEN** a promotion is awaited and the player presses digits
- **THEN** the digits are ignored, no new move begins, and the promotion is still awaited

#### Scenario: Entry resumes after the choice

- **WHEN** a promotion has been completed
- **THEN** the next digit pressed begins a new four-digit move

#### Scenario: The promotion square is announced

- **WHEN** a promotion choice becomes awaited
- **THEN** a line is printed to standard output naming the pending move and the four available choices

## ADDED Requirements

### Requirement: A three-second hold of B is reported as its own event

The keypad scanner SHALL distinguish a press of `B` from a hold of `B`. A hold SHALL be reported once, three seconds after the key goes down, and SHALL NOT be reported again while the key remains down. The press meaning of `B` SHALL be reported on release, and only when the key was released before three seconds elapsed. No other key SHALL have its meaning deferred to release, and no other key SHALL produce a hold event.

#### Scenario: A hold is reported once

- **WHEN** the player holds `B` down for ten seconds
- **THEN** a single hold event is reported, three seconds in, and nothing further is reported until the key is released and pressed again

#### Scenario: A short press is reported on release

- **WHEN** the player presses `B` and releases it after half a second
- **THEN** the `B` key is reported once, at release

#### Scenario: A completed hold suppresses the press

- **WHEN** the player holds `B` for three seconds and then releases it
- **THEN** the hold has been reported and no `B` keypress is reported on release

#### Scenario: Other letter keys are unaffected

- **WHEN** the player presses and holds `A`, `C`, or `D` for any length of time
- **THEN** that key is reported once when it goes down and no hold event is produced

#### Scenario: Digits are unaffected

- **WHEN** the player holds a digit key down
- **THEN** the digit is reported once when it goes down, as it is today

### Requirement: A completed entry is refused when the board does not own the side to move

When the device is linked to a peer, a completed four-digit entry SHALL be refused unless the side to move is the colour assigned to this board. The refusal SHALL happen after the fourth digit, SHALL leave the board unchanged, SHALL NOT consume the move number, SHALL blink the rejected-move pattern, SHALL print a reason naming the situation, and SHALL NOT transmit anything to the peer. When the device is not linked, entries for either colour SHALL be accepted as before.

#### Scenario: Entry on the peer's turn is refused

- **WHEN** a board assigned White completes an entry while it is Black's turn
- **THEN** the entry is refused, the board is unchanged, and the LED blinks the rejection pattern

#### Scenario: The refusal is explained

- **WHEN** an entry is refused because the board does not own the side to move
- **THEN** a line is printed identifying the entry and giving the reason

#### Scenario: The refusal is distinct from an illegal move

- **WHEN** an entry is refused for ownership rather than legality
- **THEN** the printed reason names the turn rather than a chess rule

#### Scenario: Unlinked play is unaffected

- **WHEN** the device holds no link and an entry is completed for either colour
- **THEN** the entry is judged on legality alone
