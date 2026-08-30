#include "link_session.hpp"

#include <cstring>

namespace {

bool reached(uint32_t now, uint32_t deadline)
{
    return static_cast<int32_t>(now - deadline) >= 0;
}

} // namespace

LinkSession::LinkSession(
    ChessGame& game,
    RandomFunction random,
    void* random_context
)
    : game_(game)
    , random_(random)
    , random_context_(random_context)
    , state_(LinkState::Unlinked)
    , token_(0)
    , peer_token_(0)
    , deadline_ms_(0)
    , expected_hash_(0)
    , last_received_hash_(0)
    , sequence_(0)
    , last_received_sequence_(0)
    , retry_count_(0)
    , initiator_(false)
    , has_color_(false)
    , deliberate_disconnect_(false)
    , reconnecting_(false)
    , reset_pending_(false)
    , link_announced_(false)
    , color_(Color::White)
    , last_message_{}
    , last_message_size_(0)
    , outputs_{}
    , output_head_(0)
    , output_count_(0)
{
}

uint32_t LinkSession::randomToken()
{
    return random_ == nullptr ? 1U : random_(random_context_);
}

void LinkSession::queue(LinkOutputType type)
{
    if (output_count_ == OUTPUT_CAPACITY) {
        return;
    }
    const size_t index = (output_head_ + output_count_) % OUTPUT_CAPACITY;
    outputs_[index] = {};
    outputs_[index].type = type;
    outputs_[index].token = token_;
    ++output_count_;
}

void LinkSession::queueColor(LinkOutputType type, Color color)
{
    queue(type);
    if (output_count_ != 0) {
        const size_t index =
            (output_head_ + output_count_ - 1) % OUTPUT_CAPACITY;
        outputs_[index].color = color;
    }
}

void LinkSession::queueMessage(const uint8_t* data, size_t size)
{
    if (data == nullptr || size > sizeof(last_message_)) {
        return;
    }
    memcpy(last_message_, data, size);
    last_message_size_ = size;
    queue(LinkOutputType::SendMessage);
    if (output_count_ != 0) {
        const size_t index =
            (output_head_ + output_count_ - 1) % OUTPUT_CAPACITY;
        memcpy(outputs_[index].data, data, size);
        outputs_[index].size = size;
    }
}

void LinkSession::sendHello()
{
    uint8_t data[LinkProtocol::MAX_MESSAGE_SIZE];
    const LinkProtocol::HelloMessage hello = {
        LinkProtocol::VERSION,
        token_,
        static_cast<uint16_t>(game_.moveNumber()),
        LinkProtocol::hashBoard(game_.board(), game_.moveNumber()),
    };
    const size_t size = LinkProtocol::encodeHello(
        hello,
        data,
        sizeof(data)
    );
    queueMessage(data, size);
}

void LinkSession::sendAck(uint8_t sequence, uint32_t hash)
{
    uint8_t data[LinkProtocol::MAX_MESSAGE_SIZE];
    const size_t size = LinkProtocol::encodeAck(
        { sequence, hash },
        data,
        sizeof(data)
    );
    queueMessage(data, size);
}

void LinkSession::sendResyncRequest(uint8_t sequence, uint32_t hash)
{
    uint8_t data[LinkProtocol::MAX_MESSAGE_SIZE];
    const size_t size = LinkProtocol::encodeResyncRequest(
        { sequence, hash },
        data,
        sizeof(data)
    );
    queueMessage(data, size);
}

void LinkSession::sendSync()
{
    LinkProtocol::SyncMessage sync = { sequence_, {} };
    LinkProtocol::encodePosition(
        game_.board(),
        game_.moveNumber(),
        sync.position
    );
    uint8_t data[LinkProtocol::MAX_MESSAGE_SIZE];
    const size_t size = LinkProtocol::encodeSync(
        sync,
        data,
        sizeof(data)
    );
    expected_hash_ = LinkProtocol::hashPosition(sync.position);
    queueMessage(data, size);
}

void LinkSession::deriveColor()
{
    const bool initiator_white = ((token_ ^ peer_token_) & 1U) == 0;
    color_ = initiator_ == initiator_white ? Color::White : Color::Black;
    has_color_ = true;
    game_.setOwnedColor(color_);
    game_.setLinkAvailable(false);
}

