#pragma once
#include <Arduino.h>
#include "Config.h"

class FanController;
class DEV_Fan;   // internal HomeSpan service (defined in .cpp)

// ---------------------------------------------------------------------------
// HomeKit accessory via HomeSpan.
//
//   * Accessory category: Fan
//   * Characteristics: Active (on/off), RotationSpeed (0..100%)
//   * HomeSpan also owns WiFi management + WiFi provisioning (its built-in
//     setup Access Point when no credentials are stored) and OTA.
//
// NOTE on provisioning: the spec asks for BLE onboarding through Apple Home.
// HomeSpan implements HAP over WiFi/IP only and has no BLE/WAC transport, so
// HomeSpan's native WiFi provisioning is used instead. This is the closest
// functional equivalent achievable with the mandated library.
// ---------------------------------------------------------------------------
class HomeKitManager {
public:
  void begin(const String& deviceName, FanController* fan);
  void loop();

  // Reflect an external state change (button / MQTT) into HomeKit.
  void notifyState(bool power, uint8_t percent);

  // Erase HomeKit pairing + WiFi credentials and reboot.
  void factoryReset();

private:
  DEV_Fan* _dev = nullptr;
};
