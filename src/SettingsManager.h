#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "Config.h"

// ---------------------------------------------------------------------------
// Persistent settings stored in NVS (Preferences).
//
// Stored:
//   speed       (uint8, 1..3)  - last selected speed level
//   device_name (string)
//   mqtt_host   (string)
//   mqtt_port   (uint16)
//
// NOTE: power state is intentionally NOT stored. The device always boots OFF.
// ---------------------------------------------------------------------------
class SettingsManager {
public:
  void begin();

  uint8_t  getSpeed();                 // 1..3 (defaults to MEDIUM)
  void     setSpeed(uint8_t level);    // persists 1..3 only

  String   getDeviceName();
  void     setDeviceName(const String& name);

  String   getMqttHost();
  void     setMqttHost(const String& host);

  uint16_t getMqttPort();
  void     setMqttPort(uint16_t port);

  String   getMqttUser();
  void     setMqttUser(const String& user);

  String   getMqttPassword();
  void     setMqttPassword(const String& pass);

  String   getWifiSsid();
  void     setWifiSsid(const String& ssid);

  String   getWifiPassword();
  void     setWifiPassword(const String& pass);

  // True once WiFi has been provisioned (SSID stored). When false the
  // device boots into the configuration captive portal.
  bool     hasWifiConfigured();

  // Wipe the whole namespace (used by long-press factory reset).
  void     factoryReset();

private:
  Preferences _prefs;
};
