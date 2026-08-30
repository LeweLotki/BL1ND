#include "link_protocol.hpp"

#include <cstring>

namespace {

constexpr uint8_t HELLO_SIZE = 11;
constexpr uint8_t MOVE_SIZE = 8;
constexpr uint8_t HASH_SIZE = 5;
constexpr uint8_t SYNC_SIZE = 37;
constexpr uint8_t RESET_SIZE = 4;

uint8_t pieceToNibble(char piece)
{
    switch (piece) {
    case ' ':
        return 0;
    case 'P':
        return 1;
    case 'N':
        return 2;
    case 'B':
        return 3;
    case 'R':
        return 4;
    case 'Q':
        return 5;
    case 'K':
        return 6;
    case 'p':
        return 7;
    case 'n':
        return 8;
    case 'b':
        return 9;
    case 'r':
        return 10;
    case 'q':
        return 11;
    case 'k':
        return 12;
    default:
        return 15;
    }
}

char nibbleToPiece(uint8_t value)
{
    constexpr char PIECES[] = " PNBRQKpnbrqk";
    return value < sizeof(PIECES) - 1 ? PIECES[value] : '\0';
}

void put16(uint8_t* out, uint16_t value)
{
    out[0] = static_cast<uint8_t>(value);
    out[1] = static_cast<uint8_t>(value >> 8);
}

uint16_t get16(const uint8_t* in)
{
    return static_cast<uint16_t>(
        static_cast<uint16_t>(in[0])
        | static_cast<uint16_t>(in[1]) << 8
    );
}

void put32(uint8_t* out, uint32_t value)
{
    out[0] = static_cast<uint8_t>(value);
    out[1] = static_cast<uint8_t>(value >> 8);
    out[2] = static_cast<uint8_t>(value >> 16);
    out[3] = static_cast<uint8_t>(value >> 24);
}

uint32_t get32(const uint8_t* in)
{
    return static_cast<uint32_t>(in[0])
        | static_cast<uint32_t>(in[1]) << 8
        | static_cast<uint32_t>(in[2]) << 16
        | static_cast<uint32_t>(in[3]) << 24;
}

bool knownType(uint8_t value)
{
    return value >= static_cast<uint8_t>(LinkProtocol::MessageType::Hello)
        && value <= static_cast<uint8_t>(LinkProtocol::MessageType::ResetAck);
}

bool validEnvelope(
    const uint8_t* data,
    size_t size,
    LinkProtocol::MessageType expected,
    uint8_t payload_size
)
{
    return data != nullptr
        && size == LinkProtocol::HEADER_SIZE + payload_size
        && data[0] == static_cast<uint8_t>(expected)
        && data[1] == payload_size;
}

size_t beginMessage(
    LinkProtocol::MessageType type,
    uint8_t payload_size,
    uint8_t* out,
    size_t capacity
)
{
    const size_t size = LinkProtocol::HEADER_SIZE + payload_size;
    if (out == nullptr || capacity < size) {
        return 0;
    }
    out[0] = static_cast<uint8_t>(type);
    out[1] = payload_size;
    return size;
}

size_t encodeHash(
    LinkProtocol::MessageType type,
    const LinkProtocol::HashMessage& message,
    uint8_t* out,
    size_t capacity
)
{
    const size_t size = beginMessage(type, HASH_SIZE, out, capacity);
    if (size == 0) {
        return 0;
    }
    out[2] = message.sequence;
    put32(out + 3, message.position_hash);
    return size;
}

bool decodeHash(
    LinkProtocol::MessageType type,
    const uint8_t* data,
    size_t size,
    LinkProtocol::HashMessage& message
)
{
    if (!validEnvelope(data, size, type, HASH_SIZE)) {
        return false;
    }
    message.sequence = data[2];
    message.position_hash = get32(data + 3);
    return true;
}

size_t encodeResetLike(
    LinkProtocol::MessageType type,
    const LinkProtocol::ResetMessage& message,
    uint8_t* out,
    size_t capacity
)
{
    const size_t size = beginMessage(type, RESET_SIZE, out, capacity);
    if (size == 0) {
        return 0;
    }
    put32(out + 2, message.token);
    return size;
}

bool decodeResetLike(
    LinkProtocol::MessageType type,
    const uint8_t* data,
    size_t size,
    LinkProtocol::ResetMessage& message
)
{
    if (!validEnvelope(data, size, type, RESET_SIZE)) {
        return false;
    }
    message.token = get32(data + 2);
    return true;
}

} // namespace

