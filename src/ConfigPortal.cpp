#include "ConfigPortal.h"
#include "SettingsManager.h"
#include "Config.h"

static const char PAGE_HEAD[] PROGMEM =
  "<!DOCTYPE html><html lang='en'><head><meta charset='utf-8'>"
  "<meta name='viewport' content='width=device-width,initial-scale=1'>"
  "<title>Fan Controller Setup</title><style>"
  "body{font-family:-apple-system,system-ui,sans-serif;background:#0f172a;color:#e2e8f0;"
  "margin:0;padding:24px;display:flex;justify-content:center}"
  ".card{background:#1e293b;border-radius:16px;padding:24px;max-width:420px;width:100%;"
  "box-shadow:0 10px 30px rgba(0,0,0,.4)}"
  "h1{font-size:20px;margin:0 0 4px}p.sub{color:#94a3b8;margin:0 0 20px;font-size:13px}"
  "label{display:block;font-size:13px;margin:14px 0 4px;color:#cbd5e1}"
  "input,select{width:100%;box-sizing:border-box;padding:11px 12px;border-radius:10px;"
  "border:1px solid #334155;background:#0f172a;color:#e2e8f0;font-size:15px}"
  ".row{display:flex;gap:10px}.row>div{flex:1}"
  "button{margin-top:22px;width:100%;padding:13px;border:0;border-radius:10px;"
  "background:#3b82f6;color:#fff;font-size:16px;font-weight:600;cursor:pointer}"
  "button:active{background:#2563eb}.hint{font-size:12px;color:#64748b;margin-top:6px}"
  "</style></head><body><div class='card'>"
  "<h1>Smart Fan Controller</h1><p class='sub'>Device setup</p>"
  "<form method='POST' action='/save'>";

static const char PAGE_SCRIPT[] PROGMEM =
  "<script>"
  "fetch('/scan').then(r=>r.json()).then(n=>{"
  "let dl=document.getElementById('nets');"
  "n.forEach(s=>{let o=document.createElement('option');o.value=s.ssid;"
  "o.label=s.ssid+' ('+s.rssi+' dBm)';dl.appendChild(o);});"
  "}).catch(()=>{});"
  "</script></div></body></html>";

void ConfigPortal::begin(SettingsManager* settings) {
  _settings = settings;

  WiFi.persistent(false);
  WiFi.mode(WIFI_AP_STA);   // AP for the portal, STA enabled so scan works

  uint8_t mac[6];
  WiFi.macAddress(mac);
  char suffix[5];
  snprintf(suffix, sizeof(suffix), "%02X%02X", mac[4], mac[5]);
  _apSsid = String("Fan-Controller-") + suffix;

  WiFi.softAP(_apSsid.c_str());
  delay(100);
  _apIP = WiFi.softAPIP();

  // Catch-all DNS: every lookup resolves to us so the OS opens the portal.
  _dns.start(53, "*", _apIP);

  _server.on("/", HTTP_GET, [this]() { handleRoot(); });
  _server.on("/scan", HTTP_GET, [this]() { handleScan(); });
  _server.on("/save", HTTP_POST, [this]() { handleSave(); });
  _server.onNotFound([this]() { handleNotFound(); });
  _server.begin();

  Serial.printf("Config portal up. Join WiFi '%s' then open http://%s\n",
                _apSsid.c_str(), _apIP.toString().c_str());
}

void ConfigPortal::loop() {
  _dns.processNextRequest();
  _server.handleClient();
}

bool ConfigPortal::redirectToPortal() {
  // If the request isn't aimed at our IP, bounce it to the portal root.
  if (_server.hostHeader() == _apIP.toString()) return false;
  _server.sendHeader("Location", String("http://") + _apIP.toString() + "/", true);
  _server.send(302, "text/plain", "");
  return true;
}

void ConfigPortal::handleRoot() {
  String html = FPSTR(PAGE_HEAD);

  html += "<label>WiFi network</label>";
  html += "<input name='ssid' list='nets' placeholder='Your WiFi SSID' "
          "autocomplete='off' required>";
  html += "<datalist id='nets'></datalist>";

  html += "<label>WiFi password</label>";
  html += "<input name='pass' type='password' placeholder='Leave blank for open network'>";

  html += "<label>Device name</label>";
  html += "<input name='name' value='" + _settings->getDeviceName() + "'>";

  html += "<div class='row'><div><label>MQTT broker (IP)</label>"
          "<input name='host' value='" + _settings->getMqttHost() + "' "
          "placeholder='192.168.0.10'></div>";
  html += "<div><label>Port</label><input name='port' type='number' value='" +
          String(_settings->getMqttPort()) + "'></div></div>";
  html += "<p class='hint'>Use the broker's IP address, not a .local name. "
          "Leave as-is if you don't use Home Assistant.</p>";

  html += "<button type='submit'>Save &amp; restart</button></form>";
  html += FPSTR(PAGE_SCRIPT);

  _server.send(200, "text/html", html);
}

void ConfigPortal::handleScan() {
  int n = WiFi.scanNetworks();
  String json = "[";
  for (int i = 0; i < n; i++) {
    if (i) json += ",";
    String ssid = WiFi.SSID(i);
    ssid.replace("\\", "\\\\");
    ssid.replace("\"", "\\\"");
    json += "{\"ssid\":\"" + ssid + "\",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
  }
  json += "]";
  WiFi.scanDelete();
  _server.send(200, "application/json", json);
}

void ConfigPortal::handleSave() {
  String ssid = _server.arg("ssid");
  String pass = _server.arg("pass");
  String name = _server.arg("name");
  String host = _server.arg("host");
  String port = _server.arg("port");

  if (ssid.length() == 0) {
    _server.send(400, "text/html",
                 "<meta http-equiv='refresh' content='2;url=/'>"
                 "WiFi SSID is required.");
    return;
  }

  _settings->setWifiSsid(ssid);
  _settings->setWifiPassword(pass);
  if (name.length()) _settings->setDeviceName(name);
  if (host.length()) _settings->setMqttHost(host);
  if (port.length()) {
    uint32_t p = port.toInt();
    if (p > 0 && p <= 65535) _settings->setMqttPort((uint16_t)p);
  }

  Serial.printf("Portal: saved WiFi '%s', rebooting\n", ssid.c_str());

  _server.send(200, "text/html",
               "<meta charset='utf-8'><body style='font-family:sans-serif;"
               "background:#0f172a;color:#e2e8f0;padding:40px;text-align:center'>"
               "<h2>Saved.</h2><p>The fan controller is restarting and will "
               "connect to your network.</p></body>");

  delay(800);
  ESP.restart();
}

void ConfigPortal::handleNotFound() {
  if (redirectToPortal()) return;
  handleRoot();
}
