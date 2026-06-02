# Smart Fan Controller (ESP32-C3)

Firmware for a 3-speed fan driven through a 3-channel relay module, with
Apple HomeKit (HomeSpan), Home Assistant (MQTT Discovery via ArduinoHA),
a physical button, OTA updates and NVS persistence.

## Hardware

| Function     | GPIO  |
| ------------ | ----- |
| Low speed    | GPIO5 |
| Medium speed | GPIO4 |
| High speed   | GPIO3 |
| User button  | GPIO6 |

* Relays are **active LOW**, only one ever energized at a time.
* Speed changes are **break-before-make**: all relays off → 200 ms → target on
  (non-blocking, handled in `RelayController`).
* Button: `GPIO6 — button — GND`, `INPUT_PULLUP`.

## Modules (`src/`)

| File | Responsibility |
| ---- | -------------- |
| `Config.h`          | Pins, constants, enums |
| `SettingsManager`   | NVS persistence (speed, name, MQTT host/port) |
| `RelayController`   | Relay drive + break-before-make timing |
| `FanController`     | State machine — single source of truth |
| `ButtonManager`     | OneButton (short = cycle, long >10 s = factory reset) |
| `HomeKitManager`    | HomeSpan (HomeKit + WiFi provisioning + OTA) |
| `MQTTManager`       | ArduinoHA (Home Assistant MQTT Discovery) |
| `main.cpp`          | Wiring + non-blocking loop |

## Behaviour

* **Boot is always OFF.** The last speed (1–3) is restored from NVS for the
  slider position, but power state is never persisted.
* Button short-press cycles `OFF → LOW → MEDIUM → HIGH → OFF`.
* Every change (button / HomeKit / MQTT) flows through `FanController`, which
  updates the relay, persists the speed, and refreshes HomeKit + MQTT so all
  interfaces stay in sync.
* Works fully **offline**: button control keeps working with no WiFi/MQTT/HA.
  MQTT only runs while WiFi is up so a missing network never blocks the loop.

## Build / flash

PlatformIO:

```bash
pio run                 # build
pio run -t upload       # flash over USB (first time)
pio device monitor      # serial @ 115200
```

OTA (after the first USB flash): uncomment the `upload_protocol = espota`
block in `platformio.ini`, set the device IP, then `pio run -t upload`.
OTA password: `fan-ota-2024` (see `Config.h`).

### Toolchain notes

* Uses the **pioarduino** platform `53.03.13` (Arduino core 3.1.3).
* **HomeSpan is pinned to `2.1.2`** — it is the last release that supports
  core < 3.3.0. Newer pioarduino releases (55.x) that ship core ≥ 3.3.0 use
  `.tar.xz` packages and require a Python built with LZMA support.
* Partition table `min_spiffs.csv` (1.9 MB app ×2, **OTA kept**) — the default
  1.25 MB app slot is too small for HomeSpan + ArduinoHA + WiFi.
* If `pio` reports *"Python's lzma module is unavailable"*, install xz:
  `brew install xz` (provides the `liblzma` the bundled Python links against).

## Provisioning (deviation from the spec)

The spec asks for **BLE onboarding through Apple Home**. HomeSpan — the
mandated HomeKit library — implements HAP over **WiFi/IP only** and has no
BLE/WAC transport, so true Apple-BLE provisioning is not possible with it.
Instead, HomeSpan's **built-in WiFi provisioning** is used:

1. On first boot (no WiFi stored) HomeSpan starts its setup Access Point.
2. Connect to it and enter your WiFi credentials.
3. Add the accessory in Apple Home using setup code **`466-37-726`**.

The provisioning step is isolated in `HomeKitManager`, so a dedicated BLE
provisioning module could be swapped in later without touching the rest.

## Factory reset

Hold the button **> 10 s**: clears NVS (speed/name/MQTT), deletes HomeKit
pairing + WiFi credentials, then reboots.
