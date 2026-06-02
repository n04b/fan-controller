#include "ButtonManager.h"

void ButtonManager::begin(Callback onClick, Callback onLongPress) {
  _onClick = onClick;
  _onLongPress = onLongPress;

  // Long press threshold = 10 s.
  _btn.setPressMs(BUTTON_LONG_PRESS_MS);

  _btn.attachClick(handleClick, this);
  _btn.attachLongPressStart(handleLongPress, this);
}

void ButtonManager::loop() {
  _btn.tick();
}

void ButtonManager::handleClick(void* ctx) {
  auto* self = static_cast<ButtonManager*>(ctx);
  Serial.println(F("Button Click"));
  if (self->_onClick) self->_onClick();
}

void ButtonManager::handleLongPress(void* ctx) {
  auto* self = static_cast<ButtonManager*>(ctx);
  Serial.println(F("Button Long Press"));
  if (self->_onLongPress) self->_onLongPress();
}
