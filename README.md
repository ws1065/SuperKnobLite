# SuperKnobLite

> A haptic FOC smart knob built on ESP32 — with a browser-based control console
> that adds remote motor control, six switchable haptic modes, and real-time position sensing.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Platform: ESP32-PICO-D4](https://img.shields.io/badge/Platform-ESP32--PICO--D4-blue.svg)](https://www.espressif.com/en/products/socs/esp32-pico-d4)
[![Firmware: SimpleFOC](https://img.shields.io/badge/Firmware-SimpleFOC%202.2.1-orange.svg)](https://github.com/simplefoc/Arduino-FOC-Library)
[![Console: Web Serial](https://img.shields.io/badge/Console-Web%20Serial%20API-purple.svg)](https://developer.mozilla.org/en-US/docs/Web/API/Web_Serial_API)

[中文文档](README_cn.md) · [技术方案](docs/技术方案.md) · [项目说明](AGENTS.md)

---

## Overview

SuperKnobLite is a three-phase brushless gimbal motor knob driven by an ESP32-PICO-D4
with an AS5600 magnetic encoder and a DRV8313 driver. A 1 kHz FOC loop runs free on a
dedicated core, producing mechanical detent feel under your fingertips.

The stock firmware only accepts a single `{"cpu","memory"}` JSON packet over serial — used
to drive the on-device CPU monitor screen. This project extends that firmware with a
backward-compatible command protocol and ships a single-file web console that drives the
hardware directly from the browser, with no native app and no server between you and the
serial port.

## Features

### Motor playground — six haptic modes

| Mode | Feel | Physics |
|---|---|---|
| **Detent** | Mechanical click-by-click notches | Position-based torque with dead zone |
| **Spring** | Progressively heavier, snaps back on release | `torque = −2.5 · δ` |
| **Viscous** | Drag increases with speed, stops dead when still | `torque = −0.15 · ω` |
| **One-way ratchet** | Forward free, reverse firmly blocked | Asymmetric velocity damping |
| **Endstop** | Free within range, hard walls at the edges | Soft spring + stiff boundary |
| **Free spin** | One flick, many revolutions | `torque = −0.004 · ω` (≈250 s decay) |

All modes share the torque controller and retain the original runaway guard
(`|ω| > 60 rad/s → zero torque`).

### Velocity servo

Closed-loop speed control with a hard safety ceiling — useful for "hold this RPM" or
reversible spin:

```json
{"t":"haptic","cmd":"velservo","enable":true,"speed":8.0,"max":15.0}
```

Negative `speed` reverses direction. `max` and the target are both clamped to ±20 rad/s
(≈191 rpm), backed by voltage limiting and the runaway guard. Measured: 8.0 → 8.11 rad/s,
−5.0 → −4.91 rad/s, a request of 50 clamped to 20.00.

### Position sensing

The knob reports angle, detent index, and velocity at 20 Hz. The console unwraps the
encoder's 0–2π wrap-around to recover true cumulative revolutions:

```json
{"t":"pos","angle":6.469,"position":0,"velocity":0.098,"haptic":0,"remote":0}
```

### Connection testing

Reliability testing validates **echo content**, not just the presence of a reply — every
packet is checked against what was sent (tolerance ±0.05), so bit-level corruption on the
wire cannot slip through. The device handshake uses two independent fingerprints: the
ESP32 ROM boot log and the SuperKnob ASCII logo.

### Screen diagnostics

Because a backlit-but-black panel is ambiguous, the firmware provides two levels of
proof: ST7735 register read-back over MISO, and a solid-color fill that bypasses LVGL
entirely. Colors visible → panel and SPI are fine, the problem is the render layer.
Still black → hardware failure.

## Quick start

### Prerequisites

- macOS / Linux / Windows, **Chrome or Edge** (Web Serial API; Safari is not supported)
- [PlatformIO Core](https://platformio.org/) — `brew install platformio`
- SuperKnobLite hardware with a working CH343P USB-serial bridge

### Build the firmware

```bash
cd "Firmware for two knob make__"
pio run        # → .pio/build/esp32doit-devkit-v1/firmware.bin
```

`espressif32@4.1.0` compiles cleanly on Apple Silicon (79 s cold, ~7 s incremental).

### Flash the firmware

> **Use the paced wrapper.** Large writes toward the device drop bytes on this CDC link
> and fail with checksum errors. See [pitfall notes](docs/技术方案.md#六踩坑记录重要).

```bash
cd "Firmware for two knob make__"
python3 ../tools/paced_esptool.py \
  --chip esp32 -p /dev/cu.usbmodem54ED0456411 -b 115200 --no-stub \
  write-flash 0x10000 .pio/build/esp32doit-devkit-v1/firmware.bin
```

Substitute `COMx` or `/dev/ttyACM0` on other platforms. A blank flash also needs the
bootloader, partition table, and boot_app0 — prebuilt images live in
`Firmware for two knob/zeasy_upload_bin/`.

### Run the console

```bash
node tools/knob_server.mjs      # → http://127.0.0.1:8123/
```

The Node service both samples real system metrics and hosts the page. Open the URL,
click **选择设备并连接**, and pick your knob. The port should appear as
`/dev/cu.usbmodem54ED0456411` — the CH343 enumerates in USB CDC mode, so macOS names it
`usbmodem*` rather than `wchusbserial*`.

| Tab | Purpose |
|---|---|
| ① 连接测试 | Port selection, auto-handshake, reliability stress test |
| ② 设备自检 | Eight self-tests across link, firmware, FOC task, touch, screen, encoder, sensors, BLE |
| 🎡 电机玩法 | Velocity servo, six haptic modes, live position sensing |
| ③ 控制面板 | Angle-loop remote control, screen tools, serial charts, event stream |
| ④ 断开连接 | Session statistics, pre-disconnect reset, summary |

## Serial protocol

Line-delimited JSON at 115200 baud. The `t` field discriminates new commands from the
legacy packet, which remains fully supported.

| Direction | Packet | Purpose |
|---|---|---|
| PC → device | `{"cpu":16.7,"memory":90.7}` | Legacy: drives the CPU monitor page |
| Device → PC | `16.70` / `90.70` | Legacy echo, used for heartbeat verification |
| PC → device | `{"t":"haptic","cmd":"set","mode":"spring"}` | Switch haptic mode |
| PC → device | `{"t":"haptic","cmd":"velservo","enable":true,"speed":8.0,"max":15.0}` | Velocity servo |
| PC → device | `{"t":"haptic","cmd":"report","enable":true,"interval":50}` | Position reporting |
| PC → device | `{"t":"motor","cmd":"rotate","speed":3,"dir":-1}` | Angle-loop takeover |
| PC → device | `{"t":"sys","cmd":"status"}` | State query |
| Device → PC | `{"t":"ack","cmd":...,"ok":true}` | Per-command acknowledgement |
| Device → PC | `{"t":"pos",...}` | Periodic position/velocity report |

**Wire-format constraint**: bare numeric lines are reserved for heartbeat echo matching,
so all new device output must be a JSON object. See
[the protocol design notes](docs/技术方案.md#二串口协议扩展设计).

## Repository layout

```
SuperKnobLite/
├── superknob_control.html        # ★ Serial console (single file, Web Serial API)
├── tools/
│   ├── paced_esptool.py          # Throttled flashing wrapper (CDC loss workaround)
│   └── knob_server.mjs           # Zero-dependency system metrics service
├── Firmware for two knob make__/  # ★ Firmware project (recommended baseline)
├── Firmware for two knob/         # Older firmware project (2022-10)
├── docs/技术方案.md               # ★ Technical design & pitfall notes
├── super_knob_for_pc.py          # Standalone PC metrics → serial script
├── AGENTS.md                      # Hardware, firmware, and environment reference
└── LICENSE                        # MIT
```

`.pio/`, `main/`, and the Altium PCB files are excluded from version control — build
artifacts, 140 MB of Windows binaries, and 100 MB of binary design files respectively.

## Project status

| Area | Result |
|---|---|
| Link reliability | 100/100 packets verified, 100% success, RTT 62 ms avg / 108 ms P95 |
| Live metrics push | 15/15 packets verified against real CPU and memory samples |
| Six haptic modes | All acknowledged and mode IDs read back correctly |
| Velocity servo | 8.0 → 8.11 rad/s; reverse −5.0 → −4.91; 50 clamped to 20.00 |
| Position sensing | 20 Hz reporting with correct revolution unwrapping |
| Display | **Hardware failure** — registers read 0xFF, solid-color fill shows nothing |

The local screen module is dead (panel or ribbon cable). Screen-dependent features are
folded into a collapsible section so everything else stays usable. The firmware, motor,
and serial paths are unaffected.

## Hardware

| Component | Part | Notes |
|---|---|---|
| MCU | ESP32-PICO-D4 | Dual-core 240 MHz, 4 MB flash |
| Motor driver | DRV8313PWPR | 3-phase FOC, 12 V supply |
| Encoder | AS5600 | 12-bit magnetic, I²C @400 kHz, 0x36 |
| Display | ST7735 80×160 | SPI @40 MHz, BGR |
| USB-serial | CH343P | CDC mode, DTR/RTS auto-download |
| Motor | 3-phase BLDC gimbal | 7 pole pairs, voltage-mode FOC |

## Known issues

1. **Screen hardware failure** — ST7735 registers read 0xFF and a direct color fill
   produces no output; diagnosed as panel or ribbon cable.
2. **Stock firmware defect** — a JSON parse failure leaves the receive buffer uncleared,
   wedging the serial link until reset. The console detects and warns about this state.
3. **Directional CDC packet loss** — device→PC is flawless (256 KB read-back verified),
   but PC→device loses bytes on large writes. All bulk writes must be paced.
4. **`--no-stub` required** — the stub downloader cannot be uploaded over CDC mode.

## License

[MIT](LICENSE) — the most permissive of the common open-source licenses. Free use,
modification, distribution, and commercial use; the only requirement is retaining the
copyright and license notices, with no warranty.

Third-party components (Simple FOC, lvgl, TFT_eSPI, ArduinoJson, ESP32 BLE Keyboard,
Adafruit NeoPixel, PubSubClient, ESP-IDF) retain their own MIT / BSD / Apache-2.0
licenses. Full inventory in [LICENSE-THIRD-PARTY.md](LICENSE-THIRD-PARTY.md).

## Credits

Based on the original SuperKnobLite firmware by congsir, with interface code from NXP.
See [AGENTS.md](AGENTS.md) for hardware details and the [technical design document](docs/技术方案.md)
for design rationale and engineering notes.