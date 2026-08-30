## Context

`ChessBoard` today is an 8×8 array of FEN letters and nothing else. It has no
notion of whose turn it is, no history, and no rules: `formatMove` returns false
only when the from-square is empty, and `applyMove` copies a character from one
square to another, with two hard-coded special cases (a king moving two files
drags a rook; a White pawn reaching rank 8 becomes a queen). The board is
unaware that either special case might be illegal.

Three constraints shape this design:

1. **The chess core must stay free of ESP-IDF and FreeRTOS.** `chess_board.cpp`
   and `chess_game.cpp` compile into a host test target under `tests/`, which is
   the only place this logic can realistically be tested — a chess rule engine
   verified by pressing keys on a bench is a rule engine that ships with bugs.
   Everything added here must build and run on the desktop.
2. **The game task has 2048 bytes of stack** and shares a 240 MHz core with the
   Wi-Fi driver and an HTTP server. Legality checking must not recurse, must not
   allocate, and must not put a search tree on the stack.
3. **The player is blindfolded.** They cannot read the console and cannot see
   the board. The LED is the entire user interface for "did that work", so its
   two patterns have to be unmistakable to someone who is only listening for a
   click or watching peripherally.

The device is also not a chess engine and never searches. It answers one
question per keypress — "is this specific move legal in this specific
position?" — plus, once per accepted move, "does the opponent have any legal
reply?". Both are shallow, which is what makes the simple approaches below
affordable.

## Goals / Non-Goals

**Goals:**

- Reject every illegal move and accept every legal one, including the cases that
  usually get skipped: absolute pins, castling through an attacked square,
  en passant expiry, and the en passant capture that exposes its own king along
  a rank.
- Put each piece's movement rules in one place, owned by that piece's class, so
  a rule can be read, tested, and corrected without touching the other five.
- Handle pins, moving into check, and answering check with no code that mentions
  pins, king safety, or check — they should fall out of a single mechanism.
- Let the player choose a promotion piece rather than assuming a queen, without
  adding a mode the player has to remember they are in.
- Make "legal" and "illegal" distinguishable through the LED alone.
- Keep the whole rule set host-testable, and cover the edge cases with tests
  that state the position they are testing.
- Leave the keypad scanner, the serial plumbing, and the Wi-Fi preview
  untouched.

**Non-Goals:**

- Draw detection by threefold repetition, the fifty-move rule, or insufficient
  material. These need history or material analysis, neither of which exists,
  and a blindfold player claiming one of them is a matter for the arbiter.
- Evaluating, suggesting, or ranking moves. The device answers legal/illegal and
  nothing more; anything else is engine assistance at the board.
- Undo, takeback, or editing an entered move. Reset remains the only escape.
- Resignation, draw offers, clocks, or any other game administration.
- Move history, PGN export, or a move list on the preview page.
- Speed. A move is entered every few seconds at best; correctness and
  readability win every trade against microseconds here.
- Changing how the four digits are entered, or how castling is entered — the
  king still moves two files.

## Decisions

### King safety by copy-apply-test, not make/unmake

A pseudo-legal move is promoted to legal by copying the whole position, applying
the move to the copy, and asking whether the mover's king is attacked on the
copy. If it is, the move is illegal.

This one mechanism covers absolute pins, partial pins along a ray, moving the
king into check, failing to answer an existing check, capturing the checking
piece, blocking a check, and the en passant capture that removes two pawns from
one rank and exposes the king to a rook. None of those appear in the code as a
named case. That is the entire reason for the choice: every one of them is a
place where a hand-written special case would be subtly wrong, and the
pin-detection code that projects rays out from the king is the classic source of
"my engine allows this one weird move" bugs.

*Why copy rather than make/unmake:* a `ChessBoard` is 64 squares plus side to
move, four castling flags, and an en passant file — about 72 bytes. Copying it
is a `memcpy` the compiler will inline. Make/unmake needs an undo record that
restores the captured piece, the castling rights, the en passant square, and the
rook in a castle, and a single missing field corrupts the position
*permanently* rather than failing loudly. Make/unmake exists because engines
copy millions of times per second; this device copies perhaps forty times per
keypress. Buying a class of state-corruption bugs to save microseconds nobody
will experience is a bad trade.

