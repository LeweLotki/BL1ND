#pragma once

#include "chess_board.hpp"

#include <cstddef>
#include <cstdint>

namespace LinkProtocol {

constexpr uint8_t VERSION = 1;
constexpr size_t POSITION_SIZE = 36;
constexpr size_t HEADER_SIZE = 2;
constexpr size_t MAX_PAYLOAD_SIZE = 37;
constexpr size_t MAX_MESSAGE_SIZE = HEADER_SIZE + MAX_PAYLOAD_SIZE;

enum class MessageType : uint8_t {
    Hello = 1,
    Move = 2,
    Ack = 3,
    ResyncRequest = 4,
    Sync = 5,
    Reset = 6,
    ResetAck = 7,
};

enum class ParseResult {
    Ok,
    UnknownType,
    Invalid,
};

struct HelloMessage {
    uint8_t version;
    uint32_t token;
    uint16_t move_number;
    uint32_t position_hash;
};

struct MoveMessage {
    uint8_t sequence;
    Square from;
    Square to;
    char promotion;
    uint32_t position_hash;
};

struct HashMessage {
    uint8_t sequence;
    uint32_t position_hash;
};

struct SyncMessage {
    uint8_t sequence;
    uint8_t position[POSITION_SIZE];
};

struct ResetMessage {
    uint32_t token;
};

void encodePosition(
    const ChessBoard& board,
    unsigned int move_number,
    uint8_t out[POSITION_SIZE]
);
bool decodePosition(
    const uint8_t in[POSITION_SIZE],
    ChessBoard& board,
    unsigned int& move_number
);
uint32_t hashPosition(const uint8_t encoded[POSITION_SIZE]);
uint32_t hashBoard(const ChessBoard& board, unsigned int move_number);

uint8_t encodeSquare(Square square);
bool decodeSquare(uint8_t encoded, Square& square);

size_t encodeHello(
    const HelloMessage& message,
    uint8_t* out,
    size_t capacity
);
size_t encodeMove(
    const MoveMessage& message,
    uint8_t* out,
    size_t capacity
);
size_t encodeAck(
    const HashMessage& message,
    uint8_t* out,
    size_t capacity
);
size_t encodeResyncRequest(
    const HashMessage& message,
    uint8_t* out,
    size_t capacity
);
size_t encodeSync(
    const SyncMessage& message,
    uint8_t* out,
    size_t capacity
);
size_t encodeReset(
    const ResetMessage& message,
    uint8_t* out,
    size_t capacity
);
size_t encodeResetAck(
    const ResetMessage& message,
    uint8_t* out,
    size_t capacity
);

ParseResult inspect(
    const uint8_t* data,
    size_t size,
    MessageType& type
);
bool decodeHello(const uint8_t* data, size_t size, HelloMessage& message);
bool decodeMove(const uint8_t* data, size_t size, MoveMessage& message);
bool decodeAck(const uint8_t* data, size_t size, HashMessage& message);
bool decodeResyncRequest(
    const uint8_t* data,
    size_t size,
    HashMessage& message
);
bool decodeSync(const uint8_t* data, size_t size, SyncMessage& message);
bool decodeReset(const uint8_t* data, size_t size, ResetMessage& message);
bool decodeResetAck(
    const uint8_t* data,
    size_t size,
    ResetMessage& message
);

} // namespace LinkProtocol