void LinkSession::becomeReady(bool linked, bool reconciled)
{
    state_ = LinkState::Ready;
    retry_count_ = 0;
    reconnecting_ = false;
    game_.setLinkAvailable(true);
    if (!has_color_) {
        deriveColor();
    }
    if (linked || !link_announced_) {
        queue(LinkOutputType::Linked);
        queueColor(LinkOutputType::ColorAssigned, color_);
        link_announced_ = true;
    }
    if (reconciled) {
        queue(LinkOutputType::Reconciled);
    }
}

void LinkSession::onEvent(const LinkEvent& event, uint32_t now_ms)
{
    switch (event.type) {
    case LinkEventType::Hold:
        if (state_ == LinkState::Unlinked) {
            token_ = randomToken();
            has_color_ = false;
            deliberate_disconnect_ = false;
            reconnecting_ = false;
            link_announced_ = false;
            state_ = LinkState::Pairing;
            game_.setOwnedColor(game_.board().sideToMove());
            game_.setLinkAvailable(false);
            deadline_ms_ = now_ms + PAIRING_TIMEOUT_MS;
            queue(LinkOutputType::PairingStarted);
            queue(LinkOutputType::StartPairing);
        }
        else {
            deliberate_disconnect_ = true;
            state_ = LinkState::Unlinked;
            has_color_ = false;
            link_announced_ = false;
            reset_pending_ = false;
            game_.clearOwnedColor();
            queue(LinkOutputType::StopPairing);
            queue(LinkOutputType::StopReconnect);
            queue(LinkOutputType::Disconnect);
            queue(LinkOutputType::Unlinked);
        }
        break;
    case LinkEventType::PeerDiscovered:
        if (state_ != LinkState::Pairing) {
            break;
        }
        peer_token_ = event.peer_token;
        if (peer_token_ == token_) {
            token_ = randomToken();
            queue(LinkOutputType::RefreshPairing);
        }
        else if (token_ > peer_token_) {
            initiator_ = true;
            queue(LinkOutputType::ConnectToPeer);
        }
        else {
            initiator_ = false;
            queue(LinkOutputType::WaitForPeer);
        }
        break;
    case LinkEventType::ConnectedAsInitiator:
    case LinkEventType::ConnectedAsAcceptor:
        reconnecting_ = state_ == LinkState::LinkLost;
        initiator_ = event.type == LinkEventType::ConnectedAsInitiator;
        deliberate_disconnect_ = false;
        state_ = LinkState::Handshaking;
        queue(LinkOutputType::StopPairing);
        queue(LinkOutputType::StopReconnect);
        sendHello();
        break;
    case LinkEventType::Disconnected:
        if (deliberate_disconnect_
            || state_ == LinkState::Unlinked
            || state_ == LinkState::Broken) {
            deliberate_disconnect_ = false;
            break;
        }
        state_ = LinkState::LinkLost;
        reconnecting_ = true;
        deadline_ms_ = now_ms + RECONNECT_TIMEOUT_MS;
        game_.setLinkAvailable(false);
        queue(LinkOutputType::LinkLost);
        queue(LinkOutputType::StartReconnect);
        break;
    case LinkEventType::PeerMessage:
        handleMessage(event.data, event.size, now_ms);
        break;
    case LinkEventType::LocalMoveAccepted:
        if (state_ != LinkState::Ready) {
            break;
        }
        ++sequence_;
        expected_hash_ = LinkProtocol::hashBoard(
            game_.board(),
            game_.moveNumber()
        );
        {
            uint8_t data[LinkProtocol::MAX_MESSAGE_SIZE];
            const size_t size = LinkProtocol::encodeMove(
                {
                    sequence_,
                    event.move.from,
                    event.move.to,
                    event.move.promotion,
                    expected_hash_,
                },
                data,
                sizeof(data)
            );
            queueMessage(data, size);
        }
        state_ = LinkState::AwaitingAck;
        retry_count_ = 0;
        deadline_ms_ = now_ms + ACK_TIMEOUT_MS;
        break;
    case LinkEventType::LocalReset:
        if (state_ != LinkState::Ready
            && state_ != LinkState::Broken
            && state_ != LinkState::LinkLost) {
            break;
        }
        token_ = randomToken();
        reset_pending_ = true;
        game_.resetGame();
        state_ = LinkState::Handshaking;
        {
            uint8_t data[LinkProtocol::MAX_MESSAGE_SIZE];
            const size_t size = LinkProtocol::encodeReset(
                { token_ },
                data,
                sizeof(data)
            );
            queueMessage(data, size);
        }
        deadline_ms_ = now_ms + ACK_TIMEOUT_MS;
        break;
    case LinkEventType::Tick:
        handleTick(now_ms);
        break;
    }
}

