## ADDED Requirements

### Requirement: A move from an empty square is rejected

If no piece stands on the from-square, the system SHALL reject the move, SHALL leave the board unchanged, and SHALL NOT increment the move number.

#### Scenario: Empty from-square

- **WHEN** the player enters 5455 (e4e5) from the starting position, where e4 is empty
- **THEN** the move is rejected as illegal with the reason `empty from-square`
- **AND** the board is unchanged and the move number is unchanged

### Requirement: Only the side to move may move

The system SHALL track whose turn it is, starting with White after startup or a reset and alternating after every accepted move. A move whose from-square holds a piece of the side that is not to move SHALL be rejected.

#### Scenario: White moves first

- **WHEN** the player enters a Black pawn move as the first move of the game
- **THEN** the move is rejected as illegal with the reason `it is White's turn`
- **AND** the board is unchanged

#### Scenario: Turn alternates after an accepted move

- **WHEN** White plays e2e4
- **THEN** the next accepted move must be a Black move
- **AND** a second White move in a row is rejected with the reason `it is Black's turn`

#### Scenario: Rejected move does not pass the turn

- **WHEN** White attempts an illegal move
- **THEN** it is still White's turn and White's next legal move is accepted

#### Scenario: Reset returns the move to White

- **WHEN** the reset key is pressed after Black has moved
- **THEN** the next accepted move must be a White move

### Requirement: A piece may not capture its own colour or stay put

A move whose to-square holds a piece of the same colour as the moving piece SHALL be rejected. A move whose from-square and to-square are the same SHALL be rejected.

#### Scenario: Capturing an own piece

- **WHEN** White attempts to move the rook from a1 onto its own pawn on a2
- **THEN** the move is rejected as illegal with the reason `own piece on a2`

#### Scenario: Move to the same square

- **WHEN** the player enters four digits that name the same square twice, such as 5252
- **THEN** the move is rejected as illegal with the reason `from and to are the same square`

### Requirement: Each piece moves only as its kind allows

Every move SHALL be checked against the movement rules of the piece standing on the from-square. A knight SHALL move in an L of two squares and one; a bishop SHALL move only on diagonals; a rook SHALL move only along a rank or file; a queen SHALL move as a rook or bishop; a king SHALL move one square in any direction, except when castling. A move to a square the piece cannot reach SHALL be rejected.

#### Scenario: Bishop moving like a rook

- **WHEN** White attempts to move a bishop along a file
- **THEN** the move is rejected as illegal with the reason `bishop cannot reach` the destination

#### Scenario: Knight L-shape

- **WHEN** White plays g1f3 from the starting position
- **THEN** the move is accepted

#### Scenario: Knight moving in a straight line

- **WHEN** White attempts to move the knight from g1 to g3
- **THEN** the move is rejected as illegal with the reason `knight cannot reach g3`

#### Scenario: King moving two squares other than castling

- **WHEN** a White king on e4 attempts to move to e6
- **THEN** the move is rejected as illegal

#### Scenario: Queen combines rook and bishop movement

- **WHEN** a White queen on d1 has a clear diagonal to h5 and moves there
- **THEN** the move is accepted

### Requirement: Sliding pieces may not jump over occupied squares

A rook, bishop, or queen SHALL be blocked by the first occupied square along its line of travel. It MAY capture the piece on that square if the piece is an enemy, and SHALL NOT move past it.

#### Scenario: Rook blocked by its own pawn

- **WHEN** White attempts a1a5 from the starting position, where the pawn on a2 blocks the file
- **THEN** the move is rejected as illegal with the reason `path to a5 is blocked`

#### Scenario: Queen blocked on the diagonal

- **WHEN** White attempts d1h5 from the starting position, where the pawn on e2 blocks the diagonal
- **THEN** the move is rejected as illegal with the reason `path to h5 is blocked`

#### Scenario: Slider captures the first piece on its line

- **WHEN** a White rook on a1 has an enemy piece on a5 and empty squares between
- **THEN** a1a5 is accepted and the enemy piece is captured

#### Scenario: Knight is not blocked

- **WHEN** White plays g1f3 from the starting position, with pawns on both sides of the knight
- **THEN** the move is accepted, because a knight is not blocked by intervening pieces

### Requirement: Pawns move forward, capture diagonally, and advance two squares only from home

A pawn SHALL move one square toward the opponent's side onto an empty square. It MAY move two squares from its starting rank only when both the intermediate and the destination squares are empty. It SHALL capture only diagonally forward onto a square occupied by an enemy piece, or by en passant. It SHALL NOT move backward, SHALL NOT capture straight ahead, and SHALL NOT move diagonally onto an empty square except by en passant.

#### Scenario: Two-square advance from home

- **WHEN** White plays e2e4 from the starting position
- **THEN** the move is accepted

#### Scenario: Two-square advance from a non-home rank

- **WHEN** a White pawn on e4 attempts to move to e6
- **THEN** the move is rejected as illegal

