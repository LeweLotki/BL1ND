#pragma once

#include "chess_game.hpp"
#include "link_protocol.hpp"

#include <cstddef>
#include <cstdint>

enum class LinkState {
    Unlinked,
    Pairing,
    Handshaking,
    Ready,
    AwaitingAck,
    Resyncing,
    Reconciling,
    LinkLost,
    Broken,
};

enum class LinkEventType {
    Hold,
    PeerDiscovered,
    ConnectedAsInitiator,
    ConnectedAsAcceptor,
    Disconnected,
    PeerMessage,
    LocalMoveAccepted,
    LocalReset,
    Tick,
};

struct LinkEvent {
    LinkEventType type;
    uint32_t peer_token;
    const uint8_t* data;
    size_t size;
    Move move;
};

enum class LinkOutputType {
    StartPairing,
    RefreshPairing,
    ConnectToPeer,
    WaitForPeer,
    StopPairing,
    Disconnect,
    StartReconnect,
    StopReconnect,
    SendMessage,
    PairingStarted,
    PairingFailed,
    Linked,
    ColorAssigned,
    RemoteMoveApplied,
    ResetApplied,
    PositionMismatch,
    PositionCorrected,
    Reconciled,
    LinkLost,
    ReconnectExpired,
    Unlinked,
    Broken,
    VersionMismatch,
    TransportError,
};

struct LinkOutput {
    LinkOutputType type;
    uint8_t data[LinkProtocol::MAX_MESSAGE_SIZE];
    size_t size;
    uint32_t token;
    uint32_t local_hash;
    uint32_t peer_hash;
    Color color;
    GameEvent game_event;
};

class LinkSession {
public:
    using RandomFunction = uint32_t (*)(void* context);

    LinkSession(
        ChessGame& game,
        RandomFunction random,
        void* random_context = nullptr
    );

    void onEvent(const LinkEvent& event, uint32_t now_ms);
    bool nextOutput(LinkOutput& output);

    LinkState state() const;
    uint32_t token() const;
    Color myColor() const;
    bool hasColor() const;
    bool canAcceptLocalMove() const;
    bool isInitiator() const;

    static LinkEvent holdEvent();
    static LinkEvent discoveredEvent(uint32_t peer_token);
    static LinkEvent connectedEvent(bool initiator);
    static LinkEvent disconnectedEvent();
    static LinkEvent messageEvent(const uint8_t* data, size_t size);
    static LinkEvent localMoveEvent(const Move& move);
    static LinkEvent localResetEvent();
    static LinkEvent tickEvent();

private:
    static constexpr size_t OUTPUT_CAPACITY = 16;
    static constexpr uint32_t PAIRING_TIMEOUT_MS = 30000;
    static constexpr uint32_t ACK_TIMEOUT_MS = 2000;
    static constexpr uint32_t RECONNECT_TIMEOUT_MS = 30000;

    uint32_t randomToken();
    void queue(LinkOutputType type);
    void queueColor(LinkOutputType type, Color color);
    void queueMessage(const uint8_t* data, size_t size);
    void sendHello();
    void sendAck(uint8_t sequence, uint32_t hash);
    void sendResyncRequest(uint8_t sequence, uint32_t hash);
    void sendSync();
    void becomeReady(bool linked, bool reconciled);
    void deriveColor();
    void handleMessage(const uint8_t* data, size_t size, uint32_t now_ms);
    void handleHello(
        const LinkProtocol::HelloMessage& message,
        uint32_t now_ms
    );
    void handleMove(
        const LinkProtocol::MoveMessage& message,
        uint32_t now_ms
    );
    void handleAck(
        const LinkProtocol::HashMessage& message,
        uint32_t now_ms
    );
    void handleResyncRequest(
        const LinkProtocol::HashMessage& message,
        uint32_t now_ms
    );
    void handleSync(
        const LinkProtocol::SyncMessage& message,
        uint32_t now_ms
    );
    void handleReset(
        const LinkProtocol::ResetMessage& message,
        uint32_t now_ms
    );
    void handleResetAck(
        const LinkProtocol::ResetMessage& message,
        uint32_t now_ms
    );
    void handleTick(uint32_t now_ms);

    ChessGame& game_;
    RandomFunction random_;
    void* random_context_;
    LinkState state_;
    uint32_t token_;
    uint32_t peer_token_;
    uint32_t deadline_ms_;
    uint32_t expected_hash_;
    uint32_t last_received_hash_;
    uint8_t sequence_;
    uint8_t last_received_sequence_;
    uint8_t retry_count_;
    bool initiator_;
    bool has_color_;
    bool deliberate_disconnect_;
    bool reconnecting_;
    bool reset_pending_;
    bool link_announced_;
    Color color_;
    uint8_t last_message_[LinkProtocol::MAX_MESSAGE_SIZE];
    size_t last_message_size_;
    LinkOutput outputs_[OUTPUT_CAPACITY];
    size_t output_head_;
    size_t output_count_;
};