void LinkSession::handleMessage(
    const uint8_t* data,
    size_t size,
    uint32_t now_ms
)
{
    LinkProtocol::MessageType type = LinkProtocol::MessageType::Hello;
    const LinkProtocol::ParseResult result =
        LinkProtocol::inspect(data, size, type);
    if (result == LinkProtocol::ParseResult::UnknownType) {
        return;
    }
    if (result != LinkProtocol::ParseResult::Ok) {
        state_ = LinkState::Broken;
        game_.setLinkAvailable(false);
        queue(LinkOutputType::Broken);
        queue(LinkOutputType::Disconnect);
        return;
    }

    switch (type) {
    case LinkProtocol::MessageType::Hello: {
        LinkProtocol::HelloMessage message = {};
        if (LinkProtocol::decodeHello(data, size, message)) {
            handleHello(message, now_ms);
        }
        break;
    }
    case LinkProtocol::MessageType::Move: {
        LinkProtocol::MoveMessage message = {};
        if (LinkProtocol::decodeMove(data, size, message)) {
            handleMove(message, now_ms);
        }
        break;
    }
    case LinkProtocol::MessageType::Ack: {
        LinkProtocol::HashMessage message = {};
        if (LinkProtocol::decodeAck(data, size, message)) {
            handleAck(message, now_ms);
        }
        break;
    }
    case LinkProtocol::MessageType::ResyncRequest: {
        LinkProtocol::HashMessage message = {};
        if (LinkProtocol::decodeResyncRequest(data, size, message)) {
            handleResyncRequest(message, now_ms);
        }
        break;
    }
    case LinkProtocol::MessageType::Sync: {
        LinkProtocol::SyncMessage message = {};
        if (LinkProtocol::decodeSync(data, size, message)) {
            handleSync(message, now_ms);
        }
        break;
    }
    case LinkProtocol::MessageType::Reset: {
        LinkProtocol::ResetMessage message = {};
        if (LinkProtocol::decodeReset(data, size, message)) {
            handleReset(message, now_ms);
        }
        break;
    }
    case LinkProtocol::MessageType::ResetAck: {
        LinkProtocol::ResetMessage message = {};
        if (LinkProtocol::decodeResetAck(data, size, message)) {
            handleResetAck(message, now_ms);
        }
        break;
    }
    }
}

void LinkSession::handleHello(
    const LinkProtocol::HelloMessage& message,
    uint32_t now_ms
)
{
    if (message.version != LinkProtocol::VERSION) {
        state_ = LinkState::Broken;
        game_.setLinkAvailable(false);
        queue(LinkOutputType::VersionMismatch);
        queue(LinkOutputType::Disconnect);
        return;
    }
    peer_token_ = message.token;
    deriveColor();
    const uint32_t local_hash = LinkProtocol::hashBoard(
        game_.board(),
        game_.moveNumber()
    );
    if (message.move_number == game_.moveNumber()
        && message.position_hash == local_hash) {
        becomeReady(!reconnecting_, reconnecting_);
        return;
    }

    state_ = LinkState::Reconciling;
    retry_count_ = 0;
    deadline_ms_ = now_ms + ACK_TIMEOUT_MS;
    if (game_.moveNumber() > message.move_number
        || (game_.moveNumber() == message.move_number && initiator_)) {
        sendSync();
    }
    else {
        sendResyncRequest(sequence_, local_hash);
    }
}

