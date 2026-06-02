#pragma once
#include <Arduino.h>
#include <functional>
#include <OneButton.h>
#include "Config.h"

// ---------------------------------------------------------------------------
// Physical button (GPIO6 -> button -> GND, INPUT_PULLUP).
//
//   * Short click       -> onClick     (cycle speed)
//   * Long press > 10 s  -> onLongPress (factory reset)
//
// Wraps OneButton; fully non-blocking via tick() in loop().
// ---------------------------------------------------------------------------
class ButtonManager {
public:
  using Callback = std::function<void()>;

  void begin(Callback onClick, Callback onLongPress);
  void loop();

private:
  static void handleClick(void* ctx);
  static void handleLongPress(void* ctx);

  OneButton _btn{PIN_BUTTON, true /*activeLow*/, true /*pullup*/};
  Callback  _onClick;
  Callback  _onLongPress;
};