namespace LinkProtocol {

static_assert(MAX_MESSAGE_SIZE == 39, "protocol message buffer is too small");

void encodePosition(
    const ChessBoard& board,
    unsigned int move_number,
    uint8_t out[POSITION_SIZE]
)
{
    memset(out, 0, POSITION_SIZE);
    for (uint8_t rank = 0; rank < 8; ++rank) {
        for (uint8_t file = 0; file < 8; ++file) {
            const uint8_t index = static_cast<uint8_t>(rank * 8 + file);
            const uint8_t value = pieceToNibble(board.pieceAt({ file, rank }));
            if ((index & 1U) == 0) {
                out[index / 2] = value;
            }
            else {
                out[index / 2] |= static_cast<uint8_t>(value << 4);
            }
        }
    }

    out[32] = board.sideToMove() == Color::Black ? 1U : 0U;
    out[32] |= board.hasCastlingRight(
        Color::White,
        MoveKind::CastleKingside
    ) ? 1U << 1 : 0U;
    out[32] |= board.hasCastlingRight(
        Color::White,
        MoveKind::CastleQueenside
    ) ? 1U << 2 : 0U;
    out[32] |= board.hasCastlingRight(
        Color::Black,
        MoveKind::CastleKingside
    ) ? 1U << 3 : 0U;
    out[32] |= board.hasCastlingRight(
        Color::Black,
        MoveKind::CastleQueenside
    ) ? 1U << 4 : 0U;

    const Square en_passant = board.enPassantTarget();
    out[33] = isOnBoard(en_passant) ? en_passant.file : 15U;
    put16(out + 34, static_cast<uint16_t>(move_number));
}

bool decodePosition(
    const uint8_t in[POSITION_SIZE],
    ChessBoard& board,
    unsigned int& move_number
)
{
    char ranks[8][9] = {};
    const char* rank_pointers[8];
    for (uint8_t rank = 0; rank < 8; ++rank) {
        rank_pointers[rank] = ranks[rank];
        for (uint8_t file = 0; file < 8; ++file) {
            const uint8_t index = static_cast<uint8_t>(rank * 8 + file);
            const uint8_t packed = in[index / 2];
            const uint8_t value = (index & 1U) == 0
                ? packed & 0x0fU
                : packed >> 4;
            const char piece = nibbleToPiece(value);
            if (piece == '\0') {
                return false;
            }
            ranks[rank][file] = piece;
        }
    }

    if ((in[32] & 0xe0U) != 0 || (in[33] > 7 && in[33] != 15)) {
        return false;
    }
    board.loadPosition(
        rank_pointers,
        (in[32] & 1U) != 0 ? Color::Black : Color::White,
        (in[32] & (1U << 1)) != 0,
        (in[32] & (1U << 2)) != 0,
        (in[32] & (1U << 3)) != 0,
        (in[32] & (1U << 4)) != 0,
        in[33] == 15 ? -1 : in[33]
    );
    move_number = get16(in + 34);
    return move_number != 0;
}

uint32_t hashPosition(const uint8_t encoded[POSITION_SIZE])
{
    uint32_t hash = 2166136261U;
    for (size_t index = 0; index < POSITION_SIZE; ++index) {
        hash ^= encoded[index];
        hash *= 16777619U;
    }
    return hash;
}

uint32_t hashBoard(const ChessBoard& board, unsigned int move_number)
{
    uint8_t encoded[POSITION_SIZE];
    encodePosition(board, move_number, encoded);
    return hashPosition(encoded);
}

uint8_t encodeSquare(Square square)
{
    return isOnBoard(square)
        ? static_cast<uint8_t>(square.rank * 8 + square.file)
        : 0xffU;
}

bool decodeSquare(uint8_t encoded, Square& square)
{
    if (encoded >= 64) {
        return false;
    }
    square = {
        static_cast<uint8_t>(encoded % 8),
        static_cast<uint8_t>(encoded / 8),
    };
    return true;
}

size_t encodeHello(
    const HelloMessage& message,
    uint8_t* out,
    size_t capacity
)
{
    const size_t size = beginMessage(
        MessageType::Hello,
        HELLO_SIZE,
        out,
        capacity
    );
    if (size == 0) {
        return 0;
    }
    out[2] = message.version;
    put32(out + 3, message.token);
    put16(out + 7, message.move_number);
    put32(out + 9, message.position_hash);
    return size;
}

size_t encodeMove(
    const MoveMessage& message,
    uint8_t* out,
    size_t capacity
)
{
    const uint8_t from = encodeSquare(message.from);
    const uint8_t to = encodeSquare(message.to);
    if (from == 0xffU || to == 0xffU) {
        return 0;
    }
    const size_t size = beginMessage(
        MessageType::Move,
        MOVE_SIZE,
        out,
        capacity
    );
    if (size == 0) {
        return 0;
    }
    out[2] = message.sequence;
    out[3] = from;
    out[4] = to;
    out[5] = static_cast<uint8_t>(message.promotion);
    put32(out + 6, message.position_hash);
    return size;
}

size_t encodeAck(
    const HashMessage& message,
    uint8_t* out,
    size_t capacity
)
{
    return encodeHash(MessageType::Ack, message, out, capacity);
}

size_t encodeResyncRequest(
    const HashMessage& message,
    uint8_t* out,
    size_t capacity
)
{
    return encodeHash(
        MessageType::ResyncRequest,
        message,
        out,
        capacity
    );
}

size_t encodeSync(
    const SyncMessage& message,
    uint8_t* out,
    size_t capacity
)
{
    const size_t size = beginMessage(
        MessageType::Sync,
        SYNC_SIZE,
        out,
        capacity
    );
    if (size == 0) {
        return 0;
    }
    out[2] = message.sequence;
    memcpy(out + 3, message.position, POSITION_SIZE);
    return size;
}

size_t encodeReset(
    const ResetMessage& message,
    uint8_t* out,
    size_t capacity
)
{
    return encodeResetLike(MessageType::Reset, message, out, capacity);
}

size_t encodeResetAck(
    const ResetMessage& message,
    uint8_t* out,
    size_t capacity
)
{
    return encodeResetLike(MessageType::ResetAck, message, out, capacity);
}

ParseResult inspect(
    const uint8_t* data,
    size_t size,
    MessageType& type
)
{
    if (data == nullptr || size < HEADER_SIZE
        || data[1] > MAX_PAYLOAD_SIZE
        || size != HEADER_SIZE + data[1]) {
        return ParseResult::Invalid;
    }
    if (!knownType(data[0])) {
        return ParseResult::UnknownType;
    }
    type = static_cast<MessageType>(data[0]);
    return ParseResult::Ok;
}

bool decodeHello(const uint8_t* data, size_t size, HelloMessage& message)
{
    if (!validEnvelope(data, size, MessageType::Hello, HELLO_SIZE)) {
        return false;
    }
    message.version = data[2];
    message.token = get32(data + 3);
    message.move_number = get16(data + 7);
    message.position_hash = get32(data + 9);
    return true;
}

bool decodeMove(const uint8_t* data, size_t size, MoveMessage& message)
{
    if (!validEnvelope(data, size, MessageType::Move, MOVE_SIZE)
        || !decodeSquare(data[3], message.from)
        || !decodeSquare(data[4], message.to)) {
        return false;
    }
    message.sequence = data[2];
    message.promotion = static_cast<char>(data[5]);
    if (message.promotion != '\0'
        && message.promotion != 'Q'
        && message.promotion != 'R'
        && message.promotion != 'B'
        && message.promotion != 'N') {
        return false;
    }
    message.position_hash = get32(data + 6);
    return true;
}

bool decodeAck(const uint8_t* data, size_t size, HashMessage& message)
{
    return decodeHash(MessageType::Ack, data, size, message);
}

bool decodeResyncRequest(
    const uint8_t* data,
    size_t size,
    HashMessage& message
)
{
    return decodeHash(MessageType::ResyncRequest, data, size, message);
}

bool decodeSync(const uint8_t* data, size_t size, SyncMessage& message)
{
    if (!validEnvelope(data, size, MessageType::Sync, SYNC_SIZE)) {
        return false;
    }
    message.sequence = data[2];
    memcpy(message.position, data + 3, POSITION_SIZE);
    return true;
}

bool decodeReset(const uint8_t* data, size_t size, ResetMessage& message)
{
    return decodeResetLike(MessageType::Reset, data, size, message);
}

bool decodeResetAck(
    const uint8_t* data,
    size_t size,
    ResetMessage& message
)
{
    return decodeResetLike(MessageType::ResetAck, data, size, message);
}

} // namespace LinkProtocol