#### Scenario: Two-square advance blocked in the middle

- **WHEN** a White pawn on e2 attempts e2e4 with a piece standing on e3
- **THEN** the move is rejected as illegal with the reason `path to e4 is blocked`

#### Scenario: Pawn cannot capture straight ahead

- **WHEN** a White pawn on e4 faces a Black piece on e5 and attempts e4e5
- **THEN** the move is rejected as illegal

#### Scenario: Pawn cannot move diagonally onto an empty square

- **WHEN** a White pawn on e4 attempts e4d5 while d5 is empty and no en passant is available
- **THEN** the move is rejected as illegal

#### Scenario: Pawn captures diagonally

- **WHEN** a White pawn on e4 faces a Black piece on d5 and plays e4d5
- **THEN** the move is accepted and the Black piece is captured

#### Scenario: Pawn cannot move backward

- **WHEN** a White pawn on e4 attempts e4e3
- **THEN** the move is rejected as illegal

#### Scenario: Black pawns move toward rank 1

- **WHEN** Black plays e7e5 from the starting position after a White move
- **THEN** the move is accepted

### Requirement: A move that leaves the mover's own king in check is rejected

After a move is applied, the moving side's king SHALL NOT be attacked by any enemy piece. Any move that would leave it attacked SHALL be rejected, whether the king moves, a piece shielding the king moves away, or a piece other than the king moves while the king is already in check.

#### Scenario: Pinned piece may not move off the pinning line

- **WHEN** a White knight on e2 stands between the White king on e1 and a Black rook on e8, and the knight is moved to c3
- **THEN** the move is rejected as illegal with the reason `king would be left in check`

#### Scenario: Pinned piece may move along the pinning line

- **WHEN** a White rook on e2 stands between the White king on e1 and a Black rook on e8, and the White rook moves to e5
- **THEN** the move is accepted, because the king is still shielded

#### Scenario: Pinned piece may capture the pinning piece

- **WHEN** a White bishop is pinned against its king by a Black bishop and captures that Black bishop
- **THEN** the move is accepted

#### Scenario: King may not walk into check

- **WHEN** the White king attempts to move to a square attacked by a Black piece
- **THEN** the move is rejected as illegal with the reason `king would be left in check`

#### Scenario: King may not stay on a line by stepping along it

- **WHEN** the White king is checked along a rank by a Black rook and attempts to move to another square on that same rank
- **THEN** the move is rejected as illegal, because the king would still be attacked

#### Scenario: Check must be answered

- **WHEN** the White king is in check and White attempts an unrelated move that neither captures the checking piece, blocks the check, nor moves the king
- **THEN** the move is rejected as illegal with the reason `king would be left in check`

#### Scenario: Blocking a check is accepted

- **WHEN** the White king is in check from a distant slider and a White piece moves onto a square between them
- **THEN** the move is accepted

#### Scenario: Capturing the checking piece is accepted

- **WHEN** the White king is in check and a White piece captures the checking piece without exposing the king
- **THEN** the move is accepted

#### Scenario: A king may be adjacent to nothing

- **WHEN** the White king attempts to move to a square adjacent to the Black king
- **THEN** the move is rejected as illegal, because that square is attacked by the Black king

### Requirement: Castling requires unspent rights

Castling SHALL be entered as a king move of two files from its home square. It SHALL be rejected once the king has moved, once the rook on that side has moved, or once that rook has been captured on its home square. Rights once lost SHALL NOT return, including when a rook moves away and later returns to its home square.

#### Scenario: Castling after the king has moved

- **WHEN** the White king moves from e1 and later returns to e1, and White then attempts e1g1
- **THEN** the move is rejected as illegal with the reason `castling rights lost`

#### Scenario: Castling after the rook has moved

- **WHEN** the h1 rook has moved and White attempts e1g1 with all squares between the king and h1 empty
- **THEN** the move is rejected as illegal with the reason `castling rights lost`

#### Scenario: Rook returning to its home square does not restore the right

- **WHEN** the h1 rook moves to h5 and later returns to h1, and White attempts e1g1
- **THEN** the move is rejected as illegal with the reason `castling rights lost`

#### Scenario: Capturing a rook on its home square removes that right

- **WHEN** a Black piece captures the White rook on h1 and White later has a rook on h1 again
- **THEN** White's kingside castling is rejected as illegal with the reason `castling rights lost`

#### Scenario: Losing one side's right does not affect the other

- **WHEN** the h1 rook has moved but the a1 rook and the king have not
- **THEN** e1g1 is rejected and e1c1 remains available

### Requirement: Castling requires an empty path and safe squares

Castling SHALL be rejected if any square between the king and the rook is occupied. It SHALL be rejected if the king is in check, if the square the king passes over is attacked, or if the destination square is attacked. For queenside castling the square adjacent to the rook MAY be attacked, but SHALL be empty.

#### Scenario: Pieces between the king and rook

