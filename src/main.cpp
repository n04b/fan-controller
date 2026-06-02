#include <Arduino.h>
#include <WiFi.h>
#include "Config.h"
#include "SettingsManager.h"
#include "RelayController.h"
#include "FanController.h"
#include "ButtonManager.h"
#include "HomeKitManager.h"
#include "MQTTManager.h"

// ---------------------------------------------------------------------------
// Smart Fan Controller - ESP32-C3
//
// Modules:
//   SettingsManager - NVS persistence (speed, names, MQTT host/port)
//   RelayController  - 3-channel active-LOW relay, break-before-make
//   FanController    - state machine, single source of truth
//   ButtonManager    - OneButton (short = cycle, long >10s = factory reset)
//   HomeKitManager   - HomeSpan (HomeKit + WiFi provisioning + OTA)
//   MQTTManager      - ArduinoHA (Home Assistant MQTT Discovery)
//
// The fan keeps working locally (button) even with no WiFi / MQTT / HA.
// ---------------------------------------------------------------------------

static SettingsManager settings;
static RelayController relay;
static FanController   fan;
static ButtonManager   button;
static HomeKitManager  homekit;
static MQTTManager     mqtt;

static bool s_wifiConnected = false;

// Log WiFi connect/disconnect transitions (HomeSpan manages the connection).
static void onWiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      if (!s_wifiConnected) {
        s_wifiConnected = true;
        Serial.print(F("WiFi Connected: "));
        Serial.println(WiFi.localIP());
      }
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      if (s_wifiConnected) {
        s_wifiConnected = false;
        Serial.println(F("WiFi Disconnected"));
      }
      break;
    default:
      break;
  }
}

// Long press (>10s): wipe everything and reboot.
static void factoryReset() {
  Serial.println(F("Factory reset: clearing NVS, HomeKit & WiFi settings"));
  relay.setLevel(LEVEL_OFF);
  settings.factoryReset();        // speed / device name / MQTT host+port
  homekit.factoryReset();         // HomeKit pairing + WiFi credentials
  delay(200);
  ESP.restart();
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(100);
  Serial.println();
  Serial.print(F("Boot - Smart Fan Controller v"));
  Serial.println(F(FW_VERSION));

  WiFi.onEvent(onWiFiEvent);

  // Persistence + hardware first so the fan is controllable ASAP.
  settings.begin();
  relay.begin();
  fan.begin(&relay, &settings);

  // Read config (device name + MQTT broker) from NVS.
  String deviceName = settings.getDeviceName();
  String mqttHost   = settings.getMqttHost();
  uint16_t mqttPort = settings.getMqttPort();

  // Button.
  button.begin(
    []() { fan.cycle(); },      // short click -> cycle speed
    factoryReset                // long press  -> factory reset
  );

  // Interfaces.
  homekit.begin(deviceName, &fan);
  mqtt.begin(mqttHost, mqttPort, deviceName, &fan);

  // Wire FanController -> interfaces (notify on every change).
  fan.setHomeKitNotify([](bool power, uint8_t percent) {
    homekit.notifyState(power, percent);
  });
  fan.setMqttNotify([](bool power, uint8_t percent) {
    mqtt.notifyState(power, percent);
  });

  // Initial sync (device boots OFF, speed restored from NVS for the slider).
  fan.publishAll();
}

void loop() {
  homekit.loop();   // HomeSpan poll (WiFi mgmt, HomeKit, OTA) - non-blocking
  mqtt.loop();      // ArduinoHA (Home Assistant) - non-blocking
  button.loop();    // OneButton tick
  relay.loop();     // break-before-make timing
  fan.loop();
}
