#include "MQTTManager.h"
#include "FanController.h"

MQTTManager* MQTTManager::_self = nullptr;

void MQTTManager::begin(const String& host, uint16_t port,
                        const String& deviceName, FanController* fan) {
  _self = this;
  _ctrl = fan;
  _host = host;
  _port = port;

  // Bound how long a connect to a down/unreachable broker can block the task.
  _net.setConnectionTimeout(2000);   // ms

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

  _mux = xSemaphoreCreateMutex();

  // All blocking network work lives here, off the main loop.
  xTaskCreatePinnedToCore(taskTrampoline, "mqtt", 8192, this,
                          1 /*priority, same as loopTask*/, &_task, 0);
}

// ---- MQTT task ------------------------------------------------------------
void MQTTManager::taskTrampoline(void* arg) {
  static_cast<MQTTManager*>(arg)->taskLoop();
}

bool MQTTManager::resolveTarget() {
  // IP literal? No DNS needed (recommended configuration).
  if (_target.fromString(_host)) return true;

  // mDNS (.local) is not resolvable by the lwIP resolver.
  if (_host.endsWith(".local")) {
    Serial.printf("MQTT: '%s' is mDNS (.local) and cannot be resolved. "
                  "Set a broker IP via serial:  @M <ip> [port]\n", _host.c_str());
    return false;
  }

  IPAddress ip;
  if (WiFi.hostByName(_host.c_str(), ip) == 1) {
    _target = ip;
    return true;
  }
  Serial.printf("MQTT: cannot resolve '%s'\n", _host.c_str());
  return false;
}

void MQTTManager::taskLoop() {
  for (;;) {
    if (WiFi.status() == WL_CONNECTED && WiFi.localIP() != IPAddress((uint32_t)0)) {
      // Resolve broker (throttled so a failing lookup can't spin).
      if (!_haveTarget && (millis() - _lastResolve >= 30000 || _lastResolve == 0)) {
        _lastResolve = millis();
        _haveTarget = resolveTarget();
      }

      if (_haveTarget) {
        if (!_started) {
          _mqtt.begin(_target, _port);
          _started = true;
        }
        _mqtt.loop();
        _connected = _mqtt.isConnected();

        // Push the latest desired state out (set by the main thread).
        bool doPub = false, power = false; uint8_t pct = 0;
        if (xSemaphoreTake(_mux, portMAX_DELAY) == pdTRUE) {
          if (_pubPending) {
            doPub = true; power = _pubPower; pct = _pubPercent;
            _pubPending = false;
          }
          xSemaphoreGive(_mux);
        }
        if (doPub) {
          _fan.setState(power);
          if (pct > 0) _fan.setSpeed(pct);
        }
      }
    } else {
      // No usable network: pause MQTT. _started / _haveTarget are kept so we
      // resume (not re-init) on reconnect; ArduinoHA reconnects the broker.
      _connected = false;
    }

    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

// ---- Main thread ----------------------------------------------------------
void MQTTManager::loop() {
  // Connect / disconnect logging + state resync on (re)connect.
  bool connected = _connected;
  if (connected && !_wasConnected) {
    Serial.println(F("MQTT Connected"));
    if (_ctrl) _ctrl->publishAll();   // discovery is automatic; resync state
  } else if (!connected && _wasConnected) {
    Serial.println(F("MQTT Disconnected"));
  }
  _wasConnected = connected;

  // Apply any commands received from Home Assistant (captured in the task).
  bool hasPower = false, power = false, hasPercent = false; uint8_t pct = 0;
  if (_mux && xSemaphoreTake(_mux, 0) == pdTRUE) {
    hasPower   = _cmdHasPower;   power = _cmdPower;
    hasPercent = _cmdHasPercent; pct   = _cmdPercent;
    _cmdHasPower = _cmdHasPercent = false;
    xSemaphoreGive(_mux);
  }
  if (_ctrl) {
    if (hasPercent)     _ctrl->setFromMqttPercent(pct);
    else if (hasPower)  _ctrl->setFromMqttPower(power);
  }
}

void MQTTManager::notifyState(bool power, uint8_t percent) {
  if (!_mux) return;
  if (xSemaphoreTake(_mux, portMAX_DELAY) == pdTRUE) {
    _pubPending = true;
    _pubPower   = power;
    _pubPercent = percent;
    xSemaphoreGive(_mux);
  }
}

// ---- ArduinoHA command callbacks (run in the MQTT task) -------------------
void MQTTManager::onStateCommand(bool state, HAFan* /*sender*/) {
  if (!_self || !_self->_mux) return;
  if (xSemaphoreTake(_self->_mux, portMAX_DELAY) == pdTRUE) {
    _self->_cmdHasPower = true;
    _self->_cmdPower    = state;
    xSemaphoreGive(_self->_mux);
  }
}

void MQTTManager::onSpeedCommand(uint16_t speed, HAFan* /*sender*/) {
  if (!_self || !_self->_mux) return;
  if (speed > 100) speed = 100;
  if (xSemaphoreTake(_self->_mux, portMAX_DELAY) == pdTRUE) {
    _self->_cmdHasPercent = true;
    _self->_cmdPercent    = (uint8_t)speed;
    xSemaphoreGive(_self->_mux);
  }
}
