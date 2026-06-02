#pragma once
#include <Arduino.h>
#include "Config.h"

// ---------------------------------------------------------------------------
// Drives the 3-channel relay module (active LOW).
//
// Guarantees:
//   * Only one relay is ever energized at a time.
//   * Speed changes are break-before-make: all relays are de-energized,
//     then after RELAY_SWITCH_DELAY_MS the target relay is energized.
//   * Fully non-blocking (the 200 ms gap is handled with millis() in loop()).
// ---------------------------------------------------------------------------
class RelayController {
public:
  void begin();

  // Request a target level (0..3). Off (0) takes effect immediately;
  // any non-zero target goes through the break-before-make sequence.
  void setLevel(uint8_t level);

  // Must be called frequently from the main loop.
  void loop();

private:
  void allOff();
  int  pinForLevel(uint8_t level);

  uint8_t  _target  = LEVEL_OFF;   // requested level
  uint8_t  _active  = LEVEL_OFF;   // currently energized level
  bool     _waiting = false;       // inside the 200 ms dead time
  uint32_t _waitStart = 0;
};
