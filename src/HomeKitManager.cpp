#include <WiFi.h>
#include "HomeSpan.h"
#include "HomeKitManager.h"
#include "FanController.h"
#include "SettingsManager.h"

// Settings pointer for the serial broker-config command (set in begin()).
static SettingsManager* s_settings = nullptr;

// ---------------------------------------------------------------------------
// HomeSpan Fan service.
// ---------------------------------------------------------------------------
struct DEV_Fan : Service::Fan {
  SpanCharacteristic* _active;
  SpanCharacteristic* _speed;
  FanController*      _fan;
  bool                _selfUpdate = false;  // guard while pushing external state

  explicit DEV_Fan(FanController* fan) : Service::Fan(), _fan(fan) {
    _active = new Characteristic::Active(0);              // 0 = OFF on boot
    _speed  = new Characteristic::RotationSpeed(0);
    _speed->setRange(0, 100, 1);
  }

  // Called by HomeSpan when the Home app changes Active or RotationSpeed.
  boolean update() override {
    if (_selfUpdate) return true;   // ignore our own setVal() echoes

    bool    on  = _active->getNewVal();
    uint8_t pct = (uint8_t)_speed->getNewVal();
    _fan->setFromHomeKit(on, pct);
    return true;
  }

  // Push state coming from another interface into HomeKit.
  void setExternal(bool power, uint8_t percent) {
    _selfUpdate = true;
    if (_active->getVal() != (power ? 1 : 0)) _active->setVal(power ? 1 : 0);
    if (_speed->getVal()  != percent)         _speed->setVal(percent);
    _selfUpdate = false;
  }
};

// ---- HomeSpan logging callbacks ------------------------------------------
static void onWifiConnect(int /*connectCount*/) {
  Serial.println(F("WiFi Connected"));
  // Disable modem power-save: a common source of dropped connections /
  // phantom "connected, IP 0.0.0.0" states on the ESP32-C3.
  WiFi.setSleep(WIFI_PS_NONE);
}

static void onPairChange(boolean isPaired) {
  Serial.println(isPaired ? F("HomeKit Paired") : F("HomeKit Reset"));
}

// Serial command: "@M <host> [port] [user] [pass]" -> store broker & reboot.
static void cmdSetMqtt(const char* buf) {
  if (!s_settings) return;
  char host[64] = {0}, user[64] = {0}, pass[64] = {0};
  unsigned int port = DEFAULT_MQTT_PORT;
  // buf starts with the command letter ('M'); parse what follows.
  int n = sscanf(buf + 1, "%63s %u %63s %63s", host, &port, user, pass);
  if (n < 1 || host[0] == 0) {
    Serial.println(F("Usage: @M <broker-ip-or-host> [port] [user] [pass]"));
    return;
  }
  s_settings->setMqttHost(String(host));
  s_settings->setMqttPort((uint16_t)port);
  if (n >= 3) s_settings->setMqttUser(String(user));   // only if supplied
  if (n >= 4) s_settings->setMqttPassword(String(pass));
  Serial.printf("MQTT broker set to %s:%u (user '%s') - rebooting\n",
                host, port, n >= 3 ? user : s_settings->getMqttUser().c_str());
  delay(200);
  ESP.restart();
}

// ---- HomeKitManager -------------------------------------------------------
void HomeKitManager::begin(const String& deviceName, FanController* fan,
                           SettingsManager* settings) {
  s_settings = settings;
  homeSpan.setLogLevel(0);
  homeSpan.setPairingCode(HOMEKIT_SETUP_CODE);
  homeSpan.setQRID(HOMEKIT_SETUP_ID);
  homeSpan.enableOTA(OTA_PASSWORD);          // OTA over WiFi, password protected
  homeSpan.setConnectionCallback(onWifiConnect);
  homeSpan.setPairCallback(onPairChange);

  // Feed HomeSpan the credentials captured by the captive portal so it
  // connects directly and never runs its own WiFi setup AP. (This boot only
  // happens once WiFi is configured; see main.cpp / ConfigPortal.)
  String ssid = settings->getWifiSsid();
  if (ssid.length()) {
    homeSpan.setWifiCredentials(ssid.c_str(), settings->getWifiPassword().c_str());
  }

  homeSpan.begin(Category::Fans, deviceName.c_str());

  new SpanAccessory();
    new Service::AccessoryInformation();
      new Characteristic::Identify();
      new Characteristic::Name(deviceName.c_str());
      new Characteristic::Manufacturer("DIY");
      new Characteristic::Model("ESP32-C3 Fan");
      new Characteristic::FirmwareRevision(FW_VERSION);
    _dev = new DEV_Fan(fan);

  // Serial CLI extension to (re)configure the MQTT broker without reflashing.
  new SpanUserCommand('M', "<host> [port] - set MQTT broker & reboot", cmdSetMqtt);
}

void HomeKitManager::loop() {
  homeSpan.poll();
}

void HomeKitManager::notifyState(bool power, uint8_t percent) {
  if (_dev) _dev->setExternal(power, percent);
}

void HomeKitManager::factoryReset() {
  Serial.println(F("HomeKit Reset"));
  // Delete HomeKit pairing data and stored WiFi credentials.
  homeSpan.processSerialCommand("H");   // delete HomeKit pairing
  homeSpan.processSerialCommand("X");   // delete WiFi credentials
}
