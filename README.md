# sofia-firmware

Firmware for the SOFIA Maestro, the ESP32 that receives commands from the
PC over Wi‑Fi. The ESP32 creates its own Wi‑Fi network, listens for UDP
commands and answers each one with an ACK or a NACK.

At this stage it implements the `PING` test command; any other command is
answered with `NACK UNKNOWN_CMD`. The protocol is specified in
[`docs/PROTOCOL.md`](docs/PROTOCOL.md). The PC side lives in
`sofia-tracker/comms/`.

## Requirements

- Python 3.12 (for PlatformIO).
- An ESP32 board with the ESP‑WROOM‑32 module:

| PlatformIO env | Board |
|---|---|
| `esp32dev` (default) | Generic ESP32‑WROOM‑32 dev board (used for testing) |
| `nodemcu-32s` | NodeMCU‑32S (the board planned for SOFIA) |

## Setup

PlatformIO is installed in a project virtual environment:

```bash
python3.12 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

The first build downloads the ESP32 toolchain and Arduino framework
(several hundred MB) into `.platformio/` inside this folder (gitignored).

On Linux, allow your user to access the board's serial port, then log out
and back in:

```bash
sudo usermod -aG dialout $USER
```

## Build and flash

Activate the environment each time you open a new terminal
(`source .venv/bin/activate`), then:

```bash
pio run -e esp32dev                         # build
pio run -e esp32dev -t upload               # build and flash
pio device monitor                          # serial log (Ctrl+C to exit)
```

Use `-e nodemcu-32s` for the NodeMCU board. The board's USB port is used
for both flashing and the serial log. If the upload doesn't start, hold the
**BOOT** button while it says `Connecting...`.

On boot, the serial log shows:

```
SOFIA Maestro firmware 0.1.0, protocol v1
Access point "SOFIA-Maestro" up, listening on 192.168.4.1:4210 (UDP)
```

## Test the link

1. Connect the PC to the Wi‑Fi network **`SOFIA-Maestro`** (password
   `sofia-maestro`, set in `include/config.h`).
2. From `sofia-tracker`, send PINGs:

   ```bash
   python -m comms ping
   ```

   ```
   PING SOFIA Maestro at 192.168.4.1:4210 (UDP)
   ACK seq=40213 firmware=0.1.0 uptime=42.3 s rtt=6.1 ms
   ...
   ```

3. The serial monitor logs each command and the response sent:

   ```
   RX CMD 0x01 seq=40213 from 192.168.4.2:51034 -> ACK
   ```

## Files

- `src/main.cpp`: Wi‑Fi access point, UDP loop, command handling and the
  duplicate‑detection cache.
- `src/protocol.h`, `src/protocol.cpp`: frame encoding/decoding and CRC.
  They don't depend on Arduino, so they also compile on a PC.
- `include/config.h`: network settings (SSID, password, IP, port) and the
  firmware version.
- `platformio.ini`: board environments (`esp32dev`, `nodemcu-32s`).
- `docs/PROTOCOL.md`: the communication standard.

## Troubleshooting

- **Upload fails with "permission denied" on `/dev/ttyUSB0` or
  `/dev/ttyACM0`.** Your user isn't in the `dialout` group yet (see
  Setup), or you haven't logged out and back in since adding it.
- **`python -m comms ping` reports "no response".** Check that the PC is
  connected to `SOFIA-Maestro` (not another network) and that the serial
  log shows the access point is up.
