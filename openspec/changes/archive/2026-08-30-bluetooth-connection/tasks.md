## 1. Canonical position encoding and hashing

- [x] 1.1 Create `src/link/link_protocol.hpp/.cpp` with no ESP-IDF or FreeRTOS include, declaring the message types, the fixed message sizes, and the 36-byte canonical position encoding
- [x] 1.2 Implement `encodePosition(const ChessBoard&, unsigned int move_number, uint8_t out[36])` packing the 64 squares as nibbles in a1..h8 order, then side to move and the four castling rights in byte 32, the en passant file or 15 in byte 33, and the move number little-endian in bytes 34–35
- [x] 1.3 Implement `decodePosition(const uint8_t in[36], ChessBoard& board, unsigned int& move_number)` restoring the position through `ChessBoard::loadPosition`, and confirm `loadPosition` can express every field the encoding carries
- [x] 1.4 Implement 32-bit FNV-1a as `hashPosition(const uint8_t encoded[36])` and a convenience `hashBoard(const ChessBoard&, unsigned int move_number)`
- [x] 1.5 Add `link_protocol.cpp` to the `chess_tests` target and the `../src/link` include directory in `tests/CMakeLists.txt`
- [x] 1.6 Test that encoding is a round trip: encode a position, decode it into a fresh board, re-encode, and assert the two 36-byte buffers are identical
- [x] 1.7 Test that the same position reached by two different move orders encodes identically
- [x] 1.8 Test that changing only a castling right, only the en passant target, only the side to move, or only the move number changes both the encoding and the hash
- [x] 1.9 Pin the starting position's encoding against a fixed byte literal, so a change to the layout fails a test rather than silently agreeing with itself

## 2. Wire messages

- [x] 2.1 Define the message header as a type byte plus a payload length byte, and a `MessageType` enum covering `Hello`, `Move`, `Ack`, `ResyncRequest`, `Sync`, `Reset`, and `ResetAck`
- [x] 2.2 Implement encoders and decoders for each message with the payload layouts from the design: `Hello` (version, token, move count, hash), `Move` (seq, from, to, promotion, hash), `Ack` and `ResyncRequest` (seq, hash), `Sync` (seq, 36-byte position), `Reset` and `ResetAck` (token)
- [x] 2.3 Encode a square as a single byte 0–63 and add helpers converting to and from `Square`
- [x] 2.4 Make every decoder reject a message whose length byte does not match its type, and skip an unknown type using the length byte rather than failing the stream
- [x] 2.5 Define `LINK_PROTOCOL_VERSION` and the maximum message size constant, and assert at compile time that the largest message fits the buffer the transport will use
- [x] 2.6 Test each message type against a fixed byte literal in both directions, so the wire layout is pinned rather than round-tripped through itself
- [x] 2.7 Test that a truncated message, an over-long message, a wrong length byte, and an unknown type are each handled without reading past the buffer

## 3. Session state machine

