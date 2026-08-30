## Why

The device exists so a player can play blindfolded at a tournament: moves go in on the keypad, nothing is ever shown to the player. An arbiter, however, has to be able to see the position — to settle a dispute, to check that the device's board matches the physical game, or simply to follow the game. Right now the only view of the board is the serial console, which means a laptop and a USB cable at the table. Giving the device its own Wi-Fi access point and a single HTML page turns any phone into an arbiter's window onto the board with no cable and no infrastructure.

## What Changes

- Bring up Wi-Fi in SoftAP mode at boot with SSID `esp_chessboard`, open (no passphrase), so a phone can join it directly.
- Run an HTTP server on the device that serves one page at `/` rendering the current position as an 8×8 grid of piece letters, addressed at the AP's gateway IP.
- Render pieces as letters, coloured by side: White pieces are white letters outlined in black, Black pieces are black letters outlined in white, so both sides are readable on both light and dark squares.
- Have the page refresh itself so an arbiter watching it sees moves appear as they are entered, without reloading by hand.
- Publish a snapshot of the board from the game task after every accepted move and after a reset, so the HTTP handler can read a consistent position without reaching into the game's own state.
- Initialise NVS at startup, which Wi-Fi requires for its calibration data.
- Register the ESP-IDF Wi-Fi, networking, and HTTP server components as dependencies of `main`.

## Capabilities

### New Capabilities

- `board-preview-access-point`: The Wi-Fi access point itself — SSID, open authentication, when it comes up, how many clients it accepts, and the address the page is reachable at.
- `board-preview-page`: What the served page contains and how it looks — the 8×8 grid, letter-per-piece rendering, the white/black outline treatment, orientation and labelling, self-refresh, and the guarantee that the position shown matches the position the game has accepted.

### Modified Capabilities

None. The keypad, move entry, notation, and LED behaviour are untouched; the preview only reads the position.

## Impact

- `main/main.cpp`: NVS init, Wi-Fi AP startup, HTTP server startup, and the new plumbing between the game and the preview.
- `main/game.cpp` / `main/game.hpp`: publishes a board snapshot after accepted moves and resets. No change to how moves are entered or printed.
- `main/CMakeLists.txt`: new source files and the `esp_wifi` / `esp_http_server` / `nvs_flash` / `esp_event` / `esp_netif` requirements.
- `sdkconfig`: no change expected — the current config already has `ESP_WIFI_SOFTAP_SUPPORT`, `LWIP_DHCPS`, and the HTTP server options enabled by default.
- `tests/`: the HTML rendering is a pure function over a 64-character board, so it is covered by the existing host test target.
- New files for the access point, the HTTP server, the page renderer, and the shared board snapshot.
- **Fair play**: an open access point that publishes the position is, by construction, also a way for the *player* to look at their own board. This change does not attempt to prevent that; it is a tournament-procedure question, and it is called out as a risk rather than solved here.
