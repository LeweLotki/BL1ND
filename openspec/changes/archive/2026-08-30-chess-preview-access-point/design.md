## Context

The firmware is four FreeRTOS tasks on an ESP32 (ESP-IDF 6.0.2): `standard_output` (queue-backed serial printer), `numpad` (4×4 matrix scan on GPIO 13/12/14/27 × 26/25/33/32, polled every 20 ms), `led` (GPIO 2, command queue), and `game`. `Game` owns a `ChessGame`, which owns the only `ChessBoard` in the system; the board is reachable through `ChessGame::board()` but only from inside the game task. `ChessBoard` and `ChessGame` are deliberately free of FreeRTOS and ESP-IDF calls and are compiled into a host test target under `tests/`.

Nothing in the firmware currently touches Wi-Fi, NVS, or the network stack, so this change brings up the whole ESP-IDF networking stack for the first time. The relevant configuration is already enabled in the default `sdkconfig` (`CONFIG_ESP_WIFI_SOFTAP_SUPPORT`, `CONFIG_LWIP_DHCPS`, the `HTTPD_*` options), and the single-app partition table has an `nvs` partition, so no configuration work is expected.

Two constraints shape the design:

1. **The board lives in another task.** An HTTP handler runs on the server's own task and must not read `ChessBoard` directly — `applyMove` writes several squares, so a handler that read mid-move could render a position with a piece on two squares or on none.
2. **The preview is an accessory, not a feature the game depends on.** A player mid-tournament cares that keypresses produce notation. If Wi-Fi or the HTTP server fails to start, move entry must carry on unaffected.

## Goals / Non-Goals

**Goals:**

- An arbiter with an unmodified phone can see the position: join `esp_chessboard`, open the gateway address, read the board.
- The page is legible at arm's length on a phone with no zooming, and both colours of piece are legible on both colours of square.
- The position shown is always a position the game actually reached — never a half-applied move.
- The page updates on its own while the arbiter watches it, so entering a move on the keypad is visible without touching the phone.
- Wi-Fi and HTTP failures degrade to "no preview", never to "no game".
- Keep the HTML generation a pure function over 64 characters so it is host-testable like the rest of the chess core.

**Non-Goals:**

- Authentication, encryption, or any restriction on who may view the board. The network is open by request.
- Preventing the player from viewing their own board. That is a tournament-procedure problem, not a firmware one.
- A captive portal, mDNS/`.local` name, or any way to reach the page other than typing the gateway IP.
- Serving anything other than the position: no move list, no notation, no clocks, no PGN, no move-number display.
- Station mode, joining an existing tournament network, or internet access of any kind.
- Interaction. The page is read-only; there is no way to enter or edit a move from the phone.
- Serving two devices' boards from one page. Each device serves its own board.

## Decisions

### SoftAP, open, SSID `esp_chessboard`, on at boot

The device runs `WIFI_MODE_AP` with `authmode = WIFI_AUTH_OPEN`, an empty password, channel 1, and `max_connection = 4`. The SoftAP netif is assembled from `ESP_NETIF_DEFAULT_WIFI_AP()`, `esp_netif_new()`, `esp_netif_attach_wifi_ap()`, and `esp_wifi_set_default_wifi_ap_handlers()`. This is the error-returning equivalent of `esp_netif_create_default_wifi_ap()`, whose implementation uses assertions and `ESP_ERROR_CHECK` and therefore contradicts the requirement that preview failure must never abort the game. The default config brings its own DHCP server and puts the device at `192.168.4.1/24`, handing clients `192.168.4.2` upward. The AP is started from `app_main` at boot and stays up for the life of the session.

*Why open:* the arbiter is a stranger to the device and may be handed it mid-round. A passphrase means the passphrase has to be written on the case, at which point it is not a secret and is only an extra step. The threat model here is not confidentiality — it is a room full of people who could all watch the position and it would not matter.

*Why not on demand:* gating the AP behind a keypress means one more piece of keymap and mode state, and an arbiter who needs to check the board would have to ask the player to enable it — exactly the interaction the device is meant to avoid. Left as an open question because it is also the fair-play mitigation.

*Why the default IP:* changing it buys nothing. `192.168.4.1` is what every ESP32 SoftAP example uses and is short enough to read off a label.

### Wi-Fi failure is logged, not fatal

Every step of bring-up (`nvs_flash_init`, `esp_netif_init`, `esp_event_loop_create_default`, `esp_wifi_init`, `esp_wifi_start`, `httpd_start`) has its error checked and reported through `StandardOutput`, and `app_main` continues to create the game tasks regardless. `ESP_ERROR_CHECK` is not used, because it aborts and reboots — turning a dead access point into a dead chess computer.

`nvs_flash_init` is retried once after `nvs_flash_erase()` when it returns `ESP_ERR_NVS_NO_FREE_PAGES` or `ESP_ERR_NVS_NEW_VERSION_FOUND`, which is the standard recovery for a first boot or a changed NVS layout.

### The board crosses tasks as a 64-byte snapshot in a length-1 queue

