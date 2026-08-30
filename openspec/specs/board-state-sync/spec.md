# board-state-sync Specification

## Purpose
TBD - created by archiving change bluetooth-connection. Update Purpose after archive.
## Requirements
### Requirement: A position has a canonical encoding that captures everything that affects play

The system SHALL define one canonical byte encoding of a game position, covering the occupant of all 64 squares, the side to move, all four castling rights, the en passant target if any, and the move number. Two positions that differ in any of those SHALL produce different encodings, and two positions that agree in all of them SHALL produce identical encodings. The encoding SHALL NOT depend on how the position was reached.

#### Scenario: Identical positions encode identically

- **WHEN** two boards reach the same position by different move orders
- **THEN** the two encodings are byte-for-byte identical

#### Scenario: Castling rights are captured

- **WHEN** two positions have identical pieces, side to move, en passant target, and move number but differ in one castling right
- **THEN** the two encodings differ

#### Scenario: En passant availability is captured

- **WHEN** two positions are identical except that one has an en passant target and the other does not
- **THEN** the two encodings differ

#### Scenario: Side to move is captured

- **WHEN** two positions have identical pieces but different sides to move
- **THEN** the two encodings differ

### Requirement: Every transmitted move carries a hash of the sender's resulting position

A move sent over the link SHALL carry a hash of the canonical encoding of the sender's position after that move has been applied. The hash SHALL be computed by a deterministic function that both boards implement identically. The hash exists to detect divergence caused by a bug, a lost message, or a corrupted transfer, and is not required to resist a deliberate attack.

#### Scenario: The hash accompanies the move

- **WHEN** a board transmits an accepted move
- **THEN** the message carries the hash of the position that board holds after the move

#### Scenario: The hash is deterministic

- **WHEN** both boards hash the same canonical encoding
- **THEN** they produce the same value

#### Scenario: Different positions hash differently

- **WHEN** two positions differ by a single piece, by the side to move, by a castling right, by the en passant target, or by the move number
- **THEN** their hashes differ

### Requirement: The receiver acknowledges agreement after applying a move

On receiving a move, the board SHALL apply it, compute the hash of its own resulting position, and compare it with the hash carried by the message. When the two agree, the board SHALL send an acknowledgement carrying that hash and SHALL treat the move as complete. The sender SHALL treat an acknowledgement with the expected hash as confirmation that both boards hold the same position.

#### Scenario: Agreement is acknowledged

- **WHEN** a received move is applied and the resulting hashes agree
- **THEN** the receiver acknowledges the move with the matching hash

#### Scenario: The sender is satisfied

- **WHEN** the sender receives an acknowledgement carrying the hash it sent
- **THEN** the move is complete and no further exchange takes place for it

#### Scenario: A missing acknowledgement is noticed

- **WHEN** a sender does not receive an acknowledgement within the acknowledgement timeout
- **THEN** it does not treat the move as agreed and begins recovery

### Requirement: Disagreement is detected rather than ignored

A board SHALL treat any of the following as a disagreement about the position: an acknowledged hash that differs from the sender's, a received move whose accompanying hash differs from the receiver's own result, a received move that is not legal in the receiver's position, and a received move whose sequence number is not the one expected next. A disagreement SHALL NOT be silently discarded and SHALL NOT leave the two boards continuing to play from different positions.

#### Scenario: Hash mismatch on receipt

- **WHEN** a received move applies legally but the resulting hashes differ
- **THEN** the receiver treats it as a disagreement and requests resynchronisation

#### Scenario: A received move is illegal locally

- **WHEN** a received move is not legal in the receiver's position
- **THEN** the receiver does not apply it, treats it as a disagreement, and requests resynchronisation

#### Scenario: An unexpected sequence number

- **WHEN** a received move carries a sequence number other than the next one expected
- **THEN** the receiver treats it as a disagreement rather than applying it

#### Scenario: A duplicate move is not applied twice

