## MODIFIED Requirements

### Requirement: Failure to bring up the preview never stops the game

If any part of the preview fails to start — persistent storage, the network interface, the Wi-Fi driver, or the HTTP server — the device SHALL report the failure on standard output and SHALL continue into normal operation. Move entry, notation output, the LED, the reset key, and Bluetooth pairing and play SHALL all work with no access point present. A preview failure SHALL NOT reboot or halt the device, and SHALL NOT prevent a Bluetooth link from being formed or kept.

#### Scenario: Wi-Fi fails to start

- **WHEN** starting the Wi-Fi access point returns an error
- **THEN** the error is reported on standard output
- **AND** the device continues running and a four-digit move still produces its notation line and LED blink

#### Scenario: HTTP server fails to start

- **WHEN** the HTTP server fails to start
- **THEN** the error is reported on standard output and move entry is unaffected

#### Scenario: No abort on error

- **WHEN** any preview startup step fails
- **THEN** the device does not reboot and does not stop responding to the keypad

#### Scenario: Pairing works without the preview

- **WHEN** the access point failed to start and the player holds `B` for three seconds
- **THEN** pairing proceeds and a link can be formed and played over

### Requirement: The preview does not disturb move entry

Running the access point and the HTTP server SHALL NOT change the behaviour of the keypad, the notation printed to standard output, or the LED. Serving requests SHALL NOT cause keypresses to be missed or reordered. Because the Wi-Fi radio is shared with Bluetooth, serving requests SHALL NOT cause a Bluetooth link to drop, SHALL NOT cause a transmitted move to be lost, and SHALL NOT delay a move reaching the peer to the point that the acknowledgement times out.

#### Scenario: Moves are unaffected while the board is being watched

- **WHEN** a phone is refreshing the preview page and the player enters a four-digit move
- **THEN** the move produces its usual notation line and single LED blink

#### Scenario: Requests do not drop keypresses

- **WHEN** the player enters four digits in quick succession while requests are being served
- **THEN** all four digits are received in the order pressed and the intended move is processed

#### Scenario: Requests do not drop the link

- **WHEN** phones are refreshing the preview page on both boards during a linked game
- **THEN** the link stays up and moves continue to be exchanged and acknowledged

#### Scenario: Requests do not lose a move

- **WHEN** a move is accepted while the HTTP server is serving a request
- **THEN** the move reaches the peer and is acknowledged

## ADDED Requirements

### Requirement: The preview remains available while a Bluetooth link is active

The access point and the preview page SHALL remain available throughout pairing and throughout a linked game, so that an arbiter watching either board's page keeps seeing the position. Enabling Bluetooth SHALL NOT require the access point to be stopped, and forming, losing, or ending a link SHALL NOT take the access point down or disconnect a joined client.

#### Scenario: The page works while paired

- **WHEN** two boards are linked and a phone is joined to one board's access point
- **THEN** the preview page is served and shows the current position

#### Scenario: Pairing does not disconnect a client

- **WHEN** a phone is joined to the access point and the player holds `B` to pair
- **THEN** the phone stays joined and the page keeps loading

#### Scenario: Both boards can be watched

- **WHEN** one phone is joined to each linked board's access point
- **THEN** both pages show the same position after each move is exchanged

#### Scenario: Losing the link does not affect the page

- **WHEN** the Bluetooth link drops
- **THEN** the access point stays up and the preview page still serves the position that board holds