- [x] 3.1 Create `src/link/link_session.hpp/.cpp` with no ESP-IDF or FreeRTOS include, declaring `LinkState` covering `Unlinked`, `Pairing`, `Handshaking`, `Ready`, `AwaitingAck`, `Resyncing`, `Reconciling`, `LinkLost`, and `Broken`
- [x] 3.2 Declare `LinkEvent` as a tagged event record covering a peer message, a peer discovered during pairing, connected as initiator, connected as acceptor, disconnected, the `B` hold, a local move accepted, a local reset, and a clock tick
- [x] 3.3 Give `LinkSession` an event entry point taking the event and a millisecond timestamp, and an outbound accessor the caller drains, so the session never calls a transport
- [x] 3.4 Implement pairing: generate a token on entry through an injected random source, bound the window, expire it with a failure outcome, and expose the token so the transport can advertise it
- [x] 3.5 Implement the initiator decision as a comparison of the two tokens, with equal tokens re-rolling and staying in `Pairing`
- [x] 3.6 Implement the `Hello` exchange on connect: send version, token, move count, and position hash; refuse the link with a distinct outcome when the peer's version differs
- [x] 3.7 Implement colour derivation as `initiator plays White when ((token_initiator ^ token_acceptor) & 1) == 0`, evaluated identically on both sides, and expose `myColor()`
- [x] 3.8 Implement the move send path: accept a local move, increment the sequence number, emit `Move` with the post-move hash, and enter `AwaitingAck`
- [x] 3.9 Implement the move receive path: validate and apply through the injected portable `ChessGame`, compare the resulting hash, expose the `GameEvent`, and emit either `Ack` or `ResyncRequest`
- [x] 3.10 Treat a received move that the caller could not apply legally as a disagreement and emit `ResyncRequest` without changing the board
- [x] 3.11 Track the expected sequence number, acknowledge a duplicate of the last applied move without applying it again, and treat any other unexpected sequence number as a disagreement
- [x] 3.12 Implement the acknowledgement timeout: retransmit `Move` once after 2 s, then have the authoritative mover escalate to a full `Sync` on a second timeout
- [x] 3.13 Implement resynchronisation: the mover answers `ResyncRequest` with `Sync`, the receiver adopts the position through the caller, and both confirm agreeing hashes before returning to `Ready`
- [x] 3.14 Count resynchronisation rounds and enter `Broken` after three failures, refusing local moves until reset or unlink
- [x] 3.15 Implement the linked reset: `Reset` with a fresh token, `ResetAck` with the peer's fresh token, both boards reset and re-derive colour by the same rule
- [x] 3.16 Implement disconnection handling: a deliberate `B` hold goes to `Unlinked` with no retry, an unexpected disconnection goes to `LinkLost` with a bounded reconnect period
- [x] 3.17 Implement reconnection reconciliation from the `Hello` move counts and hashes: equal hashes resume immediately, unequal defers to the higher move count, and a tie defers to the initiator
- [x] 3.18 Refuse local moves while in `Pairing`, `Handshaking`, `Resyncing`, `Reconciling`, `LinkLost`, and `Broken`, each with a distinguishable outcome the caller can turn into a message
- [x] 3.19 Add `link_session.cpp` to the `chess_tests` target

## 4. Two-session host test harness

- [x] 4.1 Add a harness to `tests/test_chess.cpp` holding two `LinkSession` instances, two `ChessGame` instances, a virtual clock, and a deliverable message queue in each direction
- [x] 4.2 Give the harness a deterministic random source so token sequences and therefore initiator and colour outcomes are reproducible in tests
- [x] 4.3 Give the harness controls to drop the next message in one direction, duplicate it, delay it, and disconnect and reconnect the pair
- [x] 4.4 Test pairing end to end: both sessions enter pairing, discover each other, one initiates, both reach `Ready`, and the two colours are opposite and agree
- [x] 4.5 Test that equal tokens re-roll and still reach `Ready` within the window
- [x] 4.6 Test that a pairing window with no peer expires with the failure outcome and leaves both boards unchanged
- [x] 4.7 Test a normal move: the mover applies and sends, the receiver applies and acknowledges, and both boards encode identically afterwards
- [x] 4.8 Test a whole short game played alternately through the harness, asserting the two boards agree after every move and that both move numbers match
- [x] 4.9 Test a dropped `Ack`: the mover retransmits, the receiver acknowledges the duplicate without applying it twice, and the boards still agree
- [x] 4.10 Test a corrupted receiver: mutate one board directly, deliver a move, and assert the mismatch is detected, `Sync` is sent, the receiver adopts, and both agree
- [x] 4.11 Test a received move that is illegal on the receiver's board: assert it is not applied, `ResyncRequest` is sent, and resync repairs it
- [x] 4.12 Test that a resynchronising board discards a partial four-digit entry and an awaited promotion
- [x] 4.13 Test three failed resynchronisation rounds putting the sender into `Broken` and disconnecting the peer into `LinkLost`, so both refuse local moves
- [x] 4.14 Test a disconnection between apply and delivery: reconnect, reconcile from move counts, and assert the move is recovered
- [x] 4.15 Test a reconnection where both boards have equal move counts and disagree, asserting the initiator's position wins and the substitution is reported
- [x] 4.16 Test a reconnection where nothing was missed, asserting no `Sync` is exchanged
- [x] 4.17 Test a linked reset from each side in turn: both boards return to the starting position with move number 1, and colours are re-derived
- [x] 4.18 Test that repeated linked resets do not always give the same board White
- [x] 4.19 Test that a promotion is transmitted once with the chosen piece, and that the receiver ends with that piece on the promotion square
- [x] 4.20 Test that a mate delivered over the link is detected on the receiving board and that further moves are refused there until reset
- [x] 4.21 Run the host suite and confirm it passes with `-Wall -Wextra -Wpedantic -Werror`

