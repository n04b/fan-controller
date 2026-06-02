#include "SettingsManager.h"

void SettingsManager::begin() {
  // Open (and create) the namespace in read/write mode and keep it open.
  _prefs.begin(NVS_NAMESPACE, false);
}

uint8_t SettingsManager::getSpeed() {
  uint8_t s = _prefs.getUChar("speed", LEVEL_MEDIUM);
  if (s < LEVEL_LOW || s > LEVEL_HIGH) s = LEVEL_MEDIUM;
  return s;
}

void SettingsManager::setSpeed(uint8_t level) {
  if (level < LEVEL_LOW || level > LEVEL_HIGH) return;  // never persist OFF
  if (getSpeed() == level) return;                       // avoid needless writes
  _prefs.putUChar("speed", level);
}

String SettingsManager::getDeviceName() {
  return _prefs.getString("device_name", DEFAULT_DEVICE_NAME);
}

void SettingsManager::setDeviceName(const String& name) {
  _prefs.putString("device_name", name);
}

String SettingsManager::getMqttHost() {
  return _prefs.getString("mqtt_host", DEFAULT_MQTT_HOST);
}

void SettingsManager::setMqttHost(const String& host) {
  _prefs.putString("mqtt_host", host);
}

uint16_t SettingsManager::getMqttPort() {
  return _prefs.getUShort("mqtt_port", DEFAULT_MQTT_PORT);
}

void SettingsManager::setMqttPort(uint16_t port) {
  _prefs.putUShort("mqtt_port", port);
}

String SettingsManager::getMqttUser() {
  return _prefs.getString("mqtt_user", "");
}

void SettingsManager::setMqttUser(const String& user) {
  _prefs.putString("mqtt_user", user);
}

String SettingsManager::getMqttPassword() {
  return _prefs.getString("mqtt_pass", "");
}

void SettingsManager::setMqttPassword(const String& pass) {
  _prefs.putString("mqtt_pass", pass);
}

String SettingsManager::getWifiSsid() {
  return _prefs.getString("wifi_ssid", "");
}

void SettingsManager::setWifiSsid(const String& ssid) {
  _prefs.putString("wifi_ssid", ssid);
}

String SettingsManager::getWifiPassword() {
  return _prefs.getString("wifi_pass", "");
}

void SettingsManager::setWifiPassword(const String& pass) {
  _prefs.putString("wifi_pass", pass);
}

bool SettingsManager::hasWifiConfigured() {
  return getWifiSsid().length() > 0;
}

void SettingsManager::factoryReset() {
  _prefs.clear();   // erase all keys in this namespace
}