A `BoardSnapshot` object owns a queue of length 1 whose item is a 64-byte struct of piece characters indexed `rank * 8 + file`, matching `ChessBoard`'s own `[rank][file]` layout. The game task publishes with `xQueueOverwrite` after every accepted move, after every reset, and once when the task starts; the HTTP handler reads with `xQueuePeek`, which copies the item without removing it, so any number of requests can read the same snapshot.

Publishing walks `ChessBoard::pieceAt()` over the 64 squares, so `ChessBoard` and `ChessGame` need no modification and stay FreeRTOS-free.

*Why overwrite/peek over a mutex:* `xQueueOverwrite` on a length-1 queue is a complete, atomic publish and `xQueuePeek` is a complete, atomic read; neither can observe a partial board and neither can block the other. The same idiom already appears in `NumPad`'s history, so it is not a new pattern for this codebase. A mutex around the live `ChessBoard` would work but puts the HTTP task's scheduling in the game task's critical path and lets a wedged handler hold the board.

*Why not have the handler ask the game task and wait for a reply:* a request/response pair of queues is two more objects and a timeout policy, to deliver data that changes at most a few times a minute.

*Publishing at task start* matters: it guarantees the queue is never empty, so the handler's `xQueuePeek` with a zero timeout always succeeds. Rejected moves are not republished, because the board did not change.

### The page is server-rendered HTML with a meta refresh

`GET /` returns a complete HTML document: `<meta name="viewport">` for phone scaling, `<meta http-equiv="refresh" content="2">` for self-updating, an inline `<style>`, and an 8×8 `<table>`. No JavaScript, no external assets, no second request. `Cache-Control: no-store` prevents a phone from serving a stale board back to itself. Any URI other than `/` gets the server's default 404.

*Why meta refresh over JavaScript polling:* a two-second full-page reload of a ~3 KB self-contained document is indistinguishable from a fetch-and-patch for a page that is one table, and it keeps the firmware's only client-side dependency at zero. The cost is a visible repaint every two seconds and losing scroll position — acceptable on a page that fits one screen. A `/board` endpoint returning the 64 characters plus a few lines of JS is the obvious upgrade if the repaint proves annoying.

*Why two seconds:* fast enough that an arbiter watching the phone sees a move land while the player's hand is still on the keypad, slow enough that a stray client cannot cost much. At ~3 KB a request, four clients polling is ~6 KB/s.

### Rendering: uppercase letters everywhere, colour carried by the outline

Every piece is drawn as its **uppercase** letter — `K Q R B N P` — for both sides. Side is conveyed entirely by fill and outline: White is a white letter with a black outline, Black is a black letter with a white outline. Squares keep a chessboard tint (light `#f0d9b5`, dark `#b58863`), rank 8 is at the top and file `a` on the left, with `a`–`h` labelled along the bottom and `1`–`8` down the side.

*Why not `K` versus `k`:* case is the FEN convention and is unambiguous on paper, but at phone size in a coloured cell, `P`/`p` and `K`/`k` are a squint. The outline is readable at a glance from across a table, which is the actual use.

The outline is applied twice for portability: `-webkit-text-stroke: 1px <colour>` (supported by mobile Safari and Chrome) with a four-way `text-shadow` in the same colour as a fallback for anything that ignores it. Empty squares render an empty cell.

*Why an HTML table rather than CSS grid or a drawn board:* a table is the shortest markup that survives every mobile browser without layout guesswork, and cells sized in `vmin` keep the board square on any screen without media queries.

*Why letters at all rather than piece glyphs:* the request is letters, and the Unicode chess glyphs (♔♞) depend on a font the phone may substitute badly, whereas `K` and `N` render everywhere.

### The renderer is a pure function into a caller-owned buffer

`renderBoardPage(const char cells[64], char* out, size_t out_size)` writes the whole document and returns the number of bytes written, or 0 if it would not fit. It includes nothing from FreeRTOS or ESP-IDF, so it joins `chess_board.cpp` and `chess_game.cpp` in the host test target, where the HTML can be asserted against directly.

The buffer is a 4 KB `static` array owned by the HTTP module, not a local. Keeping short CSS class names (`l`/`d` for squares, `w`/`b` for pieces) puts a cell at roughly 30 bytes, so the document lands near 3 KB; a static buffer avoids sizing the server task's stack around it. Because only one handler runs at a time on the server task, one shared buffer is sufficient — a fact worth a comment, since it stops being true if `max_open_sockets` ever grows a second worker.

### Module layout

| File | Contents | Depends on ESP-IDF? |
| --- | --- | --- |
| `board_page.hpp/.cpp` | `renderBoardPage()` — 64 chars in, HTML out | No |
| `board_snapshot.hpp/.cpp` | Length-1 queue, `publish(const ChessBoard&)` / `read(cells)` | Yes (FreeRTOS) |
| `wifi_access_point.hpp/.cpp` | NVS, netif, event loop, SoftAP config, `start()` | Yes |
| `board_server.hpp/.cpp` | `httpd` lifecycle and the `GET /` handler | Yes |
| `game.hpp/.cpp` | Gains a `BoardSnapshot&`; publishes after accepted moves and resets | Yes |
| `chess_board.*`, `chess_game.*` | Unchanged | No |
| `numpad.*`, `led.*`, `standard_output.*` | Unchanged | Yes |

