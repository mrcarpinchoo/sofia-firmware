#include "protocol.h"

#include <string.h>

namespace protocol {

namespace {

// Byte offsets inside a frame
constexpr size_t kSofOffset = 0;
constexpr size_t kVersionOffset = 1;
constexpr size_t kTypeOffset = 2;
constexpr size_t kSeqOffset = 3;
constexpr size_t kCommandOffset = 5;
constexpr size_t kLengthOffset = 6;
constexpr size_t kPayloadOffset = kHeaderSize;

uint16_t readU16(const uint8_t* p) {
  return static_cast<uint16_t>(p[0] | (p[1] << 8));
}

void writeU16(uint8_t* p, uint16_t value) {
  p[0] = static_cast<uint8_t>(value & 0xFF);
  p[1] = static_cast<uint8_t>(value >> 8);
}

}  // namespace

uint16_t crc16(const uint8_t* data, size_t size) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < size; ++i) {
    crc ^= static_cast<uint16_t>(data[i]) << 8;
    for (int bit = 0; bit < 8; ++bit) {
      crc = (crc & 0x8000) ? static_cast<uint16_t>((crc << 1) ^ 0x1021)
                           : static_cast<uint16_t>(crc << 1);
    }
  }
  return crc;
}

DecodeResult decode(const uint8_t* buffer, size_t size, Frame& frame) {
  if (size < kMinFrameSize) return DecodeResult::kTooShort;
  if (buffer[kSofOffset] != kStartOfFrame) return DecodeResult::kBadStart;

  const uint8_t length = buffer[kLengthOffset];
  if (length > kMaxPayload || size != kMinFrameSize + length) {
    return DecodeResult::kBadLength;
  }

  const size_t crcOffset = kPayloadOffset + length;
  const uint16_t expected = crc16(buffer + kVersionOffset, crcOffset - kVersionOffset);
  if (readU16(buffer + crcOffset) != expected) return DecodeResult::kBadCrc;

  frame.version = buffer[kVersionOffset];
  frame.type = static_cast<FrameType>(buffer[kTypeOffset]);
  frame.seq = readU16(buffer + kSeqOffset);
  frame.command = buffer[kCommandOffset];
  frame.length = length;
  memcpy(frame.payload, buffer + kPayloadOffset, length);
  return DecodeResult::kOk;
}

size_t encode(const Frame& frame, uint8_t* buffer, size_t capacity) {
  const size_t size = kMinFrameSize + frame.length;
  if (frame.length > kMaxPayload || size > capacity) return 0;

  buffer[kSofOffset] = kStartOfFrame;
  buffer[kVersionOffset] = frame.version;
  buffer[kTypeOffset] = static_cast<uint8_t>(frame.type);
  writeU16(buffer + kSeqOffset, frame.seq);
  buffer[kCommandOffset] = frame.command;
  buffer[kLengthOffset] = frame.length;
  memcpy(buffer + kPayloadOffset, frame.payload, frame.length);

  const size_t crcOffset = kPayloadOffset + frame.length;
  writeU16(buffer + crcOffset, crc16(buffer + kVersionOffset, crcOffset - kVersionOffset));
  return size;
}

const char* toString(DecodeResult result) {
  switch (result) {
    case DecodeResult::kOk: return "ok";
    case DecodeResult::kTooShort: return "too short";
    case DecodeResult::kBadStart: return "bad start-of-frame";
    case DecodeResult::kBadLength: return "length mismatch";
    case DecodeResult::kBadCrc: return "bad CRC";
  }
  return "unknown";
}

}  // namespace protocol
