## 1. Page renderer (host-testable, no ESP-IDF)

- [x] 1.1 Create `main/board_page.hpp` declaring `size_t renderBoardPage(const char cells[64], char* out, size_t out_size)`, where `cells` is indexed `rank * 8 + file` with FEN letters and `' '` for empty, returning bytes written or 0 if the document would not fit
- [x] 1.2 Implement the document head in `main/board_page.cpp`: `<!DOCTYPE html>`, a viewport meta tag for phone scaling, and a `<meta http-equiv="refresh" content="2">`
- [x] 1.3 Add the inline `<style>`: square cells sized in `vmin`, light `#f0d9b5` and dark `#b58863` square classes (`l`/`d`), a centred bold letter, and piece classes `w` (white fill, black outline) and `b` (black fill, white outline) using `-webkit-text-stroke` plus a four-way `text-shadow` fallback
- [x] 1.4 Emit the 8×8 table from rank 8 down to rank 1 and file `a` to `h`, choosing the square class so a1 is dark, emitting the uppercase letter of the piece with class `w` or `b` by the case of the stored character, and leaving empty squares blank
- [x] 1.5 Emit the rank labels down one side and the file labels along the bottom
- [x] 1.6 Make every write bounds-checked against `out_size`, returning 0 on overflow rather than truncating, and always NUL-terminating on success
- [x] 1.7 Add `../main/board_page.cpp` to the `chess_tests` target in `tests/CMakeLists.txt`

## 2. Renderer tests

- [x] 2.1 Add a host test that renders a starting position and asserts the returned size is non-zero and comfortably under 4096, so the buffer size is guarded against future growth
- [x] 2.2 Assert the document contains exactly 64 square cells and that the refresh meta tag and both piece classes are present
- [x] 2.3 Assert the letter and colour class for specific squares of the starting position: `K` as white on e1, `Q` as white on d1, `N` as black on g8, `P` as black on e7
- [x] 2.4 Assert an empty board renders 64 cells with no piece letters, and that a board with one White pawn on e4 shows it there and nowhere else
- [x] 2.5 Assert that an `out_size` too small for the document yields a return of 0
- [x] 2.6 Build and run the host tests with `cmake -S tests -B build-host && cmake --build build-host && ctest --test-dir build-host`

## 3. Board snapshot across tasks

- [x] 3.1 Create `main/board_snapshot.hpp` with a `BoardSnapshot` class owning a length-1 FreeRTOS queue whose item is a 64-byte struct of piece characters indexed `rank * 8 + file`
- [x] 3.2 Implement `publish(const ChessBoard&)` in `main/board_snapshot.cpp`, filling the struct with `ChessBoard::pieceAt()` over all 64 squares and publishing with `xQueueOverwrite`
- [x] 3.3 Implement `read(char cells[64])` using `xQueuePeek` with a zero timeout, returning false if nothing has been published yet
- [x] 3.4 Confirm no change is needed to `chess_board.*` or `chess_game.*` and that they stay free of FreeRTOS includes

## 4. Game publishes the position

- [x] 4.1 Add a `BoardSnapshot&` member and constructor parameter to `Game` in `main/game.hpp`
- [x] 4.2 Publish the starting position once at the top of `Game::run()`, before the key loop, so the snapshot is never empty
- [x] 4.3 Publish from a single place in `Game::handleKey()` after an accepted move and after a reset, leaving rejected moves and partial entry without a publish
- [x] 4.4 Confirm the printed notation, the move counter, and the LED blink are untouched by this change

## 5. Wi-Fi access point

