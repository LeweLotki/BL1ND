## Context

The firmware today is four FreeRTOS tasks with queues between them. `NumPad`
scans the matrix every 20 ms and posts key characters; `Game::run` blocks on
that queue forever and hands each key to `ChessGame`; `Led` and
`StandardOutput` drain their own queues; `BoardSnapshot` carries the position to
an HTTP handler with `xQueueOverwrite`. Everything below `Game` — the board, the
rules, the pieces, the notation — is free of ESP-IDF and compiles into a host
test target, which is the only reason the rule engine is trustworthy.

The radio is currently owned entirely by a Wi-Fi SoftAP started in `app_main`.
`CONFIG_BT_ENABLED` is not set, so there is no Bluetooth stack in the image at
all, and `sdkconfig` is generated and gitignored, so nothing about the radio
configuration is currently under version control.

Five constraints shape this design:

1. **Both ends of the protocol are the same firmware.** There is no reference
   implementation to test against and no third party who will notice a mistake.
   A misunderstanding of the protocol is symmetric: both boards make it, both
   agree, and nothing looks wrong until the positions differ. This is the single
   most important fact about the change and it drives the decision to make the
   link a portable state machine that can be run twice inside one host process.
2. **The failure mode is silent.** A dropped move does not produce an error; it
   produces two players confidently playing different games. Everything in the
   protocol that looks like belt-and-braces — the hash on every move, the
   acknowledgement, the sequence number — exists because the alternative to
   catching drift immediately is not catching it at all.
3. **The player is blindfolded and the console is for the arbiter.** The LED is
   the whole interface. Any state the player must know — that a link formed,
   which colour they are, that something was corrected — has to be expressible
   in blinks, and every new pattern is another thing to distinguish by feel.
4. **One 2.4 GHz radio, shared.** Wi-Fi SoftAP and BLE will run at the same
   time through ESP-IDF's software coexistence, and both cost heap on a chip
   that has already spent some on an HTTP server.
5. **NimBLE callbacks run on the NimBLE host task and must not block.** The
   existing `Led::blinkOnce` sends with `portMAX_DELAY`. Calling it from a GAP
   or GATT callback would be a latent deadlock.

## Goals / Non-Goals

**Goals:**

- Two boards discover each other and link with no address, PIN, phone, or app,
  from one gesture performed independently on each board in either order.
- Make it impossible for the two boards to hold different positions for longer
  than one move without both of them knowing and correcting it.
- Keep the pairing, handshake, play, and resynchronisation logic portable, so
  the state machine that is hardest to get right is the part that runs on the
  desktop under the existing test target.
- Assign colours randomly, agree on the assignment with no negotiation round,
  and tell a player who cannot see or read which colour they got.
- Make a linked board refuse to play its opponent's moves, and make an unlinked
  board behave exactly as it does today.
- Leave a way out that is not a power cycle: the same gesture that pairs also
  unlinks.
- Keep the Wi-Fi preview working throughout, on both boards.
- Touch the chess core as little as possible. The rules do not change; only who
  is allowed to invoke them does.

**Non-Goals:**

- Encryption, authentication, or bonding. The link is unencrypted and remembers
  nothing across a reboot. A room with a hostile actor holding this firmware is
  out of scope; a room with another pair of boards is addressed by the pairing
  window, not by cryptography.
- More than two boards, spectator connections over Bluetooth, or a board that
  holds several links.
- Synchronising move history. The boards agree on the current position and the
  move number, not on how they got there. A board that resynchronises adopts a
  position, not a game.
- Clocks, draw offers, resignation, or anything else two players would use a
  real match for.
- Playing against a phone, a computer, or an engine. The peer is a board running
  this firmware and the service identifier says so.
- Recovering a game across a power cycle. Nothing is persisted.
- Throughput. Two moves a minute is a busy game; every latency trade in this
  design is spent on certainty.

## Decisions

### The link is a portable state machine with a thin NimBLE shell

Three new files, and the split between them is the most consequential decision
here:

| File | Holds | ESP-IDF? |
| --- | --- | --- |
| `link/link_protocol.*` | Message encoding and decoding, the canonical position encoding, the hash | No |
| `link/link_session.*` | The state machine: pairing, handshake, colour derivation, move exchange, acknowledgement, mismatch, resync, reconnection | No |
| `link/bluetooth_link.*` | NimBLE: advertising, scanning, GAP and GATT callbacks, connection lifecycle, queues | Yes |

