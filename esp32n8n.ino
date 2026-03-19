#include "config.h"
#include "web_ui.h"
#include "engine.h"
#include "ota_update.h"
#include "web_server.h"

// Initialize globals
WebServer server(80);
WebSocketsServer webSocket(81);
cJSON* flowDoc = NULL;
cJSON* contextDoc = NULL;

char wifi_ssid[32] = "ESP32-WorkFlow";
char wifi_pass[32] = "12345678";
bool apMode = false;
bool stopExecution = false;
String localIP = "0.0.0.0";
String otaUrl = "";

// Tunnel globals
WiFiClient tunnelClient;
WiFiClient localClient;
String tunnelHost = "domain.com";
uint16_t tunnelPort = 9000;
String tunnelProtocol = "http";
bool tunnelEnabled = false;

void loadTunnelConfig() {
  if (LittleFS.exists("/tunnel.json")) {
    File f = LittleFS.open("/tunnel.json", "r");
    cJSON* doc = cParseFile(f);
    f.close();
    if (doc) {
      tunnelEnabled = cBool(doc, "enabled");
      tunnelHost = cStr(doc, "host", "domain.com");
      tunnelPort = cInt(doc, "port", 9000);
      tunnelProtocol = cStr(doc, "protocol", "http");
      cJSON_Delete(doc);
    }
  }
}

String getResetReason() {
  switch (esp_reset_reason()) {
    case ESP_RST_POWERON: return "Power On";
    case ESP_RST_EXT: return "External Pin";
    case ESP_RST_SW: return "Software";
    case ESP_RST_PANIC: return "Panic";
    case ESP_RST_INT_WDT: return "Int. WDT";
    case ESP_RST_TASK_WDT: return "Task WDT";
    case ESP_RST_WDT: return "Other WDT";
    case ESP_RST_DEEPSLEEP: return "Deep Sleep";
    case ESP_RST_BROWNOUT: return "Brownout";
    case ESP_RST_SDIO: return "SDIO";
    default: return "Unknown";
  }
}

void setup() {
  if (!LittleFS.begin(true)) {
    return;
  }

  // Load WiFi Config
  if (LittleFS.exists("/wifi.json")) {
    File f = LittleFS.open("/wifi.json", "r");
    cJSON* doc = cParseFile(f);
    f.close();
    if (doc) {
      strncpy(wifi_ssid, cStr(doc, "ssid", "ESP32-WorkFlow"), 31);
      strncpy(wifi_pass, cStr(doc, "pass", "12345678"), 31);
      cJSON_Delete(doc);
    }
  }

  WiFi.begin(wifi_ssid, wifi_pass);
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) { 
    delay(500); 
    retry++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    localIP = WiFi.localIP().toString();
  } else {
    WiFi.softAP("ESP32-WorkFlow-Config", "12345678");
    apMode = true;
  }

  if (!LittleFS.exists("/flows")) LittleFS.mkdir("/flows");
  
  loadOtaConfig();
  loadTunnelConfig();

  configTime(0, 0, "pool.ntp.org", "time.google.com");
  broadcastEvent("{\"event\":\"log\",\"msg\":\"⏱ NTP: Syncing time...\"}");

  setupRoutes();
  server.begin();
  
  webSocket.begin();
  webSocket.onEvent([](uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  });
}

unsigned long lastTick = 0;
unsigned long lastTunnelConnect = 0;

void loop() {
  server.handleClient();
  webSocket.loop();

  // --- Webhook Tunnel Proxy Logic ---
  if (tunnelEnabled && WiFi.status() == WL_CONNECTED) {
    if (!tunnelClient.connected()) {
      if (millis() - lastTunnelConnect > 1000) {
        lastTunnelConnect = millis();
        if (tunnelClient.connect(tunnelHost.c_str(), tunnelPort)) {
          broadcastEvent("{\"event\":\"log\",\"msg\":\"🗹 Tunnel connected: " + tunnelHost + "\"}");
        }
      }
    } else {
      // Data from Tunnel -> Local
      if (tunnelClient.available()) {
        if (!localClient.connected()) {
          String connectIP = "127.0.0.1";
          localClient.connect(connectIP.c_str(), 80);
          broadcastEvent("{\"event\":\"log\",\"msg\":\"🗘 Tunnel passing data to local (" + connectIP + ")\"}");
        }
        uint8_t buf[1024];
        int len = tunnelClient.read(buf, 1024);
        if (len > 0) {
          localClient.write(buf, len);
        }
      }
      // Data from Local -> Tunnel
      if (localClient.available()) {
        uint8_t buf[1024];
        int len = localClient.read(buf, 1024);
        if (len > 0) {
          tunnelClient.write(buf, len);
        }
      }
      
      if (!tunnelClient.connected() && localClient.connected()) {
        localClient.stop();
        broadcastEvent("{\"event\":\"log\",\"msg\":\"⚠ Tunnel closed.\"}");
      }
    }
  }
  
  static unsigned long lastStats = 0;
  if (millis() - lastStats > 1000) {
    lastStats = millis();
    String stats = "{\"event\":\"stats\",\"heap\":" + String(ESP.getFreeHeap()) + 
                   ",\"psram\":" + String(ESP.getFreePsram()) + 
                   ",\"fs_used\":" + String(LittleFS.usedBytes()) + 
                   ",\"fs_total\":" + String(LittleFS.totalBytes()) + 
                   ",\"uptime\":" + String(millis()/1000) + 
                   ",\"ip\":\"" + (apMode ? WiFi.softAPIP().toString() : localIP) + 
                   "\",\"ssid\":\"" + String(wifi_ssid) + 
                   "\",\"rssi\":" + String(WiFi.RSSI()) + 
                   ",\"cpu\":" + String(ESP.getCpuFreqMHz()) + 
                   ",\"flash\":" + String(ESP.getFlashChipSize() / (1024 * 1024)) + 
                   ",\"sdk\":\"" + String(ESP.getSdkVersion()) + 
                   "\",\"reset\":\"" + getResetReason() + 
                   "\",\"temp\":" + String(temperatureRead(), 1) + "}";
    webSocket.broadcastTXT(stats);
  }

  // Background Webhooks Queue - RUN EVERY TICK
  processWebhookQueue(); 
  
  if (millis() - lastTick > 1000) {
    lastTick = millis();
    checkAndRunSchedules();
  }
}
