#pragma once
#include <Arduino.h>
#include "Config.h"

// ---------------------------------------------------------------------------
// Drives the 3-channel relay module (active LOW).
//
// Guarantees:
//   * Only one relay is ever energized at a time.
//   * Requested changes are debounced (RELAY_DEBOUNCE_MS): the current speed
//     keeps running until a new request stays stable, so a burst of commands
//     (slider drag, fast button taps) results in a single transition and no
//     contact chatter.
//   * Transitions are break-before-make: all relays de-energize, then after
//     RELAY_SWITCH_DELAY_MS the target relay is energized.
//   * Fully non-blocking (all timing handled with millis() in loop()).
//
// State machine:
//   IDLE      - relays reflect _active, nothing pending
//   PENDING   - a new target requested, waiting out the debounce window
//   DEADTIME  - relays all off, waiting the break-before-make gap
// ---------------------------------------------------------------------------
class RelayController {
public:
  void begin();

  // Request a target level (0..3). Takes effect after debounce.
  void setLevel(uint8_t level);

  // Must be called frequently from the main loop.
  void loop();

private:
  enum State : uint8_t { IDLE, PENDING, DEADTIME };

  void allOff();
  int  pinForLevel(uint8_t level);

  State    _state     = IDLE;
  uint8_t  _active    = LEVEL_OFF;   // currently energized level
  uint8_t  _requested = LEVEL_OFF;   // latest requested level (debouncing)
  uint8_t  _committed = LEVEL_OFF;   // target locked in for the dead-time gap
  uint32_t _requestAt = 0;           // when _requested last changed
  uint32_t _deadAt    = 0;           // when the dead time started
};
