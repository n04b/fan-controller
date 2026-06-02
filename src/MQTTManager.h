#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoHA.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include "Config.h"

class FanController;

// ---------------------------------------------------------------------------
// Home Assistant integration via ArduinoHA (MQTT Discovery).
//
//   * Publishes a `fan` entity with on/off + speed (percentage).
//   * Discovery config is published automatically on MQTT connect, so the
//     user never creates entities manually.
//   * Loss of MQTT / Home Assistant does not affect local or HomeKit control.
//
// THREADING:
//   ArduinoHA / PubSubClient do blocking DNS + TCP connects. If run from the
//   main loop they stall homeSpan.poll() for seconds and HomeSpan tears the
//   WiFi link down. So ALL ArduinoHA access happens in a dedicated FreeRTOS
//   task; blocking calls there yield to the main loop instead of starving it.
//
//   Ownership split (no shared library state across threads):
//     * ArduinoHA objects  -> MQTT task only
//     * FanController       -> main thread only
//     * small command/state buffers -> shared, guarded by a mutex
// ---------------------------------------------------------------------------
class MQTTManager {
public:
  void begin(const String& host, uint16_t port,
             const String& deviceName, FanController* fan);

  // Called from the main loop: applies inbound MQTT commands to the fan and
  // logs connect/disconnect edges. Never touches the network.
  void loop();

  // Reflect a state change from another interface to Home Assistant
  // (main thread): just hands the values to the MQTT task.
  void notifyState(bool power, uint8_t percent);

private:
  static void onStateCommand(bool state, HAFan* sender);
  static void onSpeedCommand(uint16_t speed, HAFan* sender);

  static void taskTrampoline(void* arg);
  void taskLoop();                 // runs in the MQTT task
  bool resolveTarget();            // host string -> _target IP

  static MQTTManager* _self;       // singleton for C-style ArduinoHA callbacks

  WiFiClient  _net;
  HADevice    _device;
  HAMqtt      _mqtt{_net, _device};
  HAFan       _fan{"desk_fan", HAFan::SpeedsFeature};

  FanController* _ctrl = nullptr;
  String         _host;
  uint16_t       _port = DEFAULT_MQTT_PORT;

  TaskHandle_t      _task = nullptr;
  SemaphoreHandle_t _mux  = nullptr;

  IPAddress _target;
  bool      _haveTarget  = false;
  bool      _started     = false;     // _mqtt.begin() called (task only)
  uint32_t  _lastResolve = 0;         // throttle DNS attempts (task only)

  volatile bool _connected = false;   // set by task, read by main thread
  bool          _wasConnected = false;// main-thread edge detector

  // Inbound: MQTT task (callbacks) -> main thread, mutex guarded.
  bool    _cmdHasPower   = false;
  bool    _cmdPower      = false;
  bool    _cmdHasPercent = false;
  uint8_t _cmdPercent    = 0;

  // Outbound: main thread (notifyState) -> MQTT task, mutex guarded.
  bool    _pubPending = false;
  bool    _pubPower   = false;
  uint8_t _pubPercent = 0;
};
