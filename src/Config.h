#pragma once
#include <Arduino.h>

// ---------------------------------------------------------------------------
// Smart Fan Controller - global configuration
// ESP32-C3 SuperMini
// ---------------------------------------------------------------------------

#define FW_VERSION "1.0"

// ----- Relay module (3 channels, ACTIVE LOW) -------------------------------
#define PIN_RELAY_LOW        5    // GPIO5  - Low speed
#define PIN_RELAY_MEDIUM     4    // GPIO4  - Medium speed
#define PIN_RELAY_HIGH       3    // GPIO3  - High speed

#define RELAY_ACTIVE_LEVEL   LOW
#define RELAY_IDLE_LEVEL     HIGH

// Break-before-make delay when switching speed (ms). Non-blocking.
#define RELAY_SWITCH_DELAY_MS 200

// ----- User button ---------------------------------------------------------
#define PIN_BUTTON           6    // GPIO6 -> button -> GND, INPUT_PULLUP
#define BUTTON_LONG_PRESS_MS 10000  // long press = factory reset

// ----- Serial logging ------------------------------------------------------
#define SERIAL_BAUD          115200

// ----- HomeKit -------------------------------------------------------------
#define DEFAULT_DEVICE_NAME  "Desk Fan"
// HomeSpan default setup code (format: XXX-XX-XXX). Change for production.
#define HOMEKIT_SETUP_CODE   "46637726"
#define HOMEKIT_SETUP_ID     "FANC"

// ----- OTA -----------------------------------------------------------------
#define OTA_PASSWORD         "fan-ota-2024"

// ----- MQTT defaults (overridable via NVS) ---------------------------------
#define DEFAULT_MQTT_HOST    "broker.local"
#define DEFAULT_MQTT_PORT    1883

// ----- NVS namespace -------------------------------------------------------
#define NVS_NAMESPACE        "fanctl"

// ----- Fan speed levels ----------------------------------------------------
// 0 = OFF, 1 = LOW, 2 = MEDIUM, 3 = HIGH
enum FanLevel : uint8_t {
  LEVEL_OFF    = 0,
  LEVEL_LOW    = 1,
  LEVEL_MEDIUM = 2,
  LEVEL_HIGH   = 3
};

// Source of a state change (used to avoid feedback loops between interfaces).
enum ChangeSource : uint8_t {
  SRC_SYSTEM  = 0,   // boot / internal
  SRC_BUTTON  = 1,
  SRC_HOMEKIT = 2,
  SRC_MQTT    = 3
};
