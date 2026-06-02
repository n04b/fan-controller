#include "FanController.h"

void FanController::begin(RelayController* relay, SettingsManager* settings) {
  _relay = relay;
  _settings = settings;

  _savedSpeed = _settings->getSpeed();   // 1..3, restored from NVS
  _level = LEVEL_OFF;                     // device always boots OFF

  // Make sure the relay reflects the OFF state and sync all interfaces.
  apply(LEVEL_OFF, SRC_SYSTEM);
}

void FanController::loop() {
  // Reserved for future timed behaviour; state machine is event driven.
}

// ---- Mapping --------------------------------------------------------------
// 0% -> OFF, 1-33% -> LOW, 34-66% -> MEDIUM, 67-100% -> HIGH
uint8_t FanController::percentToLevel(uint8_t pct) {
  if (pct == 0)   return LEVEL_OFF;
  if (pct <= 33)  return LEVEL_LOW;
  if (pct <= 66)  return LEVEL_MEDIUM;
  return LEVEL_HIGH;
}

uint8_t FanController::levelToPercent(uint8_t level) {
  switch (level) {
    case LEVEL_LOW:    return 33;
    case LEVEL_MEDIUM: return 66;
    case LEVEL_HIGH:   return 100;
    default:           return 0;
  }
}

const char* FanController::levelName(uint8_t level) {
  switch (level) {
    case LEVEL_LOW:    return "LOW";
    case LEVEL_MEDIUM: return "MEDIUM";
    case LEVEL_HIGH:   return "HIGH";
    default:           return "OFF";
  }
}

// ---- Core ----------------------------------------------------------------
void FanController::apply(uint8_t newLevel, ChangeSource src) {
  if (newLevel > LEVEL_HIGH) newLevel = LEVEL_OFF;

  _level = newLevel;

  if (_level != LEVEL_OFF) {
    _savedSpeed = _level;
    _settings->setSpeed(_savedSpeed);    // persist last speed
  }

  // 1. update relay
  _relay->setLevel(_level);

  // logging
  if (_level == LEVEL_OFF) {
    Serial.println(F("Fan OFF"));
  } else {
    Serial.println(F("Fan ON"));
    Serial.printf("Speed %s\n", levelName(_level));
  }

  // 2. persist (done above)  3+4. notify interfaces
  publish(src);
}

void FanController::publish(ChangeSource src) {
  bool power = isOn();
  uint8_t pct = levelToPercent(displayLevel());

  // Do not echo a HomeKit-originated change back into HomeKit
  // (the controller already holds the value it just sent us).
  if (src != SRC_HOMEKIT && _hkCb) _hkCb(power, pct);

  // MQTT state topic never re-triggers a command, so always refresh it.
  if (_mqttCb) _mqttCb(power, pct);
}

void FanController::publishAll() {
  publish(SRC_SYSTEM);
}

// ---- Interface inputs -----------------------------------------------------
void FanController::cycle() {
  // OFF -> LOW -> MEDIUM -> HIGH -> OFF
  uint8_t next = (_level + 1) % 4;
  apply(next, SRC_BUTTON);
}

void FanController::setFromHomeKit(bool on, uint8_t percent) {
  if (!on) {
    apply(LEVEL_OFF, SRC_HOMEKIT);
    return;
  }
  // Active ON. If no usable speed given, fall back to last saved speed.
  uint8_t level = percentToLevel(percent);
  if (level == LEVEL_OFF) level = _savedSpeed;
  apply(level, SRC_HOMEKIT);
}

void FanController::setFromMqttPower(bool on) {
  apply(on ? _savedSpeed : LEVEL_OFF, SRC_MQTT);
}

void FanController::setFromMqttPercent(uint8_t percent) {
  apply(percentToLevel(percent), SRC_MQTT);
}
