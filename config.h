#ifndef CONFIG_H
#define CONFIG_H

#define FW_VERSION "1.0.0"

#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <HTTPClient.h>
#include <Update.h>
#include <time.h>
#include <vector>
#include <WebSocketsServer.h>
#include <cJSON.h>

// Global objects
extern WebServer server;
extern WebSocketsServer webSocket;
extern cJSON* flowDoc;
extern cJSON* contextDoc;
extern bool stopExecution;
extern String localIP;
extern String otaUrl;

// Configuration
extern char wifi_ssid[32];
extern char wifi_pass[32];
extern bool apMode;

// Tunnel
extern WiFiClient tunnelClient;
extern WiFiClient localClient;
extern String tunnelHost;
extern uint16_t tunnelPort;
extern String tunnelProtocol;
extern bool tunnelEnabled;

// cJSON helpers
inline cJSON* cGet(cJSON* obj, const char* key) { return cJSON_GetObjectItem(obj, key); }
inline const char* cStr(cJSON* obj, const char* key, const char* def = "") {
  cJSON* i = cGet(obj, key);
  return (i && cJSON_IsString(i)) ? i->valuestring : def;
}
inline int cInt(cJSON* obj, const char* key, int def = 0) {
  cJSON* i = cGet(obj, key);
  return (i && cJSON_IsNumber(i)) ? i->valueint : def;
}
inline bool cBool(cJSON* obj, const char* key, bool def = false) {
  cJSON* i = cGet(obj, key);
  if (!i) return def;
  if (cJSON_IsBool(i)) return cJSON_IsTrue(i);
  return def;
}
inline double cDbl(cJSON* obj, const char* key, double def = 0) {
  cJSON* i = cGet(obj, key);
  return (i && cJSON_IsNumber(i)) ? i->valuedouble : def;
}
inline void cSet(cJSON* obj, const char* key, const char* val) {
  cJSON_DeleteItemFromObject(obj, key);
  cJSON_AddStringToObject(obj, key, val);
}
inline void cSetInt(cJSON* obj, const char* key, int val) {
  cJSON_DeleteItemFromObject(obj, key);
  cJSON_AddNumberToObject(obj, key, val);
}
inline void cSetBool(cJSON* obj, const char* key, bool val) {
  cJSON_DeleteItemFromObject(obj, key);
  cJSON_AddBoolToObject(obj, key, val);
}
inline String cSerialize(cJSON* obj) {
  char* s = cJSON_PrintUnformatted(obj);
  String r = s ? String(s) : "{}";
  if(s) cJSON_free(s);
  return r;
}
inline cJSON* cParse(const String& s) { return cJSON_Parse(s.c_str()); }
inline cJSON* cParseFile(File& f) {
  size_t size = f.size();
  if (size == 0) return NULL;
  char* buf = (char*)malloc(size + 1);
  if (!buf) return NULL;
  f.readBytes(buf, size);
  buf[size] = '\0';
  cJSON* doc = cJSON_Parse(buf);
  free(buf);
  return doc;
}

#endif