`LinkSession` is driven entirely by events and a millisecond clock passed in by
its caller. It never calls NimBLE, never touches a queue, and never reads a
timer. Its interface is roughly:

```cpp
enum class LinkState { Unlinked, Pairing, Handshaking, Ready, AwaitingAck, Resyncing, Reconciling, LinkLost, Broken };

class LinkSession {
public:
    LinkSession(ChessGame& game, RandomFunction random, void* context);
    void onEvent(const LinkEvent& event, uint32_t now_ms);   // peer message, connect, disconnect, hold, tick
    bool nextOutput(LinkOutput& output); // transport commands and game-task outcomes
    LinkState state() const;
    Color myColor() const;
    bool canAcceptLocalMove() const;
};
```

The implemented session is constructed with a reference to the portable
`ChessGame`, rather than returning a second callback-shaped command for every
board operation. It applies received moves and adopted positions synchronously
inside `onEvent` and reports the resulting `GameEvent` as an output. This does
not change task ownership: `BluetoothLink` invokes the session only from the
game task, while NimBLE callbacks only enqueue transport events.

This is the same split that makes the chess rules trustworthy, applied to the
part of the system that needs it more. `ChessRules` can at least be checked by
hand against a rulebook; a distributed handshake cannot be checked by hand at
all, and checking it on hardware means two boards, two serial cables, and a
reproduction rate of "sometimes". On the host, both ends are objects in one
process, the clock is a variable, and a dropped message is a line of test code.

*Why not put the state machine in the NimBLE callbacks:* because that is where
it naturally wants to go, which is exactly why it must not. Logic reachable only
from a GAP callback is logic that can only be exercised by two radios, and it
runs on a task where blocking is forbidden and stack is scarce.

### BLE with NimBLE, not Bluetooth Classic and not Bluedroid

BLE, because Classic SPP would be the easier programming model — a byte stream
socket, no GATT, no attribute tables — and would cost about twice the RAM,
coexist far worse with an active Wi-Fi SoftAP, and require inquiry-based
discovery that is slower and noisier than an advertisement carrying a service
UUID. The link moves roughly forty bytes a minute. Nothing about this workload
wants a stream socket.

NimBLE rather than Bluedroid, because it is roughly 40 KB lighter on a chip that
is already running Wi-Fi, an HTTP server, and four tasks, and because BLE-only
controller mode (`CONFIG_BTDM_CTRL_MODE_BLE_ONLY`) drops the Classic controller
from the image entirely. Bluedroid's advantage is the volume of example code
written against it, which matters less here than the heap does.

### Symmetric discovery: both boards advertise and scan, a random token breaks the tie

BLE is asymmetric — someone advertises, someone scans and connects — but the
gesture is symmetric. Two players hold `B`; neither is the server. So on
entering pairing mode a board does both at once, which the ESP32 controller
supports: it advertises connectably with a 128-bit service UUID that identifies
this firmware, and it scans for that same UUID.

The advertisement carries the board's 32-bit token in its service data. When a
board sees a peer advertisement it has both tokens, and both boards evaluate one
rule:

```
the board with the numerically larger token connects;
the board with the smaller token stops scanning and keeps advertising
```

Both boards reach the same conclusion from the same two numbers, so no
negotiation round is needed and the outcome cannot be "both connect" or "neither
connects". Equal tokens — one chance in four billion per attempt — are resolved
by both boards re-rolling and continuing within the same window, which is a
three-line case that costs nothing and removes an unreachable-looking branch
from the "can't happen" category.

*Why a random token rather than comparing MAC addresses:* MAC comparison also
works, is free, and needs no advertisement payload. It is rejected because it is
deterministic: the same board would initiate every single time, so the
initiator-side code path and the acceptor-side code path would each only ever be
exercised on one physical board. A bug in the acceptor path would be invisible
on the board you happen to be debugging. Randomising the role means a few
sessions exercise both. The token also has two more jobs, below, which makes it
cheap.

*Why not a fixed "board 1 / board 2" configured at flash time:* it makes the two
images different, which is the beginning of a class of bugs where the boards are
running different firmware and nobody notices.

### One 32-bit token does three jobs

