## MODIFIED Requirements

### Requirement: A completed move updates the board

Applying an accepted move SHALL move the piece from the from-square to the to-square and leave the from-square empty. Any piece already on the to-square SHALL be removed from the board. Castling SHALL also move the rook, en passant SHALL also remove the captured pawn from its own square, and promotion SHALL replace the pawn with the chosen piece. Subsequent moves SHALL resolve against the updated board. A rejected move SHALL change nothing.

#### Scenario: Piece relocates

- **WHEN** e2e4 is applied from the starting position
- **THEN** e4 holds a White pawn and e2 is empty

#### Scenario: Later move sees the new position

- **WHEN** e2e4 is applied, Black replies, and White then enters e4e5
- **THEN** the second White move is recognised as a pawn move from e4

#### Scenario: Captured piece is removed

- **WHEN** a White piece moves onto a square occupied by a Black piece
- **THEN** the Black piece is no longer on the board and the White piece occupies that square

#### Scenario: Rejected move leaves the board alone

- **WHEN** a move is rejected as illegal
- **THEN** every square holds what it held before the entry

### Requirement: Non-pawn moves carry the piece letter

A move by a knight, bishop, rook, queen, or king SHALL be rendered with the uppercase piece letter `N`, `B`, `R`, `Q`, or `K` respectively, followed by any disambiguation characters, any capture marker, and the destination square.

#### Scenario: Knight move

- **WHEN** g1f3 is played from the starting position
- **THEN** the notation is `Nf3`

#### Scenario: King move

- **WHEN** the White king legally moves from e1 to e2
- **THEN** the notation is `Ke2`

### Requirement: Captures are marked with x

A move onto a square occupied by an enemy piece SHALL include `x` before the destination square. For a non-pawn move the `x` SHALL follow the piece letter and any disambiguation. For a pawn capture the notation SHALL begin with the file letter of the from-square instead of a piece letter. An en passant capture SHALL be rendered like any other pawn capture, naming the square the capturing pawn moves to.

#### Scenario: Piece capture

- **WHEN** a White bishop on c4 legally moves to f7 where a Black pawn stands
- **THEN** the notation is `Bxf7`

#### Scenario: Pawn capture

- **WHEN** a White pawn on e4 legally moves to d5 where a Black piece stands
- **THEN** the notation is `exd5`

#### Scenario: En passant capture

- **WHEN** a White pawn on e5 captures en passant onto the empty square d6
- **THEN** the notation is `exd6`

#### Scenario: A non-capturing move carries no x

- **WHEN** a piece moves to an empty square with no capture
- **THEN** the notation contains no `x`

### Requirement: Castling is written in the castling form

A legal castle SHALL be rendered as `O-O` when the destination file is `g` and `O-O-O` when the destination file is `c`. The corresponding rook SHALL also be moved on the board: for White, h1 to f1 or a1 to d1; for Black, h8 to f8 or a8 to d8. A king move of two files that is not a legal castle SHALL be rejected rather than rendered.

#### Scenario: Kingside castling

- **WHEN** the White king legally castles e1 to g1
- **THEN** the notation is `O-O`
- **AND** g1 holds the king, f1 holds a rook, and e1 and h1 are empty

#### Scenario: Queenside castling

- **WHEN** the White king legally castles e1 to c1
- **THEN** the notation is `O-O-O`
- **AND** c1 holds the king, d1 holds a rook, and e1 and a1 are empty

#### Scenario: Black castling

- **WHEN** the Black king legally castles e8 to g8
- **THEN** the notation is `O-O`
- **AND** g8 holds the king, f8 holds a rook, and e8 and h8 are empty

#### Scenario: Illegal two-file king move is not notated

- **WHEN** a king move of two files is entered but castling is not legal
- **THEN** no notation is produced and the move is rejected

### Requirement: A pawn reaching the last rank promotes to a queen

A pawn moving to the last rank SHALL be replaced on the board by the piece the player chose, and the notation SHALL end with `=` followed by the uppercase letter of that piece, after the destination square and before any check or checkmate suffix. This SHALL apply to White pawns reaching rank 8 and Black pawns reaching rank 1.

#### Scenario: Promotion

- **WHEN** a White pawn on e7 legally moves to an empty e8 and the player chooses a queen
- **THEN** the notation is `e8=Q` and e8 holds a White queen

#### Scenario: Promotion with capture

- **WHEN** a White pawn on e7 legally captures on d8 and the player chooses a queen
- **THEN** the notation is `exd8=Q`

#### Scenario: Under-promotion

- **WHEN** the player chooses a knight
- **THEN** the notation ends with `=N` and a knight of the moving colour stands on the promotion square

#### Scenario: Black promotion

- **WHEN** a Black pawn legally reaches rank 1 and the player chooses a rook
- **THEN** the notation ends with `=R` and a Black rook stands on the promotion square

#### Scenario: Promotion giving check

- **WHEN** a promotion leaves the enemy king attacked with a reply available
- **THEN** the notation ends with `=` the piece letter followed by `+`

### Requirement: Each accepted move prints one line to standard output

