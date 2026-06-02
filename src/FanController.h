#pragma once
#include <Arduino.h>
#include <functional>
#include "Config.h"
#include "RelayController.h"
#include "SettingsManager.h"

// ---------------------------------------------------------------------------
// Core fan state machine.
//
// Single source of truth for fan state. Every interface (button, HomeKit,
// MQTT) routes its requests here. After any change this class:
//   1. updates the relay,
//   2. persists the speed (NVS),
//   3. notifies HomeKit,
//   4. notifies MQTT.
//
// To prevent feedback loops, the change source is tracked and the originating
// interface is not echoed back to itself (HomeKit only; MQTT state topic never
// re-triggers a command so it is always refreshed).
// ---------------------------------------------------------------------------
class FanController {
public:
  // Notify callbacks: (power on/off, speed as 0..100%).
  using NotifyCb = std::function<void(bool power, uint8_t percent)>;

  void begin(RelayController* relay, SettingsManager* settings);
  void loop();

  void setHomeKitNotify(NotifyCb cb) { _hkCb = cb; }
  void setMqttNotify(NotifyCb cb)    { _mqttCb = cb; }

  // --- Inputs from interfaces ---------------------------------------------
  void cycle();                                       // button short press
  void setFromHomeKit(bool on, uint8_t percent);
  void setFromMqttPower(bool on);
  void setFromMqttPercent(uint8_t percent);

  // Push current state out to all interfaces (e.g. after boot / reconnect).
  void publishAll();

  // --- State accessors -----------------------------------------------------
  bool    isOn()      const { return _level != LEVEL_OFF; }
  uint8_t level()     const { return _level; }
  uint8_t percent()   const { return levelToPercent(displayLevel()); }

  // --- Mapping helpers (HomeKit / HA percentage <-> level) -----------------
  static uint8_t percentToLevel(uint8_t pct);
  static uint8_t levelToPercent(uint8_t level);

private:
  void apply(uint8_t newLevel, ChangeSource src);
  void publish(ChangeSource src);
  uint8_t displayLevel() const { return _level ? _level : _savedSpeed; }
  static const char* levelName(uint8_t level);

  RelayController* _relay    = nullptr;
  SettingsManager* _settings = nullptr;

  uint8_t  _level     = LEVEL_OFF;     // live level 0..3
  uint8_t  _savedSpeed = LEVEL_MEDIUM; // last non-off speed 1..3

  NotifyCb _hkCb;
  NotifyCb _mqttCb;
};