## 5. Turn ownership in the chess layer

- [x] 5.1 Add `NotYourSide` and `NotLinked` to `MoveError` in `chess_rules.hpp` and their messages `it is your opponent's turn` and `link is down` to `ChessRules::describe`
- [x] 5.2 Add an owned colour to `ChessGame` with `setOwnedColor(Color)`, `clearOwnedColor()`, and an unlinked default, and confirm `ChessRules::validate` is unchanged
- [x] 5.3 Check ownership in `ChessGame::processMove` before calling `validate`, so a wrong-turn entry is rejected for the turn rather than for a chess rule
- [x] 5.4 Add `ChessGame::applyRemoteMove(Square from, Square to, char promotion)` running the same validation and application path, bypassing the ownership check and the digit entry state, and returning the same `GameEvent` shape
- [x] 5.5 Add a way for `ChessGame` to adopt a decoded position and reset its entry state, for the resynchronisation path
- [x] 5.6 Expose the move number so the protocol can encode it, and confirm it is incremented in exactly one place
- [x] 5.7 Test that an owned colour rejects the opponent's turn with `NotYourSide`, leaves the board and move number unchanged, and accepts its own turn
- [x] 5.8 Test that clearing the owned colour restores today's behaviour of accepting either colour
- [x] 5.9 Test that `applyRemoteMove` rejects an illegal move without changing the board and accepts a legal one with the same event as a local move
- [x] 5.10 Assert the message text of both new errors, alongside the existing ones

## 6. Keypad hold detection

- [x] 6.1 Add a hold sentinel to `keypad_layout.hpp` that no physical key can produce and that differs from `NO_KEY`
- [x] 6.2 In `NumPad::run`, count consecutive scans of the same held key and emit the hold sentinel once when `B` reaches 150 scans, which is three seconds at the existing 20 ms period
- [x] 6.3 Suppress the `B` keypress for a hold that completed, and emit `'B'` on release only when the key was released before the threshold
- [x] 6.4 Confirm every other key still reports on the press edge with the existing behaviour and produces no hold event
- [x] 6.5 Confirm a release and a fresh press restart the hold count, so two two-second holds do not add up
- [x] 6.6 Confirm the hold event is emitted exactly once no matter how long the key stays down
- [x] 6.7 Extract the hold and edge decision into a small pure function if it can be done without changing the scan loop's timing, so it can be host-tested; otherwise record why it stays inside the task

## 7. LED patterns

- [x] 7.1 Add `Linked`, `ColorWhite`, and `ColorBlack` to `Led::BlinkPattern` alongside `Single` and `Error`
- [x] 7.2 Implement the timings in `Led::blink`: five cycles of 120 ms for `Linked`, one cycle of 700 ms on and 400 ms off for `ColorWhite`, two for `ColorBlack`
- [x] 7.3 Add `blinkLinked()` and `announceColor(Color)` to the public API, following the existing enqueue-and-return shape
- [x] 7.4 Confirm the command queue depth still absorbs a link confirmation followed immediately by a colour announcement without blocking the game task
- [x] 7.5 Confirm no LED call is ever made from anywhere but the game task, so the `portMAX_DELAY` send can never run on a NimBLE callback