`esp_random()` is a true hardware RNG whenever the RF subsystem is on, which it
always is here because the Wi-Fi AP starts before pairing is possible. One draw
per pairing attempt gives:

1. **The initiator decision**, above.
2. **The colour assignment.** After connecting, the initiator sends `Hello`
   carrying its token and the acceptor answers `Hello` with its own, so both
   boards hold both tokens regardless of whether the acceptor's scan ever saw
   the initiator. Then:

   ```
   initiator plays White  when  ((token_initiator ^ token_acceptor) & 1) == 0
   initiator plays Black  otherwise;  the acceptor always takes the other colour
   ```

   Both boards compute this from the same two numbers and cannot disagree. It is
   random because both inputs are, and it is not merely "the initiator is always
   White", which would make colour a function of the tie-break and therefore
   correlated with it.
3. **The reset re-roll.** A linked reset draws fresh tokens and exchanges them,
   so the same rule reassigns colours for the new game with no extra machinery.

*Why exchange tokens over the connection rather than reading them from the
advertisement:* the acceptor may never have scanned successfully — it may have
been found before it found anything. Deriving colour from advertisement data
would work on the initiator and silently fail on the acceptor. The `Hello`
exchange is one round trip that has to happen anyway to check the protocol
version.

### The GATT profile is one service and one characteristic

A single 128-bit service with a single 128-bit characteristic hosted by the
acceptor, supporting `WRITE` from the initiator and `NOTIFY` to it. The
initiator writes to send; the acceptor notifies to send. That is a symmetric
byte-message pipe with the smallest attribute table that can express one.

Writes are **with response**, and the ATT response is not treated as agreement.
The two facts are different and both are wanted: the ATT response says the bytes
reached the peer's Bluetooth stack, which distinguishes a radio problem from a
logic problem; the application-level `Ack` says the peer's *board* agrees, which
is the thing the players care about. Conflating them would mean a move that
arrived and was rejected as illegal looks identical to a move that never
arrived.

MTU is requested at 128. The largest message is 39 bytes, so anything at or
above 64 is comfortable; if negotiation somehow lands below 64 the link is
dropped with a printed reason rather than silently fragmenting, because
fragmentation is reassembly is state is bugs, for a case that should not occur.

### The wire protocol is a two-byte header and a fixed payload

| Type | Payload | Bytes | Sent by |
| --- | --- | --- | --- |
| `Hello` | version, token, move count, position hash | 11 | Both, after connect and after reconnect |
| `Move` | seq, from square, to square, promotion, post-move hash | 8 | The mover |
| `Ack` | seq, post-move hash | 5 | The receiver, on agreement |
| `ResyncRequest` | seq, the receiver's own hash | 5 | The receiver, on disagreement |
| `Sync` | seq, canonical position | 37 | The authoritative board |
| `Reset` | fresh token | 4 | The board whose reset key was pressed |
| `ResetAck` | fresh token | 4 | The other board |

Header is one type byte and one length byte, so a message of an unknown type can
be skipped rather than desynchronising the parser, and a version mismatch in
`Hello` refuses the link with a printed reason rather than misparsing.

Squares travel as a single byte 0–63 rather than as the four keypad digits,
because the digits are an input convention and the board is the thing being
agreed on. Promotion travels as the piece letter or `\0`.

The sequence number is one wrapping byte, incremented by whichever board made
the move. It exists for one case: a `Move` retransmitted after a reconnection
that the peer already applied. Without it the peer applies the move twice and
the boards diverge in the one situation the protocol is supposed to survive.

### The canonical position is 36 bytes and the hash is FNV-1a

Thirteen piece values — empty plus six of each colour — fit in a nibble, so the
64 squares pack into 32 bytes. Then one byte of side-to-move and four castling
rights, one byte of en passant file (15 meaning none), and two bytes of move
number:

```
bytes 0..31   64 nibbles, low nibble first, a1 h1 a2 ... h8
byte 32       bit 0 side to move, bits 1..4 castling rights KQkq
byte 33       en passant file 0..7, or 15 for none
bytes 34..35  move number, little endian
```

Everything that affects what moves are legal from here is in those 36 bytes, and
nothing that does not is. Castling rights and the en passant target are included
deliberately: they are invisible on the preview page and invisible to the
player, so a divergence in either would show up as a castle that works on one
board and not the other, several moves later, with no way to trace it. The move
number is included so that a numbering drift is caught as a mismatch rather than
producing two logs that disagree.

