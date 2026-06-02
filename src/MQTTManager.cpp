#include "MQTTManager.h"
#include "FanController.h"

MQTTManager* MQTTManager::_self = nullptr;

void MQTTManager::begin(const String& host, uint16_t port,
                        const String& deviceName, FanController* fan) {
  _self = this;
  _ctrl = fan;
  _host = host;
  _port = port;

  // Unique device id from the WiFi MAC (available without a connection).
  byte mac[6];
  WiFi.macAddress(mac);
  _device.setUniqueId(mac, sizeof(mac));
  _device.setName(deviceName.c_str());
  _device.setManufacturer("DIY");
  _device.setModel("ESP32-C3 Fan");
  _device.setSoftwareVersion(FW_VERSION);
  _device.enableSharedAvailability();
  _device.enableLastWill();

  // Fan entity: on/off + speed as percentage (1..100).
  _fan.setName(deviceName.c_str());
  _fan.setIcon("mdi:fan");
  _fan.setSpeedRangeMin(1);
  _fan.setSpeedRangeMax(100);
  _fan.onStateCommand(onStateCommand);
  _fan.onSpeedCommand(onSpeedCommand);

  // Actual MQTT connection is deferred to loop() until WiFi is up.
}

void MQTTManager::loop() {
  if (WiFi.status() != WL_CONNECTED) return;   // don't block without a network

  if (!_started) {
    _mqtt.begin(_host.c_str(), _port);
    _started = true;
  }

  _mqtt.loop();

  // Connect / disconnect logging + state resync on (re)connect.
  bool connected = _mqtt.isConnected();
  if (connected && !_wasConnected) {
    Serial.println(F("MQTT Connected"));
    if (_ctrl) _ctrl->publishAll();   // discovery is automatic; resync state
  } else if (!connected && _wasConnected) {
    Serial.println(F("MQTT Disconnected"));
  }
  _wasConnected = connected;
}

void MQTTManager::notifyState(bool power, uint8_t percent) {
  _fan.setState(power);
  if (percent > 0) _fan.setSpeed(percent);
}

// ---- ArduinoHA command callbacks -----------------------------------------
void MQTTManager::onStateCommand(bool state, HAFan* /*sender*/) {
  if (_self && _self->_ctrl) _self->_ctrl->setFromMqttPower(state);
}

void MQTTManager::onSpeedCommand(uint16_t speed, HAFan* /*sender*/) {
  if (_self && _self->_ctrl) {
    if (speed > 100) speed = 100;
    _self->_ctrl->setFromMqttPercent((uint8_t)speed);
  }
}