- **WHEN** a move already applied is received again with the same sequence number and hash
- **THEN** it is acknowledged again and the board is not changed a second time

### Requirement: Disagreement is repaired by sending the full position

When a disagreement is detected, the board that made the move SHALL send the full canonical encoding of its position, and the other board SHALL adopt it in place of its own, replacing all 64 squares, the side to move, the castling rights, the en passant target, and the move number. Any partial entry or awaited promotion on the adopting board SHALL be discarded. After adopting, the two boards SHALL confirm that their hashes now agree before play resumes.

#### Scenario: The mover is authoritative

- **WHEN** a disagreement is detected for a move that one board made and applied
- **THEN** that board's position is the one both boards end up holding

#### Scenario: The position is fully replaced

- **WHEN** a board adopts a resynchronised position
- **THEN** its pieces, side to move, castling rights, en passant target, and move number all match the sender's

#### Scenario: Entry state is discarded on adoption

- **WHEN** a board with two digits entered adopts a resynchronised position
- **THEN** the partial entry is discarded and the next digit begins a new move

#### Scenario: Agreement is confirmed after repair

- **WHEN** resynchronisation completes
- **THEN** the two boards exchange hashes and confirm they agree before either accepts a new move

### Requirement: A repaired disagreement is signalled and logged

A disagreement SHALL be visible rather than silent. Both boards SHALL blink the rejected-move pattern when a disagreement is detected and repaired, and both SHALL print a line to standard output recording that a mismatch occurred, which move it was detected on, and the two hashes involved.

#### Scenario: Both boards signal

- **WHEN** a disagreement is detected and repaired
- **THEN** both boards blink the rejection pattern

#### Scenario: The mismatch is logged

- **WHEN** a disagreement is detected
- **THEN** a line is printed naming the move and both hashes

#### Scenario: The correction is distinguishable from a normal move

- **WHEN** a board adopts a resynchronised position
- **THEN** its output makes clear that the position was corrected rather than advanced by a move

### Requirement: Reconnection reconciles the two positions before play resumes

When a dropped link is re-established, the boards SHALL compare their positions before accepting any new move. If the hashes agree, play SHALL resume with no further exchange. If they disagree, the board that has played more moves SHALL send its full position and the other SHALL adopt it; if both have played the same number of moves and disagree, the board that initiated the connection SHALL be authoritative. The reconciliation SHALL be reported on standard output.

#### Scenario: Nothing was missed

- **WHEN** the link is re-established and both boards hold the same position
- **THEN** play resumes immediately and neither board's position is replaced

#### Scenario: One board missed a move

- **WHEN** the link is re-established and one board has applied one more move than the other
- **THEN** the board with more moves sends its position and the other adopts it

#### Scenario: An unresolvable tie

- **WHEN** the link is re-established, the boards disagree, and both have played the same number of moves
- **THEN** the initiating board's position is adopted by the other and the substitution is reported

#### Scenario: No move is accepted before reconciliation

- **WHEN** a link has just been re-established and reconciliation has not completed
- **THEN** a completed entry is refused rather than applied and transmitted

### Requirement: Repeated failure to agree stops play rather than continuing

If resynchronisation does not produce agreement after a bounded number of attempts, the device SHALL stop accepting moves, SHALL report that the boards could not be reconciled, and SHALL require either a reset or an unlink to continue. It SHALL NOT keep playing from a position it knows may differ from its peer's.

#### Scenario: Resynchronisation keeps failing

- **WHEN** the boards fail to reach agreeing hashes after the permitted number of attempts
- **THEN** both stop accepting moves and report that they could not be reconciled

#### Scenario: Reset escapes the state

- **WHEN** the boards are unreconciled and the reset key is pressed
- **THEN** both return to the starting position and play resumes

#### Scenario: Unlink escapes the state

- **WHEN** the boards are unreconciled and the player holds `B` for three seconds
- **THEN** that board returns to unlinked operation and accepts moves for both colours again

