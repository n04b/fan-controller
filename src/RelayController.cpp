#include "RelayController.h"

void RelayController::begin() {
  pinMode(PIN_RELAY_LOW, OUTPUT);
  pinMode(PIN_RELAY_MEDIUM, OUTPUT);
  pinMode(PIN_RELAY_HIGH, OUTPUT);
  allOff();
  _state     = IDLE;
  _active    = LEVEL_OFF;
  _requested = LEVEL_OFF;
  _committed = LEVEL_OFF;
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

  // Re-requesting the value we're already debouncing: don't restart the timer.
  if (_state == PENDING && level == _requested) return;

  _requested = level;
  _requestAt = millis();
  _state = PENDING;   // current speed keeps running until the request settles
}

void RelayController::loop() {
  switch (_state) {
    case PENDING:
      if (millis() - _requestAt < RELAY_DEBOUNCE_MS) return;
      if (_requested == _active) {
        _state = IDLE;            // settled back to current speed; no change
      } else {
        allOff();                 // break before make
        _active = LEVEL_OFF;
        _committed = _requested;
        _deadAt = millis();
        _state = DEADTIME;
      }
      break;

    case DEADTIME:
      if (millis() - _deadAt < RELAY_SWITCH_DELAY_MS) return;
      if (_committed != LEVEL_OFF) {
        int pin = pinForLevel(_committed);
        if (pin >= 0) digitalWrite(pin, RELAY_ACTIVE_LEVEL);
      }
      _active = _committed;
      _state = IDLE;
      break;

    case IDLE:
    default:
      break;
  }
}
