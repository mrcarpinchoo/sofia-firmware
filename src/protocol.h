// SOFIA PC <-> Maestro protocol, version 1 (see docs/PROTOCOL.md).
//
// Frame layout (multi-byte fields are little-endian):
//
//   SOF | VER | TYPE | SEQ (2) | CMD | LEN | PAYLOAD (LEN) | CRC16 (2)
//
// The CRC is CRC-16/CCITT-FALSE over VER .. end of PAYLOAD.
//
// No Arduino dependencies, so it also compiles on a PC.
#pragma once

#include <stddef.h>
#include <stdint.h>

namespace protocol {

constexpr uint8_t kStartOfFrame = 0xA5;
constexpr uint8_t kVersion = 0x01;

constexpr size_t kHeaderSize = 7;  // SOF, VER, TYPE, SEQ (2), CMD, LEN
constexpr size_t kCrcSize = 2;
constexpr size_t kMaxPayload = 64;
constexpr size_t kMinFrameSize = kHeaderSize + kCrcSize;
constexpr size_t kMaxFrameSize = kMinFrameSize + kMaxPayload;

enum class FrameType : uint8_t {
  kCommand = 0x01,  // PC -> Maestro
  kAck = 0x02,      // Maestro -> PC: command accepted and executed
  kNack = 0x03,     // Maestro -> PC: command rejected, payload = ErrorCode
};

enum class Command : uint8_t {
  kPing = 0x01,
  // Reserved for later stages; answered with kUnknownCommand for now.
  kGetStatus = 0x02,
  kGesture = 0x10,
  kSetTarget = 0x11,
};

enum class ErrorCode : uint8_t {
  kUnknownCommand = 0x01,
  kBadLength = 0x02,
  kBadPayload = 0x03,
  kUnsupportedVersion = 0x04,
  kBusy = 0x05,
  kInternal = 0x06,
};

struct Frame {
  uint8_t version = kVersion;
  FrameType type = FrameType::kCommand;
  uint16_t seq = 0;
  uint8_t command = 0;
  uint8_t length = 0;
  uint8_t payload[kMaxPayload] = {};
};

// Why a datagram couldn't be decoded. Such datagrams are discarded without
// a response, since none of their fields can be trusted.
enum class DecodeResult {
  kOk,
  kTooShort,
  kBadStart,
  kBadLength,
  kBadCrc,
};

uint16_t crc16(const uint8_t* data, size_t size);

DecodeResult decode(const uint8_t* buffer, size_t size, Frame& frame);

// Writes the frame into buffer. Returns the number of bytes written, or 0
// if the payload is too long or the buffer too small.
size_t encode(const Frame& frame, uint8_t* buffer, size_t capacity);

const char* toString(DecodeResult result);

}  // namespace protocol
