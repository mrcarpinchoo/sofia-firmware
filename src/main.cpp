// SOFIA Maestro: Wi-Fi access point + UDP command server.
//
// Answers every valid command from the PC with exactly one ACK or NACK
// (docs/PROTOCOL.md). Only PING is implemented at this stage; any other
// command gets NACK UNKNOWN_CMD. Every frame is logged on the serial port.

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <string.h>

#include "config.h"
#include "protocol.h"

using protocol::Command;
using protocol::DecodeResult;
using protocol::ErrorCode;
using protocol::Frame;
using protocol::FrameType;

namespace {

WiFiUDP udp;

// Last response sent. If the PC retransmits the same SEQ because our
// response was lost, we resend this instead of executing the command again.
struct CachedResponse {
  bool valid = false;
  IPAddress ip;
  uint16_t port = 0;
  uint16_t seq = 0;
  uint8_t bytes[protocol::kMaxFrameSize] = {};
  size_t size = 0;
};
CachedResponse lastResponse;

Frame makeResponse(const Frame& request, FrameType type) {
  Frame response;
  response.type = type;
  response.seq = request.seq;
  response.command = request.command;
  return response;
}

Frame makeNack(const Frame& request, ErrorCode error) {
  Frame response = makeResponse(request, FrameType::kNack);
  response.length = 1;
  response.payload[0] = static_cast<uint8_t>(error);
  return response;
}

void writeU32(uint8_t* p, uint32_t value) {
  for (int i = 0; i < 4; ++i) p[i] = static_cast<uint8_t>(value >> (8 * i));
}

// PING: no payload. ACK payload = firmware major, minor, patch, uptime (ms, u32).
Frame handlePing(const Frame& request) {
  if (request.length != 0) return makeNack(request, ErrorCode::kBadLength);

  Frame response = makeResponse(request, FrameType::kAck);
  response.length = 7;
  response.payload[0] = config::kFirmwareMajor;
  response.payload[1] = config::kFirmwareMinor;
  response.payload[2] = config::kFirmwarePatch;
  writeU32(response.payload + 3, millis());
  return response;
}

Frame handleCommand(const Frame& request) {
  if (request.version != protocol::kVersion) {
    return makeNack(request, ErrorCode::kUnsupportedVersion);
  }
  switch (static_cast<Command>(request.command)) {
    case Command::kPing:
      return handlePing(request);
    default:
      return makeNack(request, ErrorCode::kUnknownCommand);
  }
}

const char* responseName(const Frame& response) {
  return response.type == FrameType::kAck ? "ACK" : "NACK";
}

bool isRetransmission(const IPAddress& ip, uint16_t port, uint16_t seq) {
  return lastResponse.valid && lastResponse.ip == ip && lastResponse.port == port &&
         lastResponse.seq == seq;
}

void send(const IPAddress& ip, uint16_t port, const uint8_t* bytes, size_t size) {
  udp.beginPacket(ip, port);
  udp.write(bytes, size);
  udp.endPacket();
}

void handleDatagram() {
  const int packetSize = udp.parsePacket();
  if (packetSize <= 0) return;

  const IPAddress ip = udp.remoteIP();
  const uint16_t port = udp.remotePort();
  uint8_t buffer[protocol::kMaxFrameSize];
  const int read = udp.read(buffer, sizeof(buffer));

  Frame request;
  DecodeResult result = DecodeResult::kBadLength;  // datagram larger than any frame
  if (read == packetSize) {
    result = protocol::decode(buffer, static_cast<size_t>(read), request);
  }
  if (result != DecodeResult::kOk) {
    Serial.printf("RX %d bytes from %s:%u discarded: %s\n", packetSize,
                  ip.toString().c_str(), port, protocol::toString(result));
    return;
  }
  if (request.type != FrameType::kCommand) {
    Serial.printf("RX type 0x%02X from %s:%u discarded: not a command\n",
                  static_cast<uint8_t>(request.type), ip.toString().c_str(), port);
    return;
  }

  if (isRetransmission(ip, port, request.seq)) {
    send(ip, port, lastResponse.bytes, lastResponse.size);
    Serial.printf("RX CMD 0x%02X seq=%u from %s:%u (retransmission) -> cached response\n",
                  request.command, request.seq, ip.toString().c_str(), port);
    return;
  }

  const Frame response = handleCommand(request);
  lastResponse.valid = true;
  lastResponse.ip = ip;
  lastResponse.port = port;
  lastResponse.seq = request.seq;
  lastResponse.size = protocol::encode(response, lastResponse.bytes, sizeof(lastResponse.bytes));
  send(ip, port, lastResponse.bytes, lastResponse.size);

  Serial.printf("RX CMD 0x%02X seq=%u from %s:%u -> %s", request.command, request.seq,
                ip.toString().c_str(), port, responseName(response));
  if (response.type == FrameType::kNack) {
    Serial.printf(" (error 0x%02X)", response.payload[0]);
  }
  Serial.println();
}

}  // namespace

void setup() {
  Serial.begin(config::kSerialBaud);
  delay(500);
  Serial.printf("\nSOFIA Maestro firmware %u.%u.%u, protocol v%u\n", config::kFirmwareMajor,
                config::kFirmwareMinor, config::kFirmwarePatch, protocol::kVersion);

  const IPAddress ip(config::kApIp[0], config::kApIp[1], config::kApIp[2], config::kApIp[3]);
  const IPAddress netmask(config::kApNetmask[0], config::kApNetmask[1], config::kApNetmask[2],
                          config::kApNetmask[3]);
  Serial.println("Starting Wi-Fi...");
  WiFi.mode(WIFI_AP);
  WiFi.setTxPower(static_cast<wifi_power_t>(config::kTxPowerQuarterDbm));
  if (!WiFi.softAPConfig(ip, ip, netmask) ||
      !WiFi.softAP(config::kApSsid, config::kApPassword, config::kApChannel)) {
    Serial.println("Failed to start the Wi-Fi access point; restarting in 5 s");
    delay(5000);
    ESP.restart();
  }

  udp.begin(config::kUdpPort);
  Serial.printf("Access point \"%s\" up, listening on %s:%u (UDP)\n", config::kApSsid,
                WiFi.softAPIP().toString().c_str(), config::kUdpPort);
}

void loop() {
  handleDatagram();
}
