// Network and firmware settings for the SOFIA Maestro.
#pragma once

#include <stdint.h>

namespace config {

// Firmware version, reported in the PING acknowledgement
constexpr uint8_t kFirmwareMajor = 0;
constexpr uint8_t kFirmwareMinor = 1;
constexpr uint8_t kFirmwarePatch = 0;

// Wi-Fi access point created by the ESP32. WPA2 needs at least 8 characters.
constexpr char kApSsid[] = "SOFIA-Maestro";
constexpr char kApPassword[] = "sofia-maestro";
constexpr uint8_t kApChannel = 6;
constexpr uint8_t kApIp[4] = {192, 168, 4, 1};
constexpr uint8_t kApNetmask[4] = {255, 255, 255, 0};

// UDP port the Maestro listens on
constexpr uint16_t kUdpPort = 4210;

constexpr uint32_t kSerialBaud = 115200;

}  // namespace config