`main/CMakeLists.txt` adds the four new sources and `esp_wifi`, `esp_http_server`, `nvs_flash`, `esp_event`, and `esp_netif` to `REQUIRES`. Dependency direction stays one-way: `board_server` → `board_snapshot` → `chess_board`, with `game` publishing into the snapshot and knowing nothing about HTTP.

### No new task of our own

The HTTP server runs on the task `httpd_start` creates; the AP runs on the Wi-Fi driver's tasks. The server task is configured at priority 3 — below the game, numpad, and output tasks at 5 — so a burst of requests cannot delay a keypress. The Wi-Fi driver's own tasks sit at priority 23 and are not ours to move; the numpad's 20 ms polling loop tolerates the resulting jitter because it acts on a held key rather than an instant.

## Risks / Trade-offs

- **The player can read their own board** → This is the sharpest trade-off in the change: a device built for blindfold play now broadcasts the position to any phone in the room, including the player's. Not solved in firmware. Mitigation is procedural — the arbiter has the phone, the player's phone is off the table by tournament rules anyway. If that proves insufficient, gating the AP behind an arbiter-only action is the fix; see Open Questions.
- **Two devices at one table both announce `esp_chessboard`** → A phone joining sees one network name and picks whichever is stronger, so an arbiter comparing two players' boards cannot tell which device answered. Mitigation for now: check the two boards one device at a time, powering one down, or trust that the position identifies itself. A MAC-derived suffix in the SSID is the real fix and is an open question, since the requested name is exactly `esp_chessboard`.
- **An open AP is open to everyone** → Anyone in range can join, and four clients can occupy `max_connection`, denying the arbiter a slot. Mitigation: `max_connection = 4` and the server's socket limit bound the damage, the AP has no route anywhere, and the only reachable endpoint is a read-only page. Power-cycling clears squatters.
- **The phone decides the AP is "no internet" and leaves** → Android and iOS probe for connectivity and may drop back to cellular or warn. Mitigation: the arbiter confirms "stay connected"; a captive portal would fix it properly and is a non-goal.
- **Wi-Fi's heap and current draw** → The Wi-Fi stack claims tens of kilobytes of heap and raises average current substantially, which matters if the device ever runs on a battery at a tournament. Mitigation: nothing here is memory-tight today, and power is a problem to solve when a battery exists. Worth measuring free heap after bring-up.
- **The rendered document outgrows the 4 KB buffer** → A future addition (labels, a move list) could push it over, and `renderBoardPage` returning 0 means the arbiter gets a 500 instead of a board. Mitigation: a host test asserts the rendered size of a full starting position stays well under the buffer, so the regression is caught on the desktop rather than at the table.
- **The snapshot is only as fresh as the last publish** → If a publish is ever missed, the page silently shows an old position, which is worse than showing nothing since an arbiter would trust it. Mitigation: publish from one place in the game's key handling, covering accepted moves and resets, rather than sprinkling calls around.
- **`-webkit-text-stroke` is not standard** → A browser honouring neither it nor `text-shadow` renders white pieces as white letters on a light square: invisible. Mitigation: the `text-shadow` fallback covers everything current; the tint of the squares is chosen so the black-outlined white letters stay readable on both.
- **Bringing up the network stack could destabilise a working device** → New tasks, new interrupts, and NVS access on a build whose only job so far was scanning a keypad. Mitigation: no `ESP_ERROR_CHECK` anywhere in bring-up, the game tasks start regardless, and on-device verification includes playing a full sequence with the AP up to confirm move entry and the LED are unaffected.

## Migration Plan

There is no persisted state and no OTA: the change ships by flashing. The preview is purely additive — no existing behaviour is modified, so a build with a broken access point still plays chess, and rollback is flashing the previous firmware. The one new use of persistent storage is the `nvs` partition, which Wi-Fi calibration writes to; it already exists in the single-app partition table, and erasing it (the retry path) costs nothing because the firmware stores nothing else there.

## Open Questions

- Should the access point be gated — a keypress, a hidden button, a timeout after boot — so the position is not continuously broadcast to the player? This is the fair-play question and the answer may come from tournament rules rather than from us.
- Should the SSID carry a device suffix (`esp_chessboard_a1b2`) so two devices at the same event are distinguishable, and does that break the arbiter's expectation of a single known name?
- Should the page show anything beyond the position — the move number, the last move in algebraic notation, whose turn it is? Useful to an arbiter checking a scoresheet, and each one is more that the player could read.
- If the two-second repaint proves distracting on a phone, is a `/board` endpoint plus a few lines of JavaScript worth the first client-side code in the project?
- When Bluetooth pairing lands and one device knows both players' positions, does the preview serve both boards, and does that make the per-device access point redundant?