void LinkSession::handleMove(
    const LinkProtocol::MoveMessage& message,
    uint32_t now_ms
)
{
    if (state_ != LinkState::Ready) {
        sendResyncRequest(
            sequence_,
            LinkProtocol::hashBoard(game_.board(), game_.moveNumber())
        );
        return;
    }
    if (message.sequence == last_received_sequence_
        && message.position_hash == last_received_hash_) {
        sendAck(message.sequence, message.position_hash);
        return;
    }
    if (message.sequence != static_cast<uint8_t>(sequence_ + 1U)) {
        queue(LinkOutputType::PositionMismatch);
        game_.setLinkAvailable(false);
        sendResyncRequest(
            sequence_,
            LinkProtocol::hashBoard(game_.board(), game_.moveNumber())
        );
        state_ = LinkState::Resyncing;
        deadline_ms_ = now_ms + ACK_TIMEOUT_MS;
        return;
    }

    const GameEvent applied = game_.applyRemoteMove(
        message.from,
        message.to,
        message.promotion
    );
    const uint32_t local_hash = LinkProtocol::hashBoard(
        game_.board(),
        game_.moveNumber()
    );
    if (applied.type != GameEventType::MoveAccepted
        || local_hash != message.position_hash) {
        queue(LinkOutputType::PositionMismatch);
        if (output_count_ != 0) {
            const size_t index =
                (output_head_ + output_count_ - 1) % OUTPUT_CAPACITY;
            outputs_[index].local_hash = local_hash;
            outputs_[index].peer_hash = message.position_hash;
        }
        sendResyncRequest(message.sequence, local_hash);
        game_.setLinkAvailable(false);
        state_ = LinkState::Resyncing;
        retry_count_ = 0;
        deadline_ms_ = now_ms + ACK_TIMEOUT_MS;
        return;
    }

    sequence_ = message.sequence;
    last_received_sequence_ = message.sequence;
    last_received_hash_ = message.position_hash;
    sendAck(message.sequence, message.position_hash);
    queue(LinkOutputType::RemoteMoveApplied);
    if (output_count_ != 0) {
        const size_t index =
            (output_head_ + output_count_ - 1) % OUTPUT_CAPACITY;
        outputs_[index].game_event = applied;
    }
}

void LinkSession::handleAck(
    const LinkProtocol::HashMessage& message,
    uint32_t
)
{
    if (message.sequence != sequence_) {
        return;
    }
    if (message.position_hash != expected_hash_) {
        queue(LinkOutputType::PositionMismatch);
        game_.setLinkAvailable(false);
        sendSync();
        state_ = LinkState::Resyncing;
        retry_count_ = 0;
        return;
    }
    if (state_ == LinkState::AwaitingAck) {
        state_ = LinkState::Ready;
        retry_count_ = 0;
    }
    else if (state_ == LinkState::Resyncing
        || state_ == LinkState::Reconciling) {
        becomeReady(false, true);
    }
}

void LinkSession::handleResyncRequest(
    const LinkProtocol::HashMessage& message,
    uint32_t now_ms
)
{
    queue(LinkOutputType::PositionMismatch);
    game_.setLinkAvailable(false);
    if (output_count_ != 0) {
        const size_t index =
            (output_head_ + output_count_ - 1) % OUTPUT_CAPACITY;
        outputs_[index].local_hash = LinkProtocol::hashBoard(
            game_.board(),
            game_.moveNumber()
        );
        outputs_[index].peer_hash = message.position_hash;
    }
    sendSync();
    state_ = LinkState::Resyncing;
    retry_count_ = 0;
    deadline_ms_ = now_ms + ACK_TIMEOUT_MS;
}

void LinkSession::handleSync(
    const LinkProtocol::SyncMessage& message,
    uint32_t
)
{
    ChessBoard board;
    unsigned int move_number = 0;
    if (!LinkProtocol::decodePosition(
            message.position,
            board,
            move_number
        )) {
        state_ = LinkState::Broken;
        game_.setLinkAvailable(false);
        queue(LinkOutputType::Broken);
        return;
    }
    game_.adoptPosition(board, move_number);
    sequence_ = message.sequence;
    const uint32_t hash = LinkProtocol::hashBoard(
        game_.board(),
        game_.moveNumber()
    );
    sendAck(sequence_, hash);
    retry_count_ = 0;
    game_.setLinkAvailable(true);
    queue(LinkOutputType::PositionCorrected);
    becomeReady(false, reconnecting_);
}

void LinkSession::handleReset(
    const LinkProtocol::ResetMessage& message,
    uint32_t
)
{
    peer_token_ = message.token;
    token_ = randomToken();
    game_.resetGame();
    uint8_t data[LinkProtocol::MAX_MESSAGE_SIZE];
    const size_t size = LinkProtocol::encodeResetAck(
        { token_ },
        data,
        sizeof(data)
    );
    queueMessage(data, size);
    deriveColor();
    state_ = LinkState::Ready;
    reset_pending_ = false;
    game_.setLinkAvailable(true);
    queue(LinkOutputType::ResetApplied);
    queueColor(LinkOutputType::ColorAssigned, color_);
}

