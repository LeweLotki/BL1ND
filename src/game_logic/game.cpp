#include "game.hpp"

#include "board_snapshot.hpp"
#include "led.hpp"
#include "numpad.hpp"
#include "standard_output.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

Game::Game(
    NumPad& numpad,
    Led& led,
    StandardOutput& output,
    BoardSnapshot& board_snapshot
)
    : numpad_(numpad)
    , led_(led)
    , output_(output)
    , board_snapshot_(board_snapshot)
    , bluetooth_link_(chess_game_)
    , input_set_(xQueueCreateSet(
        NumPad::QUEUE_LENGTH + BluetoothLink::QUEUE_LENGTH
    ))
{
    if (input_set_ != nullptr) {
        if (xQueueAddToSet(numpad_.queue(), input_set_) != pdPASS
            || xQueueAddToSet(bluetooth_link_.queue(), input_set_) != pdPASS) {
            vQueueDelete(input_set_);
            input_set_ = nullptr;
        }
    }
}

bool Game::startLink()
{
    return bluetooth_link_.start();
}

void Game::run()
{
    board_snapshot_.publish(chess_game_.board());

    if (input_set_ == nullptr) {
        output_.print("Link: failed to create input queue set\n");
        while (true) {
            char key = '\0';
            if (xQueueReceive(
                    numpad_.queue(),
                    &key,
                    portMAX_DELAY
                ) == pdTRUE) {
                handleKey(key);
            }
        }
    }

    while (true) {
        const QueueSetMemberHandle_t ready = xQueueSelectFromSet(
            input_set_,
            pdMS_TO_TICKS(100)
        );
        const uint32_t now_ms =
            static_cast<uint32_t>(xTaskGetTickCount() * portTICK_PERIOD_MS);
        if (ready == numpad_.queue()) {
            char key = '\0';
            if (xQueueReceive(numpad_.queue(), &key, 0) == pdTRUE) {
                handleKey(key);
            }
        }
        else if (ready == bluetooth_link_.queue()) {
            BluetoothEvent event = {};
            if (bluetooth_link_.receive(event, 0)) {
                bluetooth_link_.process(event, now_ms);
            }
        }
        else {
            bluetooth_link_.tick(now_ms);
        }
        drainLinkOutputs();
    }
}

void Game::handleKey(char key)
{
    const uint32_t now_ms =
        static_cast<uint32_t>(xTaskGetTickCount() * portTICK_PERIOD_MS);
    if (key == KeypadLayout::HOLD_B) {
        bluetooth_link_.handleHold(now_ms);
        drainLinkOutputs();
        return;
    }

    const GameEvent event = chess_game_.handleKey(key);
    char message[96];
    if (ChessGame::formatOutput(event, message, sizeof(message))) {
        output_.print(message);
    }

    switch (event.type) {
    case GameEventType::MoveAccepted:
        led_.blinkOnce();
        board_snapshot_.publish(chess_game_.board());
        bluetooth_link_.handleLocalMove(event.move, now_ms);
        break;
    case GameEventType::MoveRejected:
        led_.blinkError();
        break;
    case GameEventType::Reset:
        board_snapshot_.publish(chess_game_.board());
        bluetooth_link_.handleLocalReset(now_ms);
        break;
    case GameEventType::None:
    case GameEventType::PromotionPending:
        break;
    }
    drainLinkOutputs();
}

void Game::drainLinkOutputs()
{
    LinkOutput output = {};
    while (bluetooth_link_.nextOutput(output)) {
        handleLinkOutput(output);
    }
}

void Game::handleLinkOutput(const LinkOutput& output)
{
    switch (output.type) {
    case LinkOutputType::PairingStarted:
        output_.print("Link: pairing for 30 seconds\n");
        break;
    case LinkOutputType::PairingFailed:
        output_.print("Link: pairing failed, no peer found\n");
        led_.blinkError();
        break;
    case LinkOutputType::Linked:
        output_.printf("Link: connected to peer token %08lx\n",
            static_cast<unsigned long>(output.token));
        led_.blinkLinked();
        break;
    case LinkOutputType::ColorAssigned:
        output_.printf(
            "Link: playing %s\n",
            output.color == Color::White ? "White" : "Black"
        );
        led_.announceColor(output.color);
        break;
    case LinkOutputType::RemoteMoveApplied: {
        char message[96];
        if (ChessGame::formatOutput(
                output.game_event,
                message,
                sizeof(message)
            )) {
            output_.print(message);
        }
        led_.blinkOnce();
        board_snapshot_.publish(chess_game_.board());
        break;
    }
    case LinkOutputType::ResetApplied:
        output_.print("Link: both boards reset\n");
        board_snapshot_.publish(chess_game_.board());
        break;
    case LinkOutputType::PositionMismatch:
        output_.printf(
            "Link: position mismatch local=%08lx peer=%08lx\n",
            static_cast<unsigned long>(output.local_hash),
            static_cast<unsigned long>(output.peer_hash)
        );
        led_.blinkError();
        break;
    case LinkOutputType::PositionCorrected:
        output_.print("Link: position corrected from peer\n");
        board_snapshot_.publish(chess_game_.board());
        break;
    case LinkOutputType::Reconciled:
        output_.print("Link: positions reconciled\n");
        board_snapshot_.publish(chess_game_.board());
        break;
    case LinkOutputType::LinkLost:
        output_.print("Link: connection lost, reconnecting\n");
        led_.blinkError();
        break;
    case LinkOutputType::ReconnectExpired:
        output_.print("Link: reconnect period expired\n");
        break;
    case LinkOutputType::Unlinked:
        output_.print("Link: disconnected by player\n");
        break;
    case LinkOutputType::Broken:
        output_.print("Link: boards could not be reconciled\n");
        led_.blinkError();
        break;
    case LinkOutputType::VersionMismatch:
        output_.print("Link: peer firmware version is incompatible\n");
        led_.blinkError();
        break;
    case LinkOutputType::TransportError:
        output_.printf(
            "Link: negotiated MTU %lu is below the required 64 bytes\n",
            static_cast<unsigned long>(output.token)
        );
        led_.blinkError();
        break;
    case LinkOutputType::StartPairing:
    case LinkOutputType::RefreshPairing:
    case LinkOutputType::ConnectToPeer:
    case LinkOutputType::WaitForPeer:
    case LinkOutputType::StopPairing:
    case LinkOutputType::Disconnect:
    case LinkOutputType::StartReconnect:
    case LinkOutputType::StopReconnect:
    case LinkOutputType::SendMessage:
        break;
    }
}
