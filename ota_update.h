#ifndef OTA_UPDATE_H
#define OTA_UPDATE_H

#include "config.h"

void loadOtaConfig() {
  if (LittleFS.exists("/ota.json")) {
    File f = LittleFS.open("/ota.json", "r");
    cJSON* doc = cParseFile(f);
    f.close();
    if (doc) {
      otaUrl = cStr(doc, "url");
      cJSON_Delete(doc);
    }
  }
}

void saveOtaConfig() {
  cJSON* doc = cJSON_CreateObject();
  cJSON_AddStringToObject(doc, "url", otaUrl.c_str());
  File f = LittleFS.open("/ota.json", "w");
  String s = cSerialize(doc);
  f.print(s);
  f.close();
  cJSON_Delete(doc);
}

String checkForUpdate() {
  if (otaUrl.length() == 0) {
    return "{\"status\":\"error\",\"message\":\"OTA URL not configured\"}";
  }

  HTTPClient http;
  String versionUrl = otaUrl;
  if (!versionUrl.endsWith("/")) versionUrl += "/";
  versionUrl += "version.json";

  http.begin(versionUrl);
  http.setTimeout(10000);
  int code = http.GET();

  if (code != 200) {
    http.end();
    return "{\"status\":\"error\",\"message\":\"HTTP " + String(code) + "\"}";
  }

  String payload = http.getString();
  http.end();

  cJSON* doc = cJSON_Parse(payload.c_str());
  if (!doc) {
    return "{\"status\":\"error\",\"message\":\"Invalid JSON\"}";
  }

  String remoteVersion = cStr(doc, "version");
  String firmwareUrl = cStr(doc, "url");
  cJSON_Delete(doc);

  if (remoteVersion == "" || firmwareUrl == "") {
    return "{\"status\":\"error\",\"message\":\"Missing version or url in JSON\"}";
  }

  bool updateAvailable = (remoteVersion != String(FW_VERSION));

  cJSON* result = cJSON_CreateObject();
  cJSON_AddStringToObject(result, "status", "ok");
  cJSON_AddStringToObject(result, "current", FW_VERSION);
  cJSON_AddStringToObject(result, "remote", remoteVersion.c_str());
  cJSON_AddBoolToObject(result, "updateAvailable", updateAvailable);
  cJSON_AddStringToObject(result, "firmwareUrl", firmwareUrl.c_str());
  String out = cSerialize(result);
  cJSON_Delete(result);
  return out;
}

String performOtaUpdate(String firmwareUrl) {
  if (firmwareUrl.length() == 0) {
    return "{\"status\":\"error\",\"message\":\"No firmware URL\"}";
  }

  webSocket.broadcastTXT("{\"event\":\"ota\",\"status\":\"downloading\"}");

  HTTPClient http;
  http.begin(firmwareUrl);
  http.setTimeout(15000);
  int httpCode = http.GET();
  
  if (httpCode != HTTP_CODE_OK) {
    http.end();
    return "{\"status\":\"error\",\"message\":\"HTTP request failed: " + String(httpCode) + "\"}";
  }

  int contentLength = http.getSize();
  if (contentLength <= 0) {
    http.end();
    return "{\"status\":\"error\",\"message\":\"Content length is 0 or unknown\"}";
  }

  bool canBegin = Update.begin(contentLength);
  if (!canBegin) {
    http.end();
    return "{\"status\":\"error\",\"message\":\"Not enough space to begin OTA\"}";
  }

  WiFiClient* stream = http.getStreamPtr();
  size_t written = Update.writeStream(*stream);

  if (written == contentLength) {
    if (Update.end()) {
      http.end();
      if (Update.isFinished()) {
        webSocket.broadcastTXT("{\"event\":\"ota\",\"status\":\"success\"}");
        return "{\"status\":\"ok\",\"message\":\"Update successful, rebooting...\"}";
      } else {
        return "{\"status\":\"error\",\"message\":\"Update not finished? Something went wrong!\"}";
      }
    } else {
      http.end();
      return "{\"status\":\"error\",\"message\":\"Error #" + String(Update.getError()) + "\"}";
    }
  } else {
    Update.end();
    http.end();
    return "{\"status\":\"error\",\"message\":\"Written only " + String(written) + "/" + String(contentLength) + "\"}";
  }
}

#endif