The hash is 32-bit FNV-1a over those 36 bytes. It is chosen for being four lines
long, needing no table, and being identical on both boards by construction. This
is a guard against bugs and lost messages, not against an adversary; a collision
would need a second position that both is reachable and hashes the same, and at
roughly one move per thirty seconds the exposure is not worth a real hash
function's flash and cycles.

*Why hash at all rather than sending the whole 36-byte position with every
move:* mostly it would work, and 36 bytes is not expensive. It is rejected
because then the boards would never compare anything — the receiver would just
adopt whatever it was told, every move, and a bug that corrupts one board's
position would propagate to the other one silently instead of being caught. The
hash makes disagreement an event.

### The mover is authoritative, and every move is a small two-phase agreement

The sequence for one move:

1. The mover validates and applies locally, prints, blinks, publishes to the
   preview — exactly as today — and then sends `Move` with the post-move hash.
2. The receiver validates the move against its own board. If it is legal, it
   applies it, hashes, and compares.
3. Agreement: `Ack` with the hash. Both boards are done.
4. Disagreement, or an illegal received move: `ResyncRequest`. The mover answers
   with `Sync` carrying its full position; the receiver adopts it wholesale,
   discarding any partial entry, and both blink the error pattern and print the
   two hashes.

The mover is authoritative because it is the only board that has seen the move
judged against a position a human confirmed with four keypresses. The receiver
disagreeing means the receiver's position was already wrong, and adopting the
mover's is the only outcome that does not lose the move.

*Why apply locally before sending rather than after acknowledgement:* the
alternative makes every move wait a round trip before the LED confirms it, so a
link hiccup turns into an unexplained pause for a blindfolded player mid-game,
and a dropped acknowledgement would leave the mover's own board behind its own
input. Applying first means a link failure at exactly the wrong moment leaves
the mover one move ahead — which is precisely the case reconnection
reconciliation is written to handle, and it is handled by comparing move counts
rather than by hoping.

*Why not two-phase commit properly:* to make both boards apply or neither, one
of them has to be able to roll back a move it has already printed and blinked,
and the player has already heard the blink. Certainty about the position is
achievable; certainty about the player's mental state is not.

An unacknowledged `Move` is retransmitted once after 2 s, then the mover
escalates directly to a full `Sync`. A `ResyncRequest` travels in the other
direction — from the board that detected a disagreement to the mover — so using
it here would accidentally make the receiver authoritative. After three failed
resynchronisation rounds the session enters `Broken`: moves are refused with a
printed reason, and only reset or unlink escapes. Continuing to play from a
position known to be possibly wrong is worse than stopping.

### The game task waits on the keypad and the link through a queue set

`Game::run` currently blocks on the keypad queue with `portMAX_DELAY`. It now
has two sources, plus a periodic tick for the protocol's timeouts. A
`QueueSetHandle_t` combining the keypad queue and the link's inbound queue keeps
`NumPad` untouched and keeps everything on the game task:

```cpp
QueueSetHandle_t inputs = xQueueCreateSet(KEY_QUEUE_LEN + LINK_QUEUE_LEN);
xQueueAddToSet(numpad_.queue(), inputs);
xQueueAddToSet(link_.inboundQueue(), inputs);
```

The game task then selects with a timeout, and a timed-out select is the tick
that drives acknowledgement timeouts and the pairing window.

Keeping every side effect on the game task is what makes the "must not block in
a NimBLE callback" constraint a non-issue: the GAP and GATT callbacks do exactly
one thing, `xQueueSend` with a zero timeout onto the inbound queue, and every
call to `Led`, `StandardOutput`, `ChessGame`, and `BoardSnapshot` happens where
they already happen. It also means `ChessGame` needs no locking, because it is
still touched by exactly one task.

*Why not have `NumPad` and the link post into a single shared event queue:* it
is arguably cleaner and it was rejected only because it changes `NumPad`, whose
timing and edge detection are correct and were deliberately left alone by the
previous change. A queue set adds two lines to `Game` and none to the keypad.

*Why not a task per concern with a mutex on the board:* a mutex around
`ChessGame` invites a second task to hold the position mid-update while the HTTP
handler reads a snapshot, and the current single-owner design has no such
window. Nothing here needs concurrency; it needs one more input.

