#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>

class SettingsManager;

// ---------------------------------------------------------------------------
// Configuration captive portal.
//
// Runs when the device has no WiFi credentials stored (first boot or after a
// factory reset). Brings up a SoftAP + DNS catch-all so any phone/laptop that
// joins is bounced to a web form to configure:
//
//     * WiFi SSID + password   (with a live network scan)
//     * Device name            (HomeKit / Home Assistant)
//     * MQTT broker host + port
//
// On save the values are written to NVS and the device reboots into normal
// operation, where HomeSpan connects using the stored credentials
// (homeSpan.setWifiCredentials), so HomeSpan never runs its own AP setup.
//
// HomeSpan / ArduinoHA are NOT started while the portal is active.
// ---------------------------------------------------------------------------
class ConfigPortal {
public:
  void begin(SettingsManager* settings);
  void loop();

private:
  void handleRoot();
  void handleScan();
  void handleSave();
  void handleNotFound();
  bool redirectToPortal();   // captive-portal redirect; true if handled

  SettingsManager* _settings = nullptr;
  DNSServer        _dns;
  WebServer        _server{80};
  IPAddress        _apIP;
  String           _apSsid;
};
