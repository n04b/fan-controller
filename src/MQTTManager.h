#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoHA.h>
#include "Config.h"

class FanController;

// ---------------------------------------------------------------------------
// Home Assistant integration via ArduinoHA (MQTT Discovery).
//
//   * Publishes a `fan` entity with on/off + speed (percentage).
//   * Discovery config is published automatically on MQTT connect, so the
//     user never creates entities manually.
//   * Reconnection is handled internally by HAMqtt; MQTT loop only runs while
//     WiFi is up so a missing network never blocks the main loop for long.
//   * Loss of MQTT / Home Assistant does not affect local or HomeKit control.
// ---------------------------------------------------------------------------
class MQTTManager {
public:
  void begin(const String& host, uint16_t port,
             const String& deviceName, FanController* fan);
  void loop();

  // Reflect a state change from another interface to Home Assistant.
  void notifyState(bool power, uint8_t percent);

private:
  static void onStateCommand(bool state, HAFan* sender);
  static void onSpeedCommand(uint16_t speed, HAFan* sender);

  static MQTTManager* _self;   // singleton for C-style ArduinoHA callbacks

  WiFiClient  _net;
  HADevice    _device;
  HAMqtt      _mqtt{_net, _device};
  HAFan       _fan{"desk_fan", HAFan::SpeedsFeature};

  FanController* _ctrl   = nullptr;
  String         _host;
  uint16_t       _port   = DEFAULT_MQTT_PORT;
  bool           _started = false;       // mqtt.begin() called
  bool           _wasConnected = false;  // for connect/disconnect logging
};
