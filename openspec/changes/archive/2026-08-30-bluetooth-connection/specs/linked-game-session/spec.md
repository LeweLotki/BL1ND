## ADDED Requirements

### Requirement: Colours are assigned randomly and agreed by both boards

When a link is established the two boards SHALL agree that one plays White and the other plays Black. Each board SHALL generate a random token, the two boards SHALL exchange tokens over the link, and both SHALL derive the assignment from the pair of tokens by the same rule, so that both reach the same conclusion without either board choosing for the other. Neither board SHALL be able to select its own colour, and the assignment SHALL NOT depend on which board initiated the connection in a way that makes it predictable across sessions.

#### Scenario: Opposite colours are assigned

- **WHEN** two boards complete a link
- **THEN** one board is assigned White and the other is assigned Black

#### Scenario: Both boards agree

- **WHEN** the colour assignment is made
- **THEN** each board's view of its own colour and of its peer's colour agree with the other board's view

#### Scenario: The assignment is not fixed

- **WHEN** the same two boards are paired repeatedly
- **THEN** which board receives White varies between sessions

#### Scenario: The assignment is reported

- **WHEN** colours are assigned
- **THEN** each board prints the colour it is playing to standard output

### Requirement: The assigned colour is announced on the LED

Because the player cannot read standard output, the device SHALL announce its assigned colour with the LED whenever the colour is assigned or reassigned. White SHALL be announced with one long blink and Black with two long blinks. The announcement SHALL follow the five-blink link confirmation rather than replacing it, and the long blinks SHALL be distinguishable from the single blink of an accepted move.

#### Scenario: White is announced

- **WHEN** a board is assigned White
- **THEN** after the link confirmation the LED shows one long blink

#### Scenario: Black is announced

- **WHEN** a board is assigned Black
- **THEN** after the link confirmation the LED shows two long blinks

#### Scenario: The announcement follows the confirmation

- **WHEN** a link is established
- **THEN** the LED shows five blinks, then a pause, then the colour announcement

#### Scenario: Reassignment is announced again

- **WHEN** colours are reassigned after a linked reset
- **THEN** each board announces its new colour on the LED

### Requirement: A linked board accepts moves only for its own colour

While a link exists, the device SHALL accept a completed four-digit entry only when the side to move is the colour assigned to that board. When it is the peer's turn, the device SHALL reject the completed entry, SHALL leave the board unchanged, SHALL NOT consume the move number, SHALL blink the rejected-move pattern, and SHALL print a reason naming the turn. Digits SHALL still be collected while it is the peer's turn, so that the rejection happens on the fourth digit like every other rejection.

#### Scenario: Moving on your own turn

- **WHEN** a board assigned White completes a legal move while it is White's turn
- **THEN** the move is accepted

#### Scenario: Moving on the peer's turn

- **WHEN** a board assigned White completes an entry while it is Black's turn
- **THEN** the entry is rejected, the board is unchanged, the move number is unchanged, and the LED blinks the rejection pattern

#### Scenario: The rejection names the reason

- **WHEN** an entry is rejected because it is the peer's turn
- **THEN** a line is printed identifying the entry and giving the reason that it is the opponent's turn

#### Scenario: A legal move for the wrong colour is still refused

- **WHEN** a board assigned White completes an entry that is a legal Black move while it is Black's turn
- **THEN** the entry is rejected because the board does not play Black

### Requirement: An accepted move is transmitted to the peer immediately

When a linked board accepts a move, it SHALL send that move to its peer as soon as the move has been applied locally, without waiting for another keypress, a timer, or a poll. A move that is rejected SHALL NOT be transmitted. A promotion SHALL be transmitted once, when the promotion piece has been chosen and the move applied, and SHALL carry the chosen piece.

#### Scenario: A move is sent as soon as it is applied

- **WHEN** a linked board accepts a move
- **THEN** the move is transmitted to the peer without any further player action

#### Scenario: A rejected move is not sent

- **WHEN** a linked board rejects an entry for any reason
- **THEN** nothing is transmitted to the peer

#### Scenario: A pending promotion is not sent

- **WHEN** four digits complete a legal pawn move to the last rank and the promotion piece has not yet been chosen
- **THEN** nothing is transmitted until the piece is chosen