### `B`'s press meaning moves to release

`NumPad` reports a key on the press edge. A three-second hold cannot be
recognised on the press edge, and `B` already means "promote to rook", so the
two meanings have to be separated in time. The rule:

- `B` goes down: nothing is reported.
- `B` comes up before 3 s: report `'B'`, exactly as today.
- `B` stays down 3 s: report the hold sentinel once, and suppress the `'B'` that
  release would otherwise produce.

Only `B` is affected. Every other key still reports on press with the existing
edge detection, and the hold is measured by counting 20 ms scan iterations,
which needs no timer and no new task.

*Why not use a key that is currently unused, such as `0` or `#`:* it would avoid
this entire complication, and it is rejected because the request is `B` and
because the keys that are free today are free by accident rather than by design.
Deferring one key's press to its release costs at most the time the player holds
it, on a key pressed once per promotion.

*Why not treat the hold as "rook, and also pair":* because holding `B` during a
pending promotion would then promote to a rook the player did not intend, and
under-promotion is exactly the situation where the player was being deliberate.
Suppressing the press when the hold completes means the two gestures never both
fire.

*Why the hold works while linked as well as unlinked:* without it there is no
way to end a session short of pulling power, and a board that cannot be
unlinked is a board that cannot be lent to the next pair of players.

### Two new LED patterns, and why the colour needs one

| Pattern | Shape | Meaning |
| --- | --- | --- |
| `Single` | 1 × 250 ms on / 250 ms off | Move accepted (entered or received) |
| `Error` | 3 × 80 ms on / 80 ms off | Move rejected, pairing failed, link lost, position corrected |
| `Linked` | 5 × 120 ms on / 120 ms off | Link established |
| `ColorWhite` | 1 × 700 ms on / 400 ms off | You are White |
| `ColorBlack` | 2 × 700 ms on / 400 ms off | You are Black |

Five patterns is more than the previous change was comfortable with, and the
colour announcement is the one that has to justify itself. It does: the colour
is randomly assigned by the device, the player cannot read the console, and a
blindfold player who does not know their colour cannot play at all — every entry
they make will be refused and they will not know why. This is not a convenience,
it is the difference between the feature working and not.

The patterns stay distinguishable because they differ in count and in unit
duration, and because the two link patterns only ever appear in a context the
player created by holding a key or pressing reset. Nobody has to tell five fast
blinks from one slow one cold; they hear five blinks three seconds after they
let go of `B`.

`Error` is reused for pairing failure, link loss, and a corrected position
rather than getting patterns of their own. All three mean the same thing to the
player — "something went wrong, look at the console or ask the arbiter" — and
the console line distinguishes them for anyone who can act on the difference.

### Turn ownership lives in `ChessGame`, not in `ChessRules`

`ChessRules::validate` stays pure chess: it knows about side to move, it does
not know that a board might only be allowed to play one colour. `ChessGame`
gains an optional owned colour and checks it before calling `validate`:

```cpp
void ChessGame::setOwnedColor(Color color);   // linked
void ChessGame::clearOwnedColor();            // unlinked, and the default
```

The refusal surfaces as a new `MoveError::NotYourSide`, rendered as
`it is your opponent's turn`, alongside a `NotLinked` case rendered as
`link is down`. Both live in the existing enum so that `describe` remains the one
place rejection text is written, and so the host tests can pin the wording the
same way they pin every other reason.

*Why not a separate rejection channel:* every other refusal in the system is a
`MoveError` with a message, the LED pattern is chosen from the event type, and
the console line format is `<coords> = illegal: <reason>`. A parallel mechanism
for two more reasons would fork all three.

*Why the ownership check comes before legality:* a player entering a move on the
wrong turn should be told it is not their turn, not that their bishop cannot
reach that square. The first reason is the actionable one.

`ChessGame` also gains `applyRemoteMove(from, to, promotion)`, which runs the
same validation and application path as a local move but bypasses the ownership
check and the four-digit entry state, and `loadPosition` already exists on
`ChessBoard` for the resynchronisation path.

### Wi-Fi and BLE coexist, and `sdkconfig.defaults` enters the repository