- [x] 5.1 Create `main/wifi_access_point.hpp` and `main/wifi_access_point.cpp` with a `WifiAccessPoint` class exposing `bool start()`, holding a `StandardOutput&` for error reporting
- [x] 5.2 Initialise NVS, retrying once after `nvs_flash_erase()` on `ESP_ERR_NVS_NO_FREE_PAGES` or `ESP_ERR_NVS_NEW_VERSION_FOUND`
- [x] 5.3 Call `esp_netif_init()` and `esp_event_loop_create_default()`, then create the default SoftAP netif through the error-returning `esp_netif_new()` / `esp_netif_attach_wifi_ap()` / `esp_wifi_set_default_wifi_ap_handlers()` sequence so a setup failure cannot abort the game
- [x] 5.4 Initialise the driver with `WIFI_INIT_CONFIG_DEFAULT()`, set `WIFI_MODE_AP`, and configure the AP with SSID `esp_chessboard`, its exact length, an empty password, `WIFI_AUTH_OPEN`, channel 1, and `max_connection` 4
- [x] 5.5 Start the radio and report the SSID and the gateway address through `StandardOutput` so the address is visible on the console
- [x] 5.6 Check the return of every step, report the failing step and its error name through `StandardOutput`, return false, and use no `ESP_ERROR_CHECK` anywhere in the file

## 6. HTTP server

- [x] 6.1 Create `main/board_server.hpp` and `main/board_server.cpp` with a `BoardServer` class holding a `BoardSnapshot&` and a `StandardOutput&`, exposing `bool start()`
- [x] 6.2 Start the server from `HTTPD_DEFAULT_CONFIG()` with the task priority lowered to 3 so requests cannot delay the game or numpad tasks, reporting and returning false on failure
- [x] 6.3 Register a `GET` handler for `/` that receives the `BoardServer` through the handler's user context
- [x] 6.4 In the handler, read the snapshot, render into a file-scope `static char` buffer of 4096 bytes, and send the result with content type `text/html` and a `Cache-Control: no-store` header
- [x] 6.5 Comment that the single static buffer is safe only because the server serves one request at a time, and respond 500 if the snapshot is unavailable or the renderer returns 0
- [x] 6.6 Leave any other URI to the server's default not-found response

## 7. Wiring

- [x] 7.1 Add `board_page.cpp`, `board_snapshot.cpp`, `wifi_access_point.cpp`, and `board_server.cpp` to `SRCS` in `main/CMakeLists.txt`
- [x] 7.2 Add `esp_wifi`, `esp_http_server`, `nvs_flash`, `esp_event`, and `esp_netif` to `REQUIRES` in `main/CMakeLists.txt`
- [x] 7.3 In `main.cpp`, add the `BoardSnapshot`, `WifiAccessPoint`, and `BoardServer` objects and pass the snapshot into `Game`
- [x] 7.4 In `app_main`, create the `standard_output` task first, then start the access point and the HTTP server, then create the numpad, led, and game tasks, so startup errors are printable and the game starts whether or not the preview came up
- [x] 7.5 Build for the device and confirm no new warnings

## 8. Verification on device

- [x] 8.1 Flash and confirm the console reports the access point up and prints the address to open
- [x] 8.2 Join `esp_chessboard` from a phone with no password prompt, open the reported address, and confirm the starting position renders before any move is entered
- [x] 8.3 Confirm the board fits the phone screen without zooming, ranks and files are labelled, rank 8 is at the top, and a1 is a dark square
- [x] 8.4 Confirm White pieces read as white letters with a black outline and Black pieces as black letters with a white outline, and that both are legible on both square tints
- [x] 8.5 With the page open and untouched, enter 5254 and confirm the pawn appears on e4 within a few seconds, and that the console line and the LED blink are unchanged
- [x] 8.6 Enter 7163 and confirm `N` appears on f3 and g1 empties
- [x] 8.7 Play into a capture and confirm the captured piece is gone from the page, then castle and confirm both the king and rook have moved
- [x] 8.8 Press the reset key and confirm the page returns to the starting position and the client stays joined
- [x] 8.9 Enter a rejected move (5454 from the start) and three digits of a partial move, confirming the page does not change
- [x] 8.10 Request a path other than `/` and confirm a not-found response
- [x] 8.11 Join from a second phone and confirm both are served the current position
- [x] 8.12 Enter a full move with the page refreshing throughout and confirm no digit is dropped or reordered
- [x] 8.13 Print or log the free heap after bring-up and note it, so the Wi-Fi stack's cost is on record