void LinkSession::handleResetAck(
    const LinkProtocol::ResetMessage& message,
    uint32_t
)
{
    if (!reset_pending_) {
        return;
    }
    peer_token_ = message.token;
    deriveColor();
    state_ = LinkState::Ready;
    reset_pending_ = false;
    game_.setLinkAvailable(true);
    queue(LinkOutputType::ResetApplied);
    queueColor(LinkOutputType::ColorAssigned, color_);
}

void LinkSession::handleTick(uint32_t now_ms)
{
    if (state_ == LinkState::Pairing && reached(now_ms, deadline_ms_)) {
        state_ = LinkState::Unlinked;
        game_.clearOwnedColor();
        queue(LinkOutputType::StopPairing);
        queue(LinkOutputType::PairingFailed);
        return;
    }
    if (state_ == LinkState::LinkLost && reached(now_ms, deadline_ms_)) {
        queue(LinkOutputType::StopReconnect);
        queue(LinkOutputType::ReconnectExpired);
        deadline_ms_ = UINT32_MAX;
        return;
    }
    if ((state_ != LinkState::AwaitingAck
            && state_ != LinkState::Resyncing
            && state_ != LinkState::Reconciling)
        || !reached(now_ms, deadline_ms_)) {
        return;
    }

    if (state_ == LinkState::AwaitingAck && retry_count_ == 0) {
        queueMessage(last_message_, last_message_size_);
        ++retry_count_;
        deadline_ms_ = now_ms + ACK_TIMEOUT_MS;
        return;
    }
    if (retry_count_ < 3) {
        sendSync();
        state_ = LinkState::Resyncing;
        ++retry_count_;
        deadline_ms_ = now_ms + ACK_TIMEOUT_MS;
        return;
    }
    state_ = LinkState::Broken;
    game_.setLinkAvailable(false);
    queue(LinkOutputType::Broken);
    queue(LinkOutputType::Disconnect);
}

bool LinkSession::nextOutput(LinkOutput& output)
{
    if (output_count_ == 0) {
        return false;
    }
    output = outputs_[output_head_];
    output_head_ = (output_head_ + 1) % OUTPUT_CAPACITY;
    --output_count_;
    return true;
}

LinkState LinkSession::state() const
{
    return state_;
}

uint32_t LinkSession::token() const
{
    return token_;
}

Color LinkSession::myColor() const
{
    return color_;
}

bool LinkSession::hasColor() const
{
    return has_color_;
}

bool LinkSession::canAcceptLocalMove() const
{
    return state_ == LinkState::Unlinked || state_ == LinkState::Ready;
}

bool LinkSession::isInitiator() const
{
    return initiator_;
}

LinkEvent LinkSession::holdEvent()
{
    return { LinkEventType::Hold, 0, nullptr, 0, {} };
}

LinkEvent LinkSession::discoveredEvent(uint32_t peer_token)
{
    return {
        LinkEventType::PeerDiscovered,
        peer_token,
        nullptr,
        0,
        {},
    };
}

LinkEvent LinkSession::connectedEvent(bool initiator)
{
    return {
        initiator
            ? LinkEventType::ConnectedAsInitiator
            : LinkEventType::ConnectedAsAcceptor,
        0,
        nullptr,
        0,
        {},
    };
}

LinkEvent LinkSession::disconnectedEvent()
{
    return { LinkEventType::Disconnected, 0, nullptr, 0, {} };
}

LinkEvent LinkSession::messageEvent(const uint8_t* data, size_t size)
{
    return { LinkEventType::PeerMessage, 0, data, size, {} };
}

LinkEvent LinkSession::localMoveEvent(const Move& move)
{
    return { LinkEventType::LocalMoveAccepted, 0, nullptr, 0, move };
}

LinkEvent LinkSession::localResetEvent()
{
    return { LinkEventType::LocalReset, 0, nullptr, 0, {} };
}

LinkEvent LinkSession::tickEvent()
{
    return { LinkEventType::Tick, 0, nullptr, 0, {} };
}
