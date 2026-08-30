## ADDED Requirements

### Requirement: Board holds the standard starting position

The system SHALL maintain an 8×8 board. On startup and after a reset the board SHALL hold the standard chess starting position: White rooks on a1 and h1, knights on b1 and g1, bishops on c1 and f1, the queen on d1, the king on e1, White pawns across rank 2; the mirrored Black pieces on rank 8 with Black pawns across rank 7; ranks 3 through 6 empty.

#### Scenario: Starting placement

- **WHEN** the board is initialised or reset
- **THEN** e1 holds the White king, d1 the White queen, e2 a White pawn, g8 a Black knight, and e4 is empty

### Requirement: A completed move updates the board

Applying a move SHALL move the piece from the from-square to the to-square and leave the from-square empty. Any piece already on the to-square SHALL be removed from the board. Subsequent moves SHALL resolve against the updated board.

#### Scenario: Piece relocates

- **WHEN** e2e4 is applied from the starting position
- **THEN** e4 holds a White pawn and e2 is empty

#### Scenario: Later move sees the new position

- **WHEN** e2e4 is applied and then e4e5 is entered
- **THEN** the second move is recognised as a pawn move from e4

#### Scenario: Captured piece is removed

- **WHEN** a White piece moves onto a square occupied by a Black piece
- **THEN** the Black piece is no longer on the board and the White piece occupies that square

### Requirement: A move from an empty square is rejected

If no piece stands on the from-square, the system SHALL reject the move: it SHALL print a rejection message, SHALL leave the board unchanged, and SHALL NOT increment the move number.

#### Scenario: Empty from-square

- **WHEN** the player enters 5454 (e4e5) from the starting position, where e4 is empty
- **THEN** a rejection message is printed
- **AND** the board is unchanged and the move number is unchanged

### Requirement: Pawn moves are written as the destination square

A pawn move that is not a capture SHALL be rendered as the destination square alone, with no piece letter.

#### Scenario: Pawn advance

- **WHEN** e2e4 is played from the starting position
- **THEN** the notation is `e4`

#### Scenario: Single-step pawn advance

- **WHEN** d2d3 is played from the starting position
- **THEN** the notation is `d3`

### Requirement: Non-pawn moves carry the piece letter

A move by a knight, bishop, rook, queen, or king SHALL be rendered with the uppercase piece letter `N`, `B`, `R`, `Q`, or `K` respectively, followed by the destination square.

#### Scenario: Knight move

- **WHEN** g1f3 is played from the starting position
- **THEN** the notation is `Nf3`

#### Scenario: King move

- **WHEN** the White king moves from e1 to e2
- **THEN** the notation is `Ke2`

### Requirement: Captures are marked with x

A move onto an occupied square SHALL include `x` before the destination square. For a non-pawn move the `x` SHALL follow the piece letter. For a pawn capture the notation SHALL begin with the file letter of the from-square instead of a piece letter. A diagonal pawn move onto an empty square SHALL also be rendered as a capture.

#### Scenario: Piece capture

- **WHEN** a White bishop on c4 moves to f7 where a Black pawn stands
- **THEN** the notation is `Bxf7`

#### Scenario: Pawn capture

- **WHEN** a White pawn on e4 moves to d5 where a Black piece stands
- **THEN** the notation is `exd5`

#### Scenario: Diagonal pawn move to an empty square

- **WHEN** a White pawn on e5 moves diagonally to d6, which is empty
- **THEN** the notation is `exd6`

### Requirement: Castling is written in the castling form

A king move of two files from its home square SHALL be rendered as `O-O` when the destination file is `g` and `O-O-O` when the destination file is `c`. The corresponding rook SHALL also be moved on the board: for `O-O` from h1 to f1, and for `O-O-O` from a1 to d1.

#### Scenario: Kingside castling

- **WHEN** the White king moves e1 to g1
- **THEN** the notation is `O-O`
- **AND** g1 holds the king, f1 holds a rook, and e1 and h1 are empty

#### Scenario: Queenside castling

- **WHEN** the White king moves e1 to c1
- **THEN** the notation is `O-O-O`
- **AND** c1 holds the king, d1 holds a rook, and e1 and a1 are empty

### Requirement: A pawn reaching the last rank promotes to a queen

A White pawn moving to rank 8 SHALL be replaced on the board by a White queen, and the notation SHALL end with `=Q` after the destination square.

#### Scenario: Promotion

- **WHEN** a White pawn on e7 moves to e8, which is empty
- **THEN** the notation is `e8=Q` and e8 holds a White queen

#### Scenario: Promotion with capture

- **WHEN** a White pawn on e7 captures on d8
- **THEN** the notation is `exd8=Q`

### Requirement: Accepted moves are numbered from 1

Each accepted move SHALL be assigned the next move number, starting at 1 for the first move after startup or reset and incrementing by one per accepted move. Rejected moves SHALL NOT consume a number.

#### Scenario: Numbering increments

- **WHEN** e2e4 is played, then g1f3
- **THEN** the first is numbered 1 and the second is numbered 2

#### Scenario: Rejected move keeps the number

- **WHEN** a move is rejected and then a valid move is entered
- **THEN** the valid move takes the number the rejected move would have had

### Requirement: Each accepted move prints one line to standard output

An accepted move SHALL print exactly one line containing the four-character coordinate form, a space, `=`, a space, then the move number, a period, a space, and the algebraic notation.

#### Scenario: First move output

- **WHEN** the player enters 5254 from the starting position
- **THEN** standard output shows `e2e4 = 1. e4`

#### Scenario: Second move output

- **WHEN** the player then enters 7163
- **THEN** standard output shows `g1f3 = 2. Nf3`

### Requirement: Move legality is not verified

The system SHALL accept any four-digit move whose from-square holds a piece, without checking that the piece can legally make that move. Notation SHALL NOT include disambiguation characters and SHALL NOT include check or checkmate suffixes.

#### Scenario: Illegal move is accepted

- **WHEN** the player enters a move that no chess rule permits, such as a rook jumping over its own pawn
- **THEN** the move is accepted, applied to the board, and printed

#### Scenario: No disambiguation

- **WHEN** a knight moves to a square that another knight could also reach
- **THEN** the notation names only the piece letter and destination, with no file or rank disambiguation

#### Scenario: No check suffix

- **WHEN** a move gives check to the Black king
- **THEN** the notation carries no `+` or `#` suffix