## 8. NimBLE transport

- [x] 8.1 Create `sdkconfig.defaults` enabling `CONFIG_BT_ENABLED`, `CONFIG_BT_NIMBLE_ENABLED`, `CONFIG_BTDM_CTRL_MODE_BLE_ONLY`, Bluedroid off, and a preferred ATT MTU of at least 128, and confirm `CONFIG_ESP_COEX_ENABLED` stays on
- [x] 8.2 Add `bt` and `esp_hw_support` to `REQUIRES` in `src/CMakeLists.txt`, add the three link sources to `SRCS`, and add `link` to `INCLUDE_DIRS`
- [x] 8.3 Do a clean `idf.py fullclean && idf.py build` to confirm the defaults file alone produces an image with NimBLE in it
- [x] 8.4 Create `src/link/bluetooth_link.hpp/.cpp` owning the NimBLE host initialisation, the inbound queue the game task drains, and a `LinkSession`
- [x] 8.5 Define the 128-bit service UUID and the 128-bit characteristic UUID, and register a GATT service with a single characteristic supporting write-with-response and notify
- [x] 8.6 Implement pairing-mode advertising: connectable, carrying the service UUID and the session's token in service data
- [x] 8.7 Implement pairing-mode scanning for that service UUID, extracting the peer token and feeding a peer-discovered event to the session
- [x] 8.8 Implement the initiator path: stop advertising, connect, discover the service and characteristic, subscribe to notifications, and request an MTU of 128
- [x] 8.9 Implement the acceptor path: stop scanning, keep advertising until connected, then serve the characteristic
- [x] 8.10 Drop the link with a printed reason if the negotiated MTU is below 64
- [x] 8.11 Send by writing the characteristic as initiator and by notifying as acceptor, from the game task, draining whatever the session produced
- [x] 8.12 Make every GAP and GATT callback do nothing but `xQueueSend` with a zero timeout onto the inbound queue, with no logging, no LED call, and no chess call
- [x] 8.13 Feed connect, disconnect, and inbound message events to the session, and stop advertising and scanning while linked
- [x] 8.14 Set the connection parameters to a 30–50 ms interval and a 4 s supervision timeout, and implement the bounded reconnect attempt after an unexpected disconnection
- [x] 8.15 Seed the session's random source from `esp_random`, and confirm it is only called after the radio is up

## 9. Wiring the game task

- [x] 9.1 Expose the keypad queue handle from `NumPad` and the inbound queue handle from `BluetoothLink`, and stop calling `NumPad::receiveKey` from `Game`
- [x] 9.2 In `Game::run`, create a queue set sized from the two queue length constants, add both queues, and select with a timeout that doubles as the protocol tick
- [x] 9.3 Dispatch a key from the set exactly as today, and dispatch the hold sentinel to the link rather than to `ChessGame`
- [x] 9.4 Dispatch an inbound link message to the session on the game task, then handle its outcomes for remote moves, adopted positions, resets, colour ownership, and refusals
- [x] 9.5 On an accepted local move, tell the session so it can transmit, and confirm a rejected move and a pending promotion tell it nothing
- [x] 9.6 On a local reset, tell the session so it can propagate, and confirm an unlinked reset transmits nothing
- [x] 9.7 Handle every link outcome exhaustively in one switch so an unhandled case is a compiler error: link established, colour assigned, link lost, pairing failed, mismatch repaired, reconciled, broken, unlinked
- [x] 9.8 Drive the LED from that switch — five blinks on link, the colour announcement after it, the error pattern for pairing failure, link loss, and a repaired mismatch
- [x] 9.9 Print a line for every link event, and confirm nothing is printed for a `B` press that has not yet become a hold
- [x] 9.10 Publish the board snapshot after an applied remote move and after adopting a resynchronised position, and confirm nothing is published for a refused move
- [x] 9.11 Confirm `ChessGame` is still touched by exactly one task and that no mutex was needed
- [x] 9.12 Confirm `CONFIG_FREERTOS_USE_QUEUE_SETS` is enabled in the build, and add it to `sdkconfig.defaults` if it is not on by default