*Why not incremental pin detection:* it is roughly the same amount of code, it
is far harder to convince yourself is correct, and it has to be written twice —
once for pins and once for en passant's horizontal case.

### Attacks are asked of the piece, not radiated from the square

`ChessBoard::isAttacked(Square target, Color by)` walks all 64 squares and, for
each enemy piece, asks that piece's class `attacks(board, from, target)`.

The fast idiom is the reverse: stand on the target square, cast knight offsets
and eight rays outward, and see what you hit. It is about eight times cheaper.
It also means every movement rule exists twice — once in the piece class and
once in the ray-caster — and the two drift. A bishop whose diagonals are right
in one place and wrong in the other produces a position where a piece can move
somewhere it does not attack, which is the kind of bug that survives a hundred
test games and then loses one.

Asking the piece keeps each rule in exactly one file, which is the point of the
per-piece class structure. The cost is around 640 square-checks per
`isAttacked` call and roughly 25,000 per full "does this side have any legal
move" scan: a fraction of a millisecond on this hardware, against a human
pressing four keys. If profiling ever contradicts that, the ray-cast version is
a drop-in replacement behind the same method signature.

### Pieces are stateless singletons behind a `Piece` interface

```cpp
class Piece {
public:
    virtual char letter() const = 0;                 // uppercase kind letter
    virtual const char* name() const = 0;            // "bishop", for messages
    virtual void appendMoves(const ChessBoard&, Square from, MoveList&) const = 0;
    virtual bool attacks(const ChessBoard&, Square from, Square target) const = 0;

    static const Piece* forSquare(char piece);       // 'n'/'N' -> &KNIGHT
protected:
    static void appendSlides(const ChessBoard&, Square, const Offset*, int, MoveList&);
    static bool slideAttacks(const ChessBoard&, Square, Square, const Offset*, int);
    static void appendSteps(const ChessBoard&, Square, const Offset*, int, MoveList&);
    static bool stepAttacks(Square, Square, const Offset*, int);
};
```

Six subclasses live one per file in `src/game_logic/pieces/`: `Pawn`, `Knight`,
`Bishop`, `Rook`, `Queen`, `King`. Each is a stateless `const` object with a
single file-scope instance, and `Piece::forSquare` maps a board character to
one, ignoring case since colour is carried by the character, not the class.

*Why an interface and not a switch on the letter:* the request is a class per
figure, and it earns its keep — a virtual call per piece keeps `chess_board.cpp`
from growing a 200-line switch, and it makes "where are the knight's rules?"
answerable by the filename.

*Why stateless singletons:* there is no per-piece state to hold — a bishop on c1
and a bishop on f8 differ only by the arguments passed in. Instances mean
allocation or an array of objects to keep in sync with the board; singletons
mean the vtable pointer is the only overhead and everything lives in flash.

*Why `appendMoves` and `attacks` are both virtual:* `attacks` is derivable from
`appendMoves` for four of the six pieces, but not for the two that matter. A
pawn moves straight and attacks diagonally, and it attacks a diagonal square
even when that square is empty and the move is therefore unavailable. A king's
move list includes castling destinations two files away, which it does not
attack. Deriving `attacks` from the move list would get both wrong, and both
are load-bearing for check detection. Sliders and the knight implement the two
methods over the same direction table via the shared helpers, so the rule is
still written once.

*Why `name()`:* rejection messages say "bishop cannot reach d5", and the piece
already knows what it is called.

### Rules, notation, and position state are three modules

