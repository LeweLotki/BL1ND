# bluetooth-pairing Specification

## Purpose
TBD - created by archiving change bluetooth-connection. Update Purpose after archive.
## Requirements
### Requirement: Pairing is armed by holding the B key for three seconds

The device SHALL enter pairing mode when the `B` key is held continuously for three seconds while the device is not linked. A shorter press SHALL NOT enter pairing mode. Releasing `B` before three seconds have elapsed SHALL leave the device exactly as it was, with any partially entered move and any awaited promotion intact. Entering pairing mode SHALL be reported on standard output.

#### Scenario: Three-second hold arms pairing

- **WHEN** the player holds `B` for three seconds on an unlinked device
- **THEN** the device enters pairing mode and reports it on standard output

#### Scenario: Short press does not arm pairing

- **WHEN** the player presses and releases `B` in less than three seconds
- **THEN** the device does not enter pairing mode

#### Scenario: Hold does not disturb a partial entry

- **WHEN** the player has entered two digits, then holds `B` for three seconds
- **THEN** pairing mode is entered and the two entered digits are still held

#### Scenario: The hold must be continuous

- **WHEN** the player holds `B` for two seconds, releases it, and immediately holds it again for two seconds
- **THEN** pairing mode is not entered

### Requirement: Both boards advertise and scan during the pairing window

While in pairing mode a device SHALL both make itself discoverable and search for a peer at the same time, so that two devices entering pairing mode independently can find each other with no board designated in advance and no order required between the two holds. A device SHALL only consider a peer that is itself in pairing mode and running this firmware, identified by a dedicated service identifier that no other device advertises.

#### Scenario: Either order of holds works

- **WHEN** one board enters pairing mode several seconds before the other
- **THEN** the two boards still discover each other once the second one enters pairing mode

#### Scenario: Simultaneous holds work

- **WHEN** both boards enter pairing mode at the same moment
- **THEN** the two boards discover each other

#### Scenario: Unrelated devices are ignored

- **WHEN** a phone, a headset, or any other Bluetooth device is discoverable nearby
- **THEN** the board does not attempt to link to it

#### Scenario: A board not in pairing mode is not joined

- **WHEN** a board that is not in pairing mode is within range of a board that is
- **THEN** no link is formed

### Requirement: Exactly one of the two boards initiates the connection

When two boards discover each other, exactly one SHALL initiate the connection and the other SHALL accept it. The choice SHALL be made from the random tokens the two boards generated for this pairing attempt, by a rule that both boards evaluate identically, so that the decision needs no negotiation round and cannot select both or neither. If the two tokens are equal, both boards SHALL generate new tokens and retry within the remaining pairing window.

#### Scenario: One initiator is chosen

- **WHEN** two boards in pairing mode discover each other
- **THEN** one of them initiates the connection and the other accepts it

#### Scenario: Neither board is permanently the initiator

- **WHEN** the same two boards are paired repeatedly
- **THEN** which board initiates varies between attempts

#### Scenario: Equal tokens are resolved

- **WHEN** the two boards generate the same token
- **THEN** both generate new tokens and pairing continues without a link being formed from the tied attempt

### Requirement: A completed link blinks the LED five times on both boards

When a link is established and the two boards have completed their handshake, each board SHALL blink its LED five times, and SHALL do so on both boards for the same link. The five-blink pattern SHALL be distinguishable from the single blink of an accepted move and from the three rapid blinks of a rejected move. The peer's identity SHALL also be reported on standard output.

#### Scenario: Both boards confirm the link

- **WHEN** two boards complete pairing
- **THEN** each board blinks its LED five times

#### Scenario: The pattern is distinguishable

- **WHEN** the five-blink pattern is shown
- **THEN** it differs in count and duration from the accepted-move and rejected-move patterns

#### Scenario: The peer is named on the console

- **WHEN** a link is established
- **THEN** the board prints a line identifying the peer it linked to

### Requirement: A pairing window that finds no peer expires

Pairing mode SHALL last a bounded time. If no peer is found before the window expires, the device SHALL leave pairing mode, SHALL report the failure on standard output, SHALL signal the failure with the rejected-move blink pattern, and SHALL return to unlinked operation with its board and move number unchanged.

#### Scenario: No peer found

- **WHEN** a board is held in pairing mode with no other board in range until the window expires
- **THEN** the board leaves pairing mode, prints the failure, and blinks the rejection pattern

#### Scenario: The game survives a failed pairing attempt

- **WHEN** pairing fails after several moves have been played
- **THEN** the position, the side to move, and the move number are unchanged and play continues locally

### Requirement: A board links to at most one peer

A device SHALL hold at most one link at a time. While linked, a device SHALL NOT be discoverable for pairing and SHALL NOT accept a second connection.

#### Scenario: A third board cannot join

- **WHEN** two boards are linked and a third board enters pairing mode in range
- **THEN** the third board does not join the existing link and the existing link is unaffected

#### Scenario: A linked board is not discoverable

- **WHEN** a board is linked
- **THEN** it does not advertise itself for pairing

### Requirement: Holding B while linked tears the link down

Holding `B` for three seconds while linked SHALL end the link deliberately. The board SHALL disconnect, SHALL report it on standard output, SHALL NOT attempt to reconnect, and SHALL return to unlinked operation in which moves for either colour may be entered. The board's position, side to move, and move number SHALL be unchanged by the unlink. The peer SHALL observe the disconnection and SHALL treat it as a lost link.

#### Scenario: Deliberate unlink

- **WHEN** the player holds `B` for three seconds on a linked board
- **THEN** the link ends, the board reports it, and no reconnection is attempted

#### Scenario: Unlink restores single-board play

- **WHEN** a board has been unlinked
- **THEN** a move for either colour is accepted on that board as it was before any link existed

#### Scenario: Unlink preserves the position

- **WHEN** a linked game is unlinked mid-game
- **THEN** the board still holds the position reached, with the same side to move and move number

### Requirement: A link that drops on its own is reported and retried

If the link is lost without a deliberate unlink — the peer loses power, moves out of range, or the connection times out — the device SHALL report the loss on standard output, SHALL signal it with the rejected-move blink pattern, and SHALL attempt to re-establish the link with the same peer for a bounded period without requiring another three-second hold. If reconnection succeeds, the boards SHALL reconcile their positions before play resumes. If reconnection does not succeed within that period, the device SHALL report that it gave up and SHALL remain in the link-lost state until the player unlinks or pairs again.

#### Scenario: Peer goes out of range

- **WHEN** the peer board is carried out of range
- **THEN** the board reports the loss, blinks the rejection pattern, and begins trying to reconnect

#### Scenario: Peer returns

- **WHEN** the peer comes back into range within the retry period
- **THEN** the link is re-established without either player holding `B`

#### Scenario: Reconnection is abandoned

- **WHEN** the peer does not return within the retry period
- **THEN** the board reports that it stopped trying and stays in the link-lost state

#### Scenario: Escaping the link-lost state

- **WHEN** the player holds `B` for three seconds on a board in the link-lost state
- **THEN** the board returns to unlinked operation