## 10. Startup

- [x] 10.1 Construct `BluetoothLink` as part of the static `Game` wired in `main.cpp`, keeping the Wi-Fi access point start ahead of it
- [x] 10.2 Start the NimBLE host after the access point so coexistence is initialised with Wi-Fi already up, and report a failure to start without halting the device
- [x] 10.3 Confirm a failed access point start does not prevent the link from starting, and a failed link start does not prevent the game from running
- [x] 10.4 Record the free heap after startup before and after this change, and note the difference
- [x] 10.5 Re-check the game task's stack against the added 36-byte buffers and the queue set, raising it from 4096 only if the high-water mark says so

## 11. Documentation

- [x] 11.1 Add a "Playing against another board" section to the README covering the three-second `B` hold on both boards, the five blinks, and the colour announcement
- [x] 11.2 Document the colour announcement patterns and the fact that colour is assigned randomly and reassigned on every reset
- [x] 11.3 Document that a linked board refuses moves for the other colour, and what the refusal looks like
- [x] 11.4 Document that reset resets both boards, and that holding `B` again unlinks
- [x] 11.5 Document the `B`-on-release timing change and why it exists
- [x] 11.6 Document the position-mismatch behaviour: what the error blinks mean, what the console prints, and that the mover's position wins
- [x] 11.7 Add a note that the link is unencrypted and unauthenticated, that both boards must run the same firmware version, and that two pairs of boards pairing simultaneously in one room can cross-pair
- [x] 11.8 Update the feature list, the project structure listing with `src/link/`, and the LED pattern table
- [x] 11.9 Note that `sdkconfig.defaults` now carries the radio configuration and that a clean checkout builds with Bluetooth enabled

## 12. On-device verification

- [x] 12.1 Flash both boards, hold `B` on each, and confirm five blinks on both and a peer line on both consoles
- [x] 12.2 Confirm the colour announcement follows the five blinks on both boards and that the two colours are opposite
- [x] 12.3 Pair and reset repeatedly and confirm the colour assignment varies rather than always favouring one board
- [x] 12.4 Play a full game alternating boards, confirming each move appears on both consoles with the same notation and move number, and both preview pages agree
- [x] 12.5 Enter a move on the wrong turn and confirm the rejection blinks, the reason on the console, and that nothing reaches the peer
- [x] 12.6 Promote a pawn over the link with each of the four letter keys across separate games and confirm the piece on both boards
- [x] 12.7 Confirm holding `B` during a pending promotion does not place a rook
- [x] 12.8 Press reset on each board in turn and confirm both return to the starting position and both announce a new colour
- [x] 12.9 Carry one board out of range mid-game, confirm both report the loss and blink the error pattern, then bring it back and confirm the link returns without another hold and the positions reconcile
- [x] 12.10 Power-cycle one board mid-game and confirm the other reports the loss and eventually gives up rather than hanging
- [x] 12.11 Make a move on one board with the other powered off, then reconnect, and confirm the missed move is recovered rather than lost
- [x] 12.12 Hold `B` on a linked board and confirm it unlinks, the peer notices, and the unlinked board accepts moves for both colours again
- [x] 12.13 Join a phone to each board's access point and confirm both preview pages keep loading throughout pairing and a full linked game
- [x] 12.14 Refresh both preview pages continuously while playing and confirm no move is lost and the link does not drop
- [x] 12.15 Attempt to pair a third board while two are linked and confirm the existing link is unaffected
- [x] 12.16 Confirm a short press of `B` outside a promotion still does nothing visible, and that a promotion rook lands on release without a noticeable delay
- [x] 12.17 Play a long linked game and confirm no reboot, checking the game task and NimBLE host task high-water marks if convenient
- [x] 12.18 Record the free heap on both boards during a linked game with a phone joined, and confirm the margin is comfortable