| File | Holds | ESP-IDF? |
| --- | --- | --- |
| `move.hpp` | `Square`, `Color`, `MoveKind`, `Move`, `MoveList` | No |
| `pieces/piece.*` | The interface, the registry, the ray and step helpers | No |
| `pieces/{pawn,knight,bishop,rook,queen,king}.*` | One movement rule each | No |
| `chess_board.*` | Position state and `apply`; `pieceAt`, `isAttacked`, `kingSquare` | No |
| `chess_rules.*` | Validation, legal-move filtering, position status | No |
| `chess_notation.*` | SAN rendering, disambiguation, suffixes, rejection text | No |
| `chess_game.*` | Entry state machine, move numbering, game-over lock | No |
| `game.cpp` | Chooses the LED pattern from the event | Yes |
| `led.*` | Blink patterns | Yes |

`ChessBoard` keeps the position and knows how to change it, but does not decide
whether a change is allowed. `ChessRules` decides. `ChessNotation` describes.
The split matters because validation needs the position *before* the move and
notation needs both before (for disambiguation, which is about the alternatives
that existed) and after (for `+` and `#`, which are about the result).

*Why not keep it all in `ChessBoard`:* it would triple in size and mix three
unrelated reasons to change. The current `formatMove` already conflates
description with validation, which is why it returns a bool that means "the
square was not empty".

Header cycles are avoided by `move.hpp` owning the shared value types and
`piece.hpp` forward-declaring `class ChessBoard`.

### Position state gains four fields, and castling rights are derived from squares touched

`ChessBoard` adds `Color side_to_move_`, four castling-right flags, and an
en passant file (with a sentinel for "none"). Side to move alternates on every
applied move and resets to White.

Castling rights are cleared by a single rule applied on every move: any right
whose king square or rook square is the move's from-square or to-square is
cleared. Moving the king clears both of its rights; moving the h1 rook clears
White's kingside right; *capturing* the h1 rook also clears it, because h1 is
the to-square. The awkward case — a rook that moves away and returns — is
handled because the right was cleared when it left and nothing ever restores it.

*Why derive rather than track events:* the "rook captured on its home square"
case is the one everybody forgets, and it costs nothing here.

The en passant file is set only by a two-square pawn advance and cleared by
every other move, including the opponent's. That expiry is the whole rule, so it
belongs in `apply` rather than anywhere a caller could forget it.

### The promotion piece does not affect legality, so the letter is asked for after the move is validated

When four digits describe a pawn move to the last rank, the move is validated
immediately using a queen as a placeholder. If it is illegal, it is rejected on
the fourth digit like any other move, with the LED's triple blink — the player
is not made to pick a promotion piece for a move that was never going to happen.
If it is legal, entry enters a pending state and waits for `A`, `B`, `C`, or `D`.

This is sound because the promoted piece's identity cannot change whether the
move is legal. Legality after the move depends only on whether the mover's king
is attacked, and the promoted piece is the mover's own: it occupies the same
square and blocks the same lines whichever piece it becomes. It can only ever
attack the *enemy* king, which does not affect the mover's legality.

While pending, digits and every other key are ignored and the partial state is
kept. Reset cancels it, as it cancels everything. The console prints a prompt
line naming the four choices, which the player cannot see but the arbiter can,
and the LED stays dark — a blink means a completed move, and nothing has been
completed yet.

*Why not auto-queen with an override:* under-promotion would then depend on
pressing a key *before* the move rather than after, which is backwards from how
a player thinks ("I promote... to a knight") and impossible to correct once the
fourth digit lands.

*Why the LED stays dark rather than signalling "waiting":* a third pattern is a
third thing to distinguish by feel. The player who just entered a promotion
knows a letter is expected, and if they forget, no blink is itself the signal
that the device is still waiting.

### One legal blink, three fast blinks for illegal

`Led` gains a pattern to its command queue: `Single` is the existing 250 ms
on / 250 ms off, `Error` is three cycles of 80 ms on / 80 ms off.

The two are distinguishable by duration alone — one slow pulse against a
stutter — which is what matters for a player who is not looking directly at it.
Three fast blinks total 480 ms, close enough to the single blink's 500 ms that
neither delays the LED task noticeably, and the queue depth of 8 already absorbs
faster entry than a human can produce.

*Why not colour or a buzzer:* there is one LED on GPIO 2 and no sound hardware.

### Rejections name a reason