An accepted move SHALL print one line containing the four-character coordinate form, a space, `=`, a space, then the move number, a period, a space, and the algebraic notation. When that move ends the game, a further result line SHALL follow it. No other line SHALL be printed for an accepted move.

#### Scenario: First move output

- **WHEN** the player enters 5254 from the starting position
- **THEN** standard output shows `e2e4 = 1. e4`

#### Scenario: Second move output

- **WHEN** the player then enters 7275
- **THEN** standard output shows `g7g5 = 2. g5`

#### Scenario: Game-ending move prints a result line

- **WHEN** an accepted move delivers checkmate
- **THEN** the move line is followed by a line reporting the result

## ADDED Requirements

### Requirement: Identical pieces are disambiguated

When two or more pieces of the same kind and colour could legally move to the same destination, the notation SHALL identify the mover. The from-square's file letter SHALL be used when it is unique among those pieces, otherwise the from-square's rank digit when that is unique, otherwise both. Only pieces that could make the move legally SHALL be counted, so a piece pinned against its own king SHALL NOT force disambiguation. Pawns and kings SHALL NOT be disambiguated this way.

#### Scenario: File disambiguation

- **WHEN** knights on b1 and f3 can both legally move to d2, and the b1 knight moves there
- **THEN** the notation is `Nbd2`

#### Scenario: Rank disambiguation

- **WHEN** rooks on e1 and e5 can both legally move to e3, and the e1 rook moves there
- **THEN** the notation is `R1e3`

#### Scenario: Full-square disambiguation

- **WHEN** three queens of the same colour can legally reach the same square and neither the file nor the rank alone identifies the mover
- **THEN** the notation names the whole from-square, such as `Qh4e1`

#### Scenario: Disambiguation with a capture

- **WHEN** disambiguation is required and the move is a capture
- **THEN** the disambiguation characters come before the `x`, as in `Nbxd2`

#### Scenario: A pinned twin does not force disambiguation

- **WHEN** a second piece of the same kind could reach the square but is pinned against its own king
- **THEN** no disambiguation is added

#### Scenario: A single candidate needs no disambiguation

- **WHEN** only one piece of that kind and colour can reach the destination
- **THEN** the notation carries only the piece letter and the destination

### Requirement: Check and checkmate are marked

Notation SHALL end with `+` when the move leaves the opponent's king attacked and the opponent has at least one legal reply, and with `#` when the opponent's king is attacked and has no legal reply. A move that leaves the opponent with no legal reply and their king unattacked SHALL carry no suffix.

#### Scenario: Check suffix

- **WHEN** a move attacks the enemy king and the enemy can reply
- **THEN** the notation ends with `+`

#### Scenario: Checkmate suffix

- **WHEN** a move attacks the enemy king and the enemy has no legal reply
- **THEN** the notation ends with `#`

#### Scenario: Stalemate carries no suffix

- **WHEN** a move leaves the enemy with no legal reply and their king unattacked
- **THEN** the notation carries neither `+` nor `#`

#### Scenario: Castling can give check

- **WHEN** castling attacks the enemy king along the rook's new file
- **THEN** the notation is `O-O+` or `O-O-O+` as appropriate

### Requirement: A rejected move prints its reason

A move rejected as illegal SHALL print one line containing the four-character coordinate form, a space, `=`, a space, `illegal:`, a space, and a description of why the move was refused. The description SHALL identify the specific rule that refused the move rather than reporting a generic failure.

#### Scenario: Empty from-square

- **WHEN** the from-square holds no piece
- **THEN** the line reads `<coordinates> = illegal: empty from-square`

#### Scenario: Wrong side to move

- **WHEN** the player moves a piece of the side that is not to move
- **THEN** the line names whose turn it is

#### Scenario: Movement rule refused the move

- **WHEN** a bishop is asked to move along a file
- **THEN** the line names the piece and the square it cannot reach

#### Scenario: Blocked path

- **WHEN** a slider's path to the destination is occupied
- **THEN** the line reports that the path to that square is blocked

#### Scenario: King safety refused the move

- **WHEN** the move would leave the mover's own king attacked
- **THEN** the line reports that the king would be left in check

#### Scenario: Castling refused

- **WHEN** castling is refused
- **THEN** the line distinguishes lost rights, a blocked path, and an attacked square

## REMOVED Requirements

### Requirement: A move from an empty square is rejected

**Reason**: The empty from-square is one legality rule among many now, so it moves to the new `chess-move-legality` capability alongside turn order, movement rules, and king safety, where the rejection reasons are specified together.

**Migration**: The behaviour is unchanged apart from the message wording, which becomes `illegal: empty from-square` in place of `invalid: empty from-square`. See the requirement of the same name in `chess-move-legality`.

### Requirement: Move legality is not verified

**Reason**: This change exists to verify legality, so the requirement states the opposite of the intended behaviour. Its notation clauses are replaced by the new disambiguation and check-suffix requirements.

**Migration**: Callers that relied on any occupied from-square being accepted must now enter legal moves. Moves that break a movement rule, ignore turn order, or leave the mover's king in check are rejected and the board is unchanged; see `chess-move-legality`. Notation now includes disambiguation and `+`/`#` suffixes.
