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
* Speed requests are **debounced** (150 ms): the running speed is held until a
  new request settles, so command bursts (slider drags, fast taps) cause one
  transition instead of contact chatter.
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
| `HomeKitManager`    | HomeSpan (HomeKit + OTA), connects with portal creds |
| `MQTTManager`       | ArduinoHA (Home Assistant MQTT Discovery) |
| `ConfigPortal`      | First-run captive portal (WiFi / name / MQTT) |
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

## Provisioning — captive portal

On first boot (or after a factory reset) the device has no WiFi credentials
and starts a **configuration captive portal**:

1. Join the WiFi network **`Fan-Controller-XXXX`** (open, no password).
2. Your phone/laptop auto-opens the setup page (or browse to
   `http://192.168.4.1`). The page lets you set:
   * WiFi network + password (with a live scan list),
   * device name,
   * MQTT broker IP + port.
3. Save → the device reboots and connects to your WiFi.
4. Add the accessory in Apple Home using setup code **`466-37-726`**.

The captured WiFi credentials are handed to HomeSpan via
`homeSpan.setWifiCredentials()`, so HomeSpan connects directly and never runs
its own setup AP. The portal lives in `ConfigPortal` and HomeSpan/ArduinoHA are
not started while it is active.

### Note on the spec's BLE onboarding

The spec asks for **BLE onboarding through Apple Home**. HomeSpan implements
HAP over **WiFi/IP only** (no BLE/WAC transport), so true Apple-BLE
provisioning isn't possible with the mandated library. The captive portal is
the functional replacement and additionally configures the device name and
MQTT broker, which Apple's BLE onboarding could not.

## Home Assistant / MQTT broker

The broker host, port, **username and password** are set in the captive portal
during setup. To change them later without re-provisioning, use the serial
monitor (115200):

```
@M <host> [port] [user] [pass]
@M 192.168.0.10 1883 fanuser s3cret
```

Username/password are optional — leave them blank (portal) or omit them
(`@M`) for an anonymous broker connection.

* **Use an IP address, not a `.local` name.** The ESP32 lwIP resolver does not
  do mDNS, so `broker.local` (the placeholder default) always fails DNS.
* ArduinoHA runs in its own FreeRTOS task, so an unreachable broker can never
  stall HomeKit/WiFi — the fan and HomeKit keep working regardless. Discovery
  is published automatically once the broker connects.

## Factory reset

Hold the button **> 10 s**: clears NVS (speed/name/MQTT), deletes HomeKit
pairing + WiFi credentials, then reboots.
