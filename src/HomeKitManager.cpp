#include "HomeSpan.h"
#include "HomeKitManager.h"
#include "FanController.h"

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
static void onWifiConnect() {
  Serial.println(F("WiFi Connected"));
}

static void onPairChange(boolean isPaired) {
  Serial.println(isPaired ? F("HomeKit Paired") : F("HomeKit Reset"));
}

// ---- HomeKitManager -------------------------------------------------------
void HomeKitManager::begin(const String& deviceName, FanController* fan) {
  homeSpan.setLogLevel(0);
  homeSpan.setPairingCode(HOMEKIT_SETUP_CODE);
  homeSpan.setQRID(HOMEKIT_SETUP_ID);
  homeSpan.enableOTA(OTA_PASSWORD);          // OTA over WiFi, password protected
  homeSpan.setWifiCallback(onWifiConnect);
  homeSpan.setPairCallback(onPairChange);

  // HomeSpan owns WiFi: if no credentials are stored it starts its built-in
  // provisioning Access Point so the user can supply WiFi settings.
  homeSpan.begin(Category::Fans, deviceName.c_str());

  new SpanAccessory();
    new Service::AccessoryInformation();
      new Characteristic::Identify();
      new Characteristic::Name(deviceName.c_str());
      new Characteristic::Manufacturer("DIY");
      new Characteristic::Model("ESP32-C3 Fan");
      new Characteristic::FirmwareRevision(FW_VERSION);
    _dev = new DEV_Fan(fan);
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