Each rejection carries a `MoveError`, rendered onto the console line:

| Error | Message |
| --- | --- |
| `EmptyFromSquare` | `empty from-square` |
| `NotYourPiece` | `it is White's turn` / `it is Black's turn` |
| `SameSquare` | `from and to are the same square` |
| `FriendlyPiece` | `own piece on d5` |
| `Unreachable` | `bishop cannot reach d5` |
| `PathBlocked` | `path to d5 is blocked` |
| `KingLeftInCheck` | `king would be left in check` |
| `CastlingRightsLost` | `castling rights lost` |
| `CastlingPathBlocked` | `castling path is blocked` |
| `CastlingThroughCheck` | `cannot castle out of, through, or into check` |
| `EnPassantUnavailable` | `en passant no longer available` |
| `GameOver` | `game is over, press reset` |

The reason is the difference between a device that says no and a device that can
be argued with. At the bench, `illegal: king would be left in check` on a move
the player was sure of is either a bug report or a lesson, and neither is
available from three blinks.

This replaces the current `invalid: empty from-square` wording; every rejection
now reads `<coords> = illegal: <reason>`, so there is one form to recognise
rather than two.

### Notation is generated against both positions

Disambiguation asks which *legal* moves existed before: if another piece of the
same kind and colour could also have moved to the destination, the file letter
is added, or the rank digit if the file does not distinguish them, or both if
neither does. Legality matters here — a twin knight that is pinned cannot move
there, so no disambiguation is needed, and adding it anyway would be wrong.

`+` and `#` are decided on the position after the move: the opponent's king
attacked and the opponent has a reply gives `+`; attacked with no reply gives
`#`; not attacked with no reply is stalemate and takes no suffix.

The `algebraic` buffer grows from 8 to 12 bytes. The worst case is seven
characters plus a terminator (`Qa1xb2#`, `axb8=Q+`), which fits in 8 exactly —
too exactly to leave alone in a `snprintf` target.

### Game over is a latch on the entry state machine

When an accepted move leaves the opponent with no legal reply, `ChessGame`
records checkmate or stalemate, prints a result line after the move line, and
refuses every subsequent move with `GameOver` — triple blink, no board change —
until reset. The move that delivers mate is legal and gets its single blink.

*Why latch rather than simply generating no legal moves:* without the latch the
player gets three blinks with the reason "king would be left in check" forever
and no indication the game ended, which is indistinguishable from the device
having gone wrong.

### The game task's stack goes to 4096 bytes

Legality checking adds a `ChessBoard` copy (~72 bytes) and a `MoveList` (64
moves at 6 bytes, 384 bytes) to the game task's frame, plus the existing 64-byte
snapshot buffer and the widened message buffer. Nothing recurses and nothing
nests more than two calls deep, so the peak is under a kilobyte — but 2048 bytes
was never sized with a rule engine in mind, and a stack overflow on this task is
a reboot mid-game.

`MoveList` is capped at 64: the most any single piece can generate is a queen's
27, and no call site holds more than one piece's moves at a time.

## Risks / Trade-offs

- **The LED becomes a position oracle** → A blindfold player can now probe: enter
  a speculative move, and one blink versus three tells them whether a square is
  occupied, defended, or whether their king is pinned — information they are
  supposed to be holding in their head. This is inherent to the feature and
  cannot be fixed by hiding it, since the same signal is what makes the device
  useful. Mitigation is procedural and now possible: every rejected attempt is
  printed to the console with its reason, so an arbiter reviewing the log sees
  probing as a run of illegal moves rather than the occasional slip. Worth
  stating in the README rather than discovering at a tournament.
- **A rule engine written from scratch will have bugs, and a wrong rejection is
  worse than no checking** → A player who knows their move is legal and gets
  three blinks has no recourse except reset, which loses the game. Mitigation:
  the entire rule set is host-testable and the tests are written against named
  positions, with the edge cases the proposal calls out covered explicitly; the
  copy-apply-test design removes the largest bug class outright. Residual risk
  is real and the README should say the device is not an arbiter of last resort.