- **WHEN** White attempts e1g1 from the starting position, with the bishop on f1 and the knight on g1
- **THEN** the move is rejected as illegal with the reason `castling path is blocked`

#### Scenario: Castling out of check

- **WHEN** the White king is in check and White attempts e1g1 with an otherwise legal castle
- **THEN** the move is rejected as illegal with the reason `cannot castle out of, through, or into check`

#### Scenario: Castling through an attacked square

- **WHEN** f1 is attacked by a Black piece and White attempts e1g1
- **THEN** the move is rejected as illegal with the reason `cannot castle out of, through, or into check`

#### Scenario: Castling into an attacked square

- **WHEN** g1 is attacked by a Black piece and White attempts e1g1
- **THEN** the move is rejected as illegal with the reason `cannot castle out of, through, or into check`

#### Scenario: Queenside castling over an attacked b-file square

- **WHEN** b1 is attacked but empty, and c1, d1, e1 are safe and c1 and d1 are empty
- **THEN** e1c1 is accepted

#### Scenario: Queenside castling with a piece on b1

- **WHEN** b1 is occupied and White attempts e1c1
- **THEN** the move is rejected as illegal with the reason `castling path is blocked`

#### Scenario: Legal castling moves both pieces

- **WHEN** all castling conditions are met and White plays e1g1
- **THEN** the move is accepted, g1 holds the king, f1 holds the rook, and e1 and h1 are empty

### Requirement: En passant is available only on the move immediately after the double advance

When a pawn advances two squares, the square it passed over SHALL become the en passant target for the opponent's next move only. An enemy pawn on the adjacent file and correct rank MAY capture onto that square, removing the pawn that advanced. The target SHALL be cleared after any move, so the capture SHALL be rejected if the opponent plays anything else first.

#### Scenario: En passant capture immediately after the advance

- **WHEN** a White pawn stands on e5, Black plays d7d5, and White plays e5d6
- **THEN** the move is accepted, e5 and d5 are empty, and d6 holds the White pawn

#### Scenario: En passant expires after an intervening move

- **WHEN** a White pawn stands on e5, Black plays d7d5, White plays some other move, Black replies, and White then attempts e5d6
- **THEN** the move is rejected as illegal with the reason `en passant no longer available`

#### Scenario: No en passant after a single-square advance

- **WHEN** a White pawn stands on e5, Black plays d6d5, and White attempts e5d6
- **THEN** the move is rejected as illegal

#### Scenario: En passant that would expose the own king is rejected

- **WHEN** the capturing pawn and the captured pawn are the only pieces between the mover's king and an enemy rook on the same rank, and the en passant capture is attempted
- **THEN** the move is rejected as illegal with the reason `king would be left in check`

#### Scenario: Black captures en passant

- **WHEN** a Black pawn stands on d4, White plays e2e4, and Black plays d4e3
- **THEN** the move is accepted, d4 and e4 are empty, and e3 holds the Black pawn

### Requirement: A pawn reaching the last rank promotes

A White pawn moving to rank 8 and a Black pawn moving to rank 1 SHALL be replaced on the board by a piece of the mover's colour chosen by the player. The choice SHALL NOT affect whether the move is legal. A pawn SHALL NOT remain a pawn on the last rank.

#### Scenario: White promotion

- **WHEN** a White pawn on e7 moves to an empty e8 and the player chooses a queen
- **THEN** e8 holds a White queen and e7 is empty

#### Scenario: Black promotion

- **WHEN** a Black pawn on e2 moves to an empty e1 and the player chooses a knight
- **THEN** e1 holds a Black knight and e2 is empty

#### Scenario: Promotion by capture

- **WHEN** a White pawn on e7 captures on d8 and the player chooses a rook
- **THEN** d8 holds a White rook and both e7 and the captured piece are gone

#### Scenario: Illegal promotion move is rejected before the choice

- **WHEN** the four digits describe a pawn move to the last rank that is blocked or would leave the king in check
- **THEN** the move is rejected as illegal on the fourth digit and no promotion piece is requested

#### Scenario: Under-promotion is legal

- **WHEN** a promotion is completed with a choice other than a queen
- **THEN** the move is accepted and the chosen piece stands on the promotion square

### Requirement: A rejected move changes nothing

When a move is rejected as illegal, the system SHALL leave every square of the board unchanged, SHALL leave the side to move unchanged, SHALL leave castling rights and the en passant target unchanged, and SHALL NOT increment the move number.

#### Scenario: Board is untouched after a rejection

- **WHEN** an illegal move is entered
- **THEN** every square holds what it held before, and the board preview does not change

#### Scenario: Move number survives a rejection

- **WHEN** a move is rejected and then a legal move is entered
- **THEN** the legal move takes the number the rejected move would have had

#### Scenario: En passant survives a rejection

- **WHEN** Black plays a two-square pawn advance, White enters an illegal move, and White then plays the en passant capture
- **THEN** the en passant capture is accepted, because the rejected entry did not consume the opportunity
