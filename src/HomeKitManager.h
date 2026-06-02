#pragma once
#include <Arduino.h>
#include "Config.h"

class FanController;
class SettingsManager;
class DEV_Fan;   // internal HomeSpan service (defined in .cpp)

// ---------------------------------------------------------------------------
// HomeKit accessory via HomeSpan.
//
//   * Accessory category: Fan
//   * Characteristics: Active (on/off), RotationSpeed (0..100%)
//   * HomeSpan manages the WiFi connection (using credentials captured by the
//     captive portal and passed in via homeSpan.setWifiCredentials) and OTA.
//
// NOTE on provisioning: the spec asks for BLE onboarding through Apple Home.
// HomeSpan implements HAP over WiFi/IP only and has no BLE/WAC transport, so
// onboarding is handled by the ConfigPortal captive portal instead. HomeSpan
// never runs its own setup AP because credentials are supplied up front.
// ---------------------------------------------------------------------------
class HomeKitManager {
public:
  void begin(const String& deviceName, FanController* fan, SettingsManager* settings);
  void loop();

  // Reflect an external state change (button / MQTT) into HomeKit.
  void notifyState(bool power, uint8_t percent);

  // Erase HomeKit pairing + WiFi credentials and reboot.
  void factoryReset();

private:
  DEV_Fan* _dev = nullptr;
};
