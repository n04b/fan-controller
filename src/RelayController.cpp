#include "RelayController.h"

void RelayController::begin() {
  pinMode(PIN_RELAY_LOW, OUTPUT);
  pinMode(PIN_RELAY_MEDIUM, OUTPUT);
  pinMode(PIN_RELAY_HIGH, OUTPUT);
  allOff();
  _target = LEVEL_OFF;
  _active = LEVEL_OFF;
  _waiting = false;
}

int RelayController::pinForLevel(uint8_t level) {
  switch (level) {
    case LEVEL_LOW:    return PIN_RELAY_LOW;
    case LEVEL_MEDIUM: return PIN_RELAY_MEDIUM;
    case LEVEL_HIGH:   return PIN_RELAY_HIGH;
    default:           return -1;
  }
}

void RelayController::allOff() {
  digitalWrite(PIN_RELAY_LOW, RELAY_IDLE_LEVEL);
  digitalWrite(PIN_RELAY_MEDIUM, RELAY_IDLE_LEVEL);
  digitalWrite(PIN_RELAY_HIGH, RELAY_IDLE_LEVEL);
}

void RelayController::setLevel(uint8_t level) {
  if (level > LEVEL_HIGH) level = LEVEL_OFF;
  if (level == _target && level == _active && !_waiting) return;  // no change

  _target = level;

  // Always de-energize everything first (break-before-make).
  allOff();
  _active = LEVEL_OFF;

  if (level == LEVEL_OFF) {
    _waiting = false;             // nothing more to do
  } else {
    _waiting = true;              // wait the dead time, then energize target
    _waitStart = millis();
  }
}

void RelayController::loop() {
  if (!_waiting) return;
  if (millis() - _waitStart < RELAY_SWITCH_DELAY_MS) return;

  int pin = pinForLevel(_target);
  if (pin >= 0) {
    digitalWrite(pin, RELAY_ACTIVE_LEVEL);
    _active = _target;
  }
  _waiting = false;
}
