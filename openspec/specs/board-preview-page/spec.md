# board-preview-page Specification

## Purpose
TBD - created by archiving change chess-preview-access-point. Update Purpose after archive.
## Requirements
### Requirement: A single page serves the current position

An HTTP `GET` of `/` SHALL return a complete, self-contained HTML document showing the position, with an HTML content type. The document SHALL require no further requests to render: no external stylesheet, script, image, or font. Any other path SHALL return a not-found response rather than the page.

#### Scenario: Root path returns the board

- **WHEN** a browser issues `GET /`
- **THEN** the response is a complete HTML document rendering the position

#### Scenario: No external resources

- **WHEN** the page is loaded
- **THEN** it renders fully with no additional requests to the device or to the internet

#### Scenario: Unknown path

- **WHEN** a browser requests a path other than `/`
- **THEN** the device responds with a not-found status and does not serve the board

### Requirement: The board is an 8x8 grid of squares

The page SHALL render the board as an 8×8 grid of squares. Squares SHALL be tinted in the alternating light/dark chessboard pattern, with a1 dark. The grid SHALL be oriented with rank 8 at the top and file `a` on the left. Files SHALL be labelled `a` through `h` and ranks `1` through `8` so a square can be identified without counting. The grid SHALL stay square-shaped and fit the width of a phone screen without horizontal scrolling or pinch-zooming.

#### Scenario: Grid shape and orientation

- **WHEN** the page is rendered from the starting position
- **THEN** it contains 64 squares, with the Black pieces along the top edge and the White pieces along the bottom edge

#### Scenario: Alternating tints

- **WHEN** the page is rendered
- **THEN** adjacent squares differ in tint and a1 is a dark square

#### Scenario: Coordinates are labelled

- **WHEN** the arbiter looks at the page
- **THEN** the files are labelled `a` to `h` and the ranks `1` to `8`

#### Scenario: Fits a phone screen

- **WHEN** the page is opened on a phone in portrait orientation
- **THEN** the whole board is visible at once without horizontal scrolling or zooming

### Requirement: Each piece is drawn as an uppercase letter on its square

Every piece on the board SHALL be drawn as a single uppercase letter on the square it occupies: `K` king, `Q` queen, `R` rook, `B` bishop, `N` knight, `P` pawn. Both colours SHALL use the same uppercase letters. An empty square SHALL show no letter.

#### Scenario: Starting position letters

- **WHEN** the page is rendered from the starting position
- **THEN** e1 shows `K`, d1 shows `Q`, a1 shows `R`, g1 shows `N`, e2 shows `P`, and e8 shows `K`

#### Scenario: Empty squares are blank

- **WHEN** the page is rendered from the starting position
- **THEN** every square on ranks 3 through 6 shows no letter

#### Scenario: Letters follow the position

- **WHEN** the position has a White pawn on e4 and nothing on e2
- **THEN** e4 shows `P` and e2 shows no letter

### Requirement: Colour is conveyed by letter fill and outline

A White piece SHALL be drawn as a white letter with a black outline. A Black piece SHALL be drawn as a black letter with a white outline. The outline SHALL be applied so that letters of both colours remain legible on both light and dark squares.

#### Scenario: White piece appearance

- **WHEN** a White piece is rendered
- **THEN** its letter is filled white and outlined in black

#### Scenario: Black piece appearance

- **WHEN** a Black piece is rendered
- **THEN** its letter is filled black and outlined in white

#### Scenario: Both colours legible on both square tints

- **WHEN** the arbiter views a White piece on a light square and a Black piece on a dark square
- **THEN** both letters are distinguishable from the square behind them

#### Scenario: Colour does not depend on letter case

- **WHEN** a White piece and a Black piece of the same type are both on the board
- **THEN** they show the same letter and are told apart only by fill and outline

### Requirement: The page updates itself while it is open

The page SHALL refresh its own contents periodically, without the arbiter reloading it, so that moves entered on the keypad appear on their own. The refresh interval SHALL be a small number of seconds. A refresh SHALL NOT be served from a cached copy of an earlier position.

#### Scenario: A move appears without reloading

- **WHEN** the page is open on a phone and the player completes a four-digit move
- **THEN** the phone shows the updated position within a few seconds with no interaction

#### Scenario: A reset appears without reloading

- **WHEN** the page is open and the player presses the reset key
- **THEN** the phone shows the starting position within a few seconds

#### Scenario: Refreshes are not cached

- **WHEN** the page refreshes itself
- **THEN** the response reflects the device's current position rather than a cached earlier one

### Requirement: The served position matches the position the game accepted

The position on the page SHALL be the position after the most recently accepted move. It SHALL NOT show a partially applied move: a move that relocates more than one piece, such as castling, SHALL appear either entirely applied or not applied at all. A move that was rejected, or a move that is still being typed, SHALL NOT change the position shown.

#### Scenario: Position reflects accepted moves

- **WHEN** the player has completed the moves e2e4 and g1f3
- **THEN** the page shows `P` on e4, `N` on f3, and nothing on e2 or g1

#### Scenario: Castling appears atomically

- **WHEN** the page is requested repeatedly while White castles
- **THEN** no response shows the king moved without the rook, or the rook moved without the king

#### Scenario: Partial entry does not change the page

- **WHEN** the player has pressed fewer than four digits of a move
- **THEN** the page still shows the position before that move

#### Scenario: Rejected move does not change the page

- **WHEN** a four-digit move is rejected because the from-square is empty
- **THEN** the page shows the same position as before the attempt

#### Scenario: Starting position is served before any move

- **WHEN** the page is requested after startup and before any move has been entered
- **THEN** it shows the standard starting position rather than an empty or blank board