`CONFIG_ESP_COEX_ENABLED` is already `y`; what is missing is the Bluetooth stack
itself. The build needs `CONFIG_BT_ENABLED`, `CONFIG_BT_NIMBLE_ENABLED`,
`CONFIG_BTDM_CTRL_MODE_BLE_ONLY`, and Bluedroid off. Those currently could only
live in the generated, gitignored `sdkconfig`, which means the build is not
reproducible from a clean checkout — a second developer, or the same developer
after `idf.py fullclean`, gets an image with no Bluetooth in it and no
indication why.

So `sdkconfig.defaults` is added and tracked. This is a small change with a
disproportionate effect: from here on, radio configuration is reviewable.

The AP is not stopped for pairing or for a linked game. Coexistence timeslices
the radio, and the traffic profile is favourable — the preview is a page a phone
loads occasionally, and the link is forty bytes a minute — but it is not free,
and it shares core 0 with the Wi-Fi driver. The connection interval is requested
at 30–50 ms with a 4 s supervision timeout, which is slack enough that a burst
of HTTP traffic does not drop the link, and tight enough that a board carried out
of range is noticed within a few seconds.

Heap is the real cost. NimBLE in BLE-only mode is on the order of 50 KB on top of
Wi-Fi and the HTTP server. `app_main` already prints free heap after startup, so
the figure before and after this change is recorded rather than guessed.

### Both ends of the protocol are tested in one host process

Because `LinkSession` and `LinkProtocol` have no ESP-IDF, the host test target
can instantiate two sessions, each with its own `ChessBoard`, and hand each one
whatever the other produced. A virtual clock makes timeouts a variable rather
than a wait.

That harness makes the interesting cases ordinary tests: a move delivered
normally; a move whose `Ack` is dropped; a move delivered to a board whose
position was corrupted on purpose, checking that the mismatch is caught and the
resync repairs it; a move that is illegal on the receiver; a duplicate move after
a simulated reconnection; a disconnection between apply and send, with
reconciliation choosing the board with more moves; both boards resetting; the
tie in the colour derivation; and three failed resyncs putting the session into
`Broken`.

Every one of those is a scenario in the specs, and none of them is reliably
reproducible with two boards on a bench, which is the whole argument.

## Risks / Trade-offs

- **The link is unauthenticated and unencrypted, so anything running this
  firmware can join a pairing window** → Two boards in pairing mode within radio
  range of two other boards in pairing mode can cross-pair, and both pairs would
  see a successful link. Mitigation is procedural and partial: the window is
  bounded, both players must deliberately hold a key, and the console names the
  peer address so an arbiter can check. BLE Secure Connections with a passkey
  displayed in blinks would close it and is deliberately deferred — it is a
  change of its own and it makes the pairing gesture longer. Worth stating in the
  README rather than discovering at a club night.
- **A protocol misunderstanding is symmetric and therefore invisible** → Both
  boards run the same encoder and decoder, so a field written in the wrong order
  is written and read in the wrong order and everything appears to work until a
  board is upgraded. Mitigation: the version byte in `Hello` refuses a link
  between mismatched firmware, and the host harness tests the encoding against
  fixed byte literals rather than against a round trip through itself, so a
  changed layout fails a test instead of quietly agreeing with itself.
- **Applying before sending can leave the mover one move ahead** → If the link
  drops between `apply` and the peer receiving `Move`, the two boards differ by
  exactly one move. Mitigation: reconnection compares move counts and hashes
  before anything else, and the board with more moves is authoritative, so the
  move is recovered rather than lost. The residual case is a drop combined with
  the peer also having moved, which the move-count tie-break resolves in favour
  of the initiator and reports — one move can be lost there, loudly.
- **The mover being authoritative means a corrupted mover corrupts both** →
  Resynchronisation propagates the mover's position without questioning it, so a
  board whose memory was disturbed hands its bad position to a board that was
  fine. Mitigation: none within the protocol, which cannot tell "your position is
  wrong" from "my position is wrong". The mismatch is printed with both hashes,
  which is enough for an arbiter to reset. The alternative — refusing to
  reconcile and stopping the game — is worse for the overwhelmingly more common
  cause, which is a lost message.
- **The LED now carries five meanings for a player who cannot see it well** →
  Two of them are new and one of them, the colour, is load-bearing. Mitigation:
  the two new patterns only appear immediately after an action the player took,
  and the colour announcement is repeated after every linked reset. If it proves
  unreliable in practice, announcing the colour on demand from a key is a small
  follow-up.
