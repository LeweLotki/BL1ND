## Why

The device is a blindfold board for one player, and blindfold chess takes two.
Today the only way to play a game is for one person to key in both sides' moves
on a single board, which means the opponent can hear every entry, the position
lives in exactly one place, and there is no second board for the other player to
hold in their head. Two boards that share a position turn a practice aid into an
actual game between two blindfold players sitting across from each other, each
entering only their own moves.

The reason this is worth building carefully rather than quickly is that the
failure the link can introduce is the same failure the whole device exists to
prevent: two mental boards silently drifting apart. A move that is applied on
one board and lost on the other produces two players confidently playing two
different games, and neither of them can see that it happened. So the link is
not just a transport — it is a continuous agreement that both boards hold the
same position, checked on every single move.

## What Changes

- Holding `B` for three seconds on both boards puts them into pairing mode,
  where they discover each other over Bluetooth Low Energy and form a link
  without any address, PIN, or phone involved.
- On a successful link, both boards blink the LED five times.
- The two boards agree on colours as part of the link handshake: each generates
  a random token, they exchange tokens, and both derive the same assignment from
  the pair. One board is White, the other is Black, and neither side chooses.
- Because a blindfold player cannot read the console, the LED announces the
  assigned colour immediately after the connection blinks: one long blink for
  White, two for Black.
- **BREAKING**: While linked, a board accepts moves only for its own colour. A
  four-digit entry made when it is the opponent's turn is rejected with the
  existing error blink and a printed reason, where today either colour can be
  entered on any board.
- Every accepted move is transmitted to the other board the moment it is applied
  locally. The receiving board validates it against its own position, applies
  it, prints it, and blinks exactly as if it had been entered by hand.
- Every move message carries a 32-bit hash of the sender's position after the
  move. The receiver applies the move, hashes its own result, and answers with
  either an acknowledgement carrying the matching hash or a resynchronisation
  request. This is the handshake that makes silent drift impossible.
- When the hashes disagree, or when a received move is not legal on the
  receiver's board, the board that made the move sends its full position, the
  other adopts it, and both blink the error pattern so the players know
  something was corrected.
- **BREAKING**: Pressing the reset key while linked resets both boards. The
  initiating board resets, tells the other, and both re-roll their tokens so the
  colours are randomly reassigned for the new game and announced on the LED
  again.
- Holding `B` for three seconds while linked tears the link down and returns the
  board to today's single-board behaviour, so there is a way out that is not a
  power cycle.
- **BREAKING**: The `B` key's promotion meaning now fires when the key is
  released rather than when it is pressed, because a press must be allowed to
  become a three-second hold. No other key changes.
- A dropped link is not the same as an unlinked board: the board keeps its
  colour, keeps refusing the opponent's moves, and tries to reconnect. On
  reconnection the two positions are compared and reconciled before play
  resumes.
- The Wi-Fi board preview keeps running while Bluetooth is active, using the
  ESP32's Wi-Fi/Bluetooth coexistence, so an arbiter can still watch either
  board's page during a linked game.
- A `sdkconfig.defaults` file enters the repository so the Bluetooth
  configuration is reproducible rather than living only in the untracked
  generated `sdkconfig`.

## Capabilities

### New Capabilities

- `bluetooth-pairing`: How two boards find each other and form a link — the
  three-second hold that arms pairing, symmetric discovery where both boards
  advertise and scan at once, the token comparison that decides which one
  initiates, the five-blink confirmation, the bounded pairing window, tearing
  the link down deliberately, and what happens when the link drops on its own.
- `linked-game-session`: The gameplay contract over a link — how colours are
  assigned and announced, that a board accepts moves only for its own colour,
  that an accepted move is transmitted immediately and applied on the far side,
  that a reset on either board resets both and reassigns colours, and how a
  board behaves while the link is down.
- `board-state-sync`: The agreement that both boards hold the same position —
  the canonical encoding of a position, the hash carried by every move, the
  acknowledgement that confirms agreement, the detection of disagreement
  including a received move that is illegal locally, the full-position
  resynchronisation that repairs it, and the reconciliation performed when a
  dropped link comes back.

### Modified Capabilities

- `chess-move-entry`: `B` gains a hold gesture and its promotion meaning moves
  from press to release, so "letter keys carry meaning only while a promotion
  choice is awaited" is no longer the whole story. Entry gains a rejection for
  moves made on the wrong board while linked. Reset is no longer purely local
  and no longer always silent on the LED. The requirement that standard output
  carries only move results and rejection messages has to admit link status
  lines.
- `board-preview-access-point`: The access point now has to share the radio with
  Bluetooth rather than owning it, so the requirement that the preview never
  blocks the game has to extend to never blocking the link, and the preview has
  to be specified as remaining available while a link is active.

## Impact

- New `src/link/`: `link_protocol.*` (message encoding, position packing,
  hashing) and `link_session.*` (the pairing, handshake, play, and resync state
  machine) contain no ESP-IDF or FreeRTOS and compile into the host test target,
  the same split that makes the chess core testable. `bluetooth_link.*` is the
  only new file that touches ESP-IDF, wrapping NimBLE and the queues.
- `src/peripherals/numpad.*` and `keypad_layout.hpp`: hold detection with a
  sentinel code for the three-second `B` hold, and deferral of `B`'s press event
  to its release.
- `src/peripherals/led.*`: two new patterns — the five-blink link confirmation
  and the one-or-two long-blink colour announcement.
- `src/game_logic/chess_game.*`: an owned colour that gates entry while linked,
  an entry point for applying a move that arrived from the peer, a
  position-loading path for resynchronisation, and a new rejection reason.
- `src/game_logic/chess_rules.*`: a `NotYourSide` error value and its message.
  `ChessRules::validate` itself stays pure chess — ownership is checked before
  it is called.
- `src/game_logic/game.cpp`: waits on the keypad and the link at once through a
  FreeRTOS queue set instead of blocking on the keypad alone, and dispatches
  link events alongside keys.
- `src/main.cpp`: constructs and starts the link, and the game task's stack is
  re-checked against the added buffers.
- `src/CMakeLists.txt`: the `bt` component, the new sources, and the `link`
  include directory. `tests/CMakeLists.txt`: the two portable link sources.
- `tests/test_chess.cpp`: gains a two-session harness that runs both ends of the
  protocol against each other in one host process, so the link is tested without
  two boards on a bench.
- New `sdkconfig.defaults` enabling NimBLE, BLE-only controller mode, and
  Wi-Fi/Bluetooth coexistence, and the free-heap figure printed at startup
  becomes a number worth watching.
- `README.md`: hardware section unchanged, but pairing, colours, turn ownership,
  the new LED patterns, and linked reset all need documenting.
- **Not addressed**: encrypted or authenticated Bluetooth pairing, links between
  more than two boards, remembering a peer across a reboot, spectating a linked
  game from a phone over Bluetooth, and synchronising move history rather than
  the current position.