- **Turn order becomes mandatory, which breaks setting up a position** → Today
  the device can be driven into any position by entering moves for either side.
  With alternating turns that stops, so an arbiter cannot key in a position to
  check something. Mitigation: none in this change; reset plus replaying the
  game from move 1 is the only route. A position-entry mode is a separate
  change.
- **Promotion can strand the device waiting for a letter** → If the player
  enters a promotion and does not press `A`–`D`, the device ignores all digits
  and appears dead. Mitigation: the console prints the prompt, and reset always
  escapes. The failure is visible to the arbiter even though it is not to the
  player.
- **Castling rights and en passant are invisible state** → Nothing on the
  preview page or the console shows that a right has been lost or that
  en passant is available this move only, so a rejected castle looks arbitrary
  from outside. Mitigation: the rejection reason names it (`castling rights
  lost`), which is the cheapest form of the fix.
- **The preview page does not say whose turn it is** → With turn order now
  enforced, an arbiter looking at the page cannot tell why a move was refused.
  Mitigation: out of scope here; the console line carries the reason. Adding a
  side-to-move indicator to the page is a small follow-up.
- **`isAttacked` scanning 64 squares is roughly eight times slower than
  ray-casting** → At an estimated 25,000 square-checks for a full legal-reply
  scan this is still well under a millisecond, but it is on the path of every
  accepted move and shares a core with the Wi-Fi driver. Mitigation: measure the
  worst case on device once; the ray-cast implementation is a drop-in behind the
  same signature if the measurement surprises.
- **The event type gains values that existing consumers do not handle** →
  `game.cpp` and the tests switch on `GameEventType`, and a missed case is a
  move that silently produces no output or no blink. Mitigation: handle the
  event type exhaustively in one place in `Game::handleKey` and let
  `-Wall -Wextra -Werror`, already on for the host tests, catch unhandled enum
  values.
- **Existing tests assert the behaviour being removed** → `testLegalityIsNotChecked`
  asserts a rook may jump its own pawn, and the promotion tests reach rank 8 via
  moves that are now illegal. Mitigation: they are rewritten as part of this
  change rather than deleted, so the removal is deliberate and visible in review.
- **Every test position must now be reached by legal moves** → Setting up an
  en passant or pin test takes a plausible opening sequence rather than three
  arbitrary moves, which makes the tests longer and their intent less obvious.
  Mitigation: a test helper that plays a sequence of four-digit strings, and a
  comment naming the position each test is building.

## Migration Plan

There is no persisted state, no OTA, and no stored games: the change ships by
flashing, and rollback is flashing the previous image. A game in progress at the
moment of flashing is lost either way, which is already true.

The one behavioural discontinuity worth planning for is that **the device
becomes stricter**, so any habit built on the old permissiveness stops working:
entering both sides' moves to set up a position, moving a pawn to the last rank
and expecting a queen without pressing a letter, and reading a single blink as
"received" rather than "legal". The README's "Chess behavior and limitations"
section currently documents the permissive behaviour and is rewritten in this
change, so the documentation and the firmware move together.

Order of work matters for keeping the tree testable: the piece classes and the
position state land first with the host tests green, then validation, then
notation, then the entry state machine and the LED, then the device build. The
existing tests are updated in the same step that breaks them rather than left
red across several commits.

## Open Questions

- Should the preview page show side to move, and a check indicator? It would
  explain a refusal to an arbiter, but it is also one more thing the player
  could read off a phone.
- Should the device claim draws it can detect cheaply — insufficient material is
  a scan of the board, unlike repetition — or stay silent on all draws for
  consistency?
- Is a position-entry or "free placement" mode needed for arbiters and for
  resuming an adjourned game, now that turn order makes replaying from move 1
  the only way to reach a position?
- Should a rejected move be visible to the player beyond the blink pattern — for
  instance, should the same reason be spoken by blink counts — or is "illegal,
  think again" the correct amount of help for blindfold play?
- Should castling be enterable as its own key sequence rather than a two-file
  king move, now that the two-file move can be refused for six different
  reasons?