#### Scenario: The promotion piece is carried

- **WHEN** a promotion to a knight is accepted on one board
- **THEN** the peer's board shows a knight on the promotion square

### Requirement: A move received from the peer is applied as if it had been entered locally

A move that arrives over the link SHALL be validated against the receiving board's own position and, when legal, applied. The receiving board SHALL then print the move with its move number in the same form as a locally entered move, SHALL blink the accepted-move pattern, SHALL publish the new position to the board preview, and SHALL evaluate checkmate and stalemate exactly as it would for a local move. A received move SHALL NOT be re-transmitted back to the peer.

#### Scenario: The far board follows the move

- **WHEN** White plays a move on its board
- **THEN** the Black board applies the same move and it becomes Black's turn on both boards

#### Scenario: The far board signals the move

- **WHEN** a move arrives over the link and is applied
- **THEN** the LED blinks once and the move is printed with its move number

#### Scenario: The preview follows the move

- **WHEN** a move arrives over the link and is applied
- **THEN** the board preview page on the receiving device shows the new position

#### Scenario: Mate is detected on both boards

- **WHEN** a move arriving over the link delivers checkmate
- **THEN** the receiving board prints the result and refuses further moves until reset, as it would for a locally entered mate

#### Scenario: No echo

- **WHEN** a board applies a move received from its peer
- **THEN** it does not send that move back

### Requirement: Reset on either board resets both and reassigns colours

Pressing the reset key on a linked board SHALL reset both boards to the standard starting position, with White to move, full castling rights, no en passant target, no partial entry, no awaited promotion, no stored game result, and the move number back to 1. Both boards SHALL then generate fresh tokens, exchange them, derive a new colour assignment by the same rule as at link time, and announce the new colour on the LED. The reset SHALL take effect on both boards regardless of which one initiated it and regardless of whose turn it was.

#### Scenario: Reset propagates

- **WHEN** the player presses the reset key on either linked board
- **THEN** both boards hold the standard starting position and the next accepted move is numbered 1

#### Scenario: Colours are re-rolled

- **WHEN** a linked reset completes
- **THEN** both boards derive a new colour assignment and each announces its colour on the LED

#### Scenario: The reassignment can change sides

- **WHEN** linked resets are repeated
- **THEN** a board that was White does not always remain White

#### Scenario: Reset clears a finished game on both boards

- **WHEN** the game has ended in checkmate and either board is reset
- **THEN** both boards accept moves again from the starting position

#### Scenario: Reset clears a pending promotion on both boards

- **WHEN** one board is awaiting a promotion choice and the other board is reset
- **THEN** the awaited promotion is abandoned and both boards hold the starting position

### Requirement: A board with a lost link does not play both sides

While the link is lost but not deliberately ended, the device SHALL keep the colour it was assigned and SHALL keep refusing entries made on the peer's turn. It SHALL NOT silently fall back to accepting moves for both colours, because doing so would let one player advance the game while the other board cannot follow.

#### Scenario: The colour survives a dropped link

- **WHEN** the link drops on a board assigned White
- **THEN** that board still refuses entries made while it is Black's turn

#### Scenario: Moves are refused while the link is down

- **WHEN** the link is down and it is the board's own turn
- **THEN** the entry is refused with a reason naming the missing link, and the board is unchanged

#### Scenario: Play resumes on reconnection

- **WHEN** the link is re-established and the positions have been reconciled
- **THEN** the board accepts moves for its own colour again

### Requirement: An unlinked board behaves exactly as it did before

When no link has ever been formed, or after a link has been deliberately ended, the device SHALL behave as it does without this capability: moves for either colour may be entered on the same board, reset is local, and no colour is announced.

#### Scenario: A fresh board plays both sides

- **WHEN** the device has been powered on and never paired
- **THEN** moves for White and Black may both be entered on it

#### Scenario: Reset is local when unlinked

- **WHEN** the reset key is pressed on an unlinked board
- **THEN** the board resets and the LED does not signal

#### Scenario: No colour is announced when unlinked

- **WHEN** the device starts up unlinked
- **THEN** no colour announcement is shown on the LED
