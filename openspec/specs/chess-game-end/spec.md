# chess-game-end Specification

## Purpose
TBD - created by archiving change detect-legal-moves. Update Purpose after archive.
## Requirements
### Requirement: Checkmate is detected and reported

After an accepted move, if the side now to move has no legal move and its king is attacked, the system SHALL declare checkmate and SHALL print a result line naming the winning side after the move line.

#### Scenario: White delivers checkmate

- **WHEN** White plays a move after which Black has no legal move and the Black king is attacked
- **THEN** the move is accepted and printed with a `#` suffix
- **AND** a result line reports checkmate and that White wins

#### Scenario: Black delivers checkmate

- **WHEN** Black plays a move after which White has no legal move and the White king is attacked
- **THEN** the move is accepted and a result line reports checkmate and that Black wins

#### Scenario: Check alone is not checkmate

- **WHEN** a move attacks the enemy king but the enemy has at least one legal reply
- **THEN** no result line is printed and the game continues

### Requirement: Stalemate is detected and reported

After an accepted move, if the side now to move has no legal move and its king is not attacked, the system SHALL declare stalemate and SHALL print a result line reporting a draw after the move line.

#### Scenario: Stalemate ends the game

- **WHEN** a move leaves the opponent with no legal move and their king not attacked
- **THEN** the move is accepted with no check or checkmate suffix
- **AND** a result line reports stalemate and a draw

#### Scenario: Having only illegal moves is stalemate, not checkmate

- **WHEN** every pseudo-legal move of the side to move would leave their own king in check, and their king is not currently attacked
- **THEN** the position is reported as stalemate

### Requirement: The mating move is confirmed like any other legal move

The move that ends the game SHALL be treated as an accepted move: the board SHALL be updated, the move number SHALL be consumed, the move SHALL be printed, the board preview SHALL be published, and the LED SHALL give the accepted-move signal.

#### Scenario: LED confirms the final move

- **WHEN** a move delivers checkmate
- **THEN** the LED gives the accepted-move signal, not the rejection signal

#### Scenario: Preview shows the final position

- **WHEN** a move delivers checkmate or stalemate
- **THEN** the board preview shows the position after that move

### Requirement: No move is accepted after the game has ended

Once checkmate or stalemate has been declared, the system SHALL reject every completed move entry, SHALL leave the board unchanged, and SHALL give the rejection signal, until the game is reset.

#### Scenario: Move after checkmate is refused

- **WHEN** the player enters a four-digit move after checkmate has been declared
- **THEN** the move is rejected as illegal with the reason `game is over, press reset`
- **AND** the board and the move number are unchanged

#### Scenario: A move that would otherwise be legal is still refused

- **WHEN** the entered move would have been legal in the final position
- **THEN** it is still rejected because the game has ended

#### Scenario: Digit entry still accumulates

- **WHEN** the player presses digits after the game has ended
- **THEN** nothing happens until the fourth digit, which produces the rejection

### Requirement: Reset clears the game result

Pressing the reset key after checkmate or stalemate SHALL clear the result and start a new game, so that moves are accepted again from the starting position with White to move and move number 1.

#### Scenario: New game after checkmate

- **WHEN** the reset key is pressed after checkmate
- **THEN** the board holds the starting position
- **AND** the next legal White move is accepted and numbered 1