- **Wi-Fi and BLE share one radio and about 50 KB of heap** → Coexistence could
  show up as a dropped link during a burst of HTTP traffic, or as an allocation
  failure at startup on a board that was already close to the edge. Mitigation:
  the free-heap figure at startup is recorded before and after, the supervision
  timeout is set with slack, and a spec scenario requires the link to survive
  both boards being watched on phones. If the measurement disappoints, stopping
  the AP while linked is a one-line fallback that the design deliberately leaves
  available.
- **A queue set is easy to misuse** → Reading directly from a member queue
  bypasses the set and desynchronises it, and the set must be at least as large
  as the sum of the member queue lengths. Mitigation: `NumPad::receiveKey` is no
  longer called by `Game` and the set is sized from the two queue length
  constants rather than a literal. Both are one-line mistakes that would produce
  intermittent lost keys, so they are called out in the tasks.
- **`B` no longer acts on press** → A player used to the old timing will notice
  the rook promotion lands on release. The window is small and only one key is
  affected, but a promotion is a high-stakes moment. Mitigation: documented in
  the README, and a spec scenario pins it.
- **Turn ownership makes probing more attractive, not less** → A blindfold
  player can already probe legality with the LED. Now they can also probe
  whether a move is theirs. Mitigation: as before, every refusal is printed with
  its reason and an arbiter reading the log sees a run of refusals.
- **`Broken` is a state a player can reach and not understand** → Three failed
  resyncs stop the game with three fast blinks, which is the same pattern as a
  rejected move. A player could sit there entering moves and getting the error
  pattern with no idea why. Mitigation: the console says so plainly, and both
  escapes — reset and the `B` hold — are gestures the player already knows.
- **The move number rides in the hash, so a numbering bug becomes a link
  failure** → A discrepancy in when the move number increments would present as a
  position mismatch on the first move, which is a confusing symptom for a cause
  that has nothing to do with Bluetooth. Mitigation: the host harness plays whole
  games through two sessions, so a numbering discrepancy fails on the desktop
  rather than on hardware.

## Migration Plan

Nothing is persisted, there is no OTA, and there are no stored games: the change
ships by flashing both boards, and rollback is flashing the previous image. Both
boards must be flashed together — the version byte in `Hello` will refuse a link
between old and new firmware, which is the intended behaviour but is worth
knowing before wondering why pairing fails.

The behavioural discontinuities are all conditional on being linked, which keeps
the risk contained: an unlinked board behaves exactly as it does today, so a
player who never holds `B` sees only one change, the `B`-on-release timing. Once
linked, three habits stop working — entering both colours on one board, expecting
reset to be local, and expecting the LED to only ever mean "move accepted" or
"move rejected".

Order of work matters for keeping the tree testable, and the ordering is chosen
so the hardest part is proven before any radio is involved: the portable protocol
and position encoding first, with byte-literal tests; then the session state
machine with the two-session host harness, which is where most of the risk is
retired; then the NimBLE transport; then the keypad hold, the LED patterns, and
the ownership check; then wiring in `Game` and `main`; then the device build and
the coexistence measurements. The host tests stay green at every step because the
first two stages add code that the firmware does not yet call.

## Open Questions

- Should the colour announcement be repeatable on demand — a short press of a
  currently unused key such as `#` — for a player who missed it or forgot? It is
  three lines and it removes the only way to be stuck not knowing your colour.
- Should the preview page show which colour this board is playing and whether the
  link is up? It would let an arbiter diagnose a refused move at a glance, and it
  is the natural home for the peer address and the last hash.
- Is a 30-second pairing window right? Long enough for two players to find the
  key blindfolded, short enough that a board is not discoverable across a whole
  round.
- Should a linked game refuse to start unless both boards are at the starting
  position, rather than resynchronising whatever they happen to hold? Pairing
  mid-game is currently allowed and resolves by reconciliation, which may be more
  surprise than convenience.
- Should the boards exchange move history rather than positions, so that a
  resynchronised board could still print a complete game log? It would make the
  console output of a corrected board trustworthy as a record, at the cost of
  unbounded state.
- Should `Broken` get its own LED pattern after all, given that it is the one
  error state a player cannot act their way out of by trying again?
- Does the arbiter need a way to see both boards on one page — one board relaying
  the peer's hash and colour to its own preview — or is watching two pages
  acceptable?
