# board-preview-access-point Specification

## Purpose
TBD - created by archiving change chess-preview-access-point. Update Purpose after archive.
## Requirements
### Requirement: The device hosts an open access point named esp_chessboard

The device SHALL operate its Wi-Fi radio as a software access point advertising the SSID `esp_chessboard`. The network SHALL be open: no passphrase, no authentication step, so that a phone can join it from its Wi-Fi list without entering a credential. The SSID SHALL be exactly `esp_chessboard`, with no suffix and no hidden-network flag.

#### Scenario: Access point is visible

- **WHEN** the device has finished starting up
- **THEN** a phone scanning for Wi-Fi networks in range lists `esp_chessboard`

#### Scenario: Joining requires no password

- **WHEN** an arbiter selects `esp_chessboard` on a phone
- **THEN** the phone joins without prompting for a passphrase

#### Scenario: Station mode is not used

- **WHEN** the device starts up
- **THEN** it does not attempt to join any other network

### Requirement: The access point comes up at boot and stays up

The access point SHALL be started as part of device startup, without any keypress or other user action, and SHALL remain available for as long as the device is powered. The access point SHALL NOT be affected by move entry or by a game reset.

#### Scenario: Available without interaction

- **WHEN** the device is powered on and no key has ever been pressed
- **THEN** the access point is available and the preview page can be opened

#### Scenario: Survives a game reset

- **WHEN** the player presses the reset key
- **THEN** the access point stays up and a joined client stays joined

### Requirement: A joined client is addressed automatically and reaches the page at the gateway

The device SHALL assign an address to each joining client automatically, so that no manual network configuration is needed on the phone. The preview page SHALL be served over HTTP on the default port at the device's gateway address on that network.

#### Scenario: Client is addressed automatically

- **WHEN** a phone joins `esp_chessboard`
- **THEN** it receives an address and a gateway address without manual configuration

#### Scenario: Page is reachable at the gateway address

- **WHEN** the arbiter opens `http://<gateway address>/` in a phone browser
- **THEN** the board preview page is returned

### Requirement: The access point accepts several clients at once

The access point SHALL accept more than one simultaneous client, and SHALL serve the preview page to each of them independently. The number of simultaneous clients SHALL be bounded so that clients cannot exhaust the device's resources.

#### Scenario: Two phones view the board

- **WHEN** two phones are joined and both open the preview page
- **THEN** both are served the current position

#### Scenario: Client limit is bounded

- **WHEN** more clients attempt to join than the configured limit allows
- **THEN** the excess clients are refused and the already-joined clients keep working

### Requirement: Failure to bring up the preview never stops the game

If any part of the preview fails to start — persistent storage, the network interface, the Wi-Fi driver, or the HTTP server — the device SHALL report the failure on standard output and SHALL continue into normal operation. Move entry, notation output, the LED, and the reset key SHALL all work with no access point present. A preview failure SHALL NOT reboot or halt the device.

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

### Requirement: The preview does not disturb move entry

Running the access point and the HTTP server SHALL NOT change the behaviour of the keypad, the notation printed to standard output, or the LED. Serving requests SHALL NOT cause keypresses to be missed or reordered.

#### Scenario: Moves are unaffected while the board is being watched

- **WHEN** a phone is refreshing the preview page and the player enters a four-digit move
- **THEN** the move produces its usual notation line and single LED blink

#### Scenario: Requests do not drop keypresses

- **WHEN** the player enters four digits in quick succession while requests are being served
- **THEN** all four digits are received in the order pressed and the intended move is processed

