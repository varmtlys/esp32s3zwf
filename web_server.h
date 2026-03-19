#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "config.h"
#include "web_ui.h"
#include "engine.h"

bool loadFlow(String name) {
  String path = "/flows/" + name + ".json";
  if (!LittleFS.exists(path)) return false;
  File f = LittleFS.open(path, "r");
  if (flowDoc) cJSON_Delete(flowDoc);
  flowDoc = cParseFile(f);
  f.close();
  return (flowDoc != NULL);
}

void setupRoutes() {
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", INDEX_HTML);
  });

  server.on("/api/flows", HTTP_GET, []() {
    cJSON* arr = cJSON_CreateArray();
    File root = LittleFS.open("/flows");
    File file = root.openNextFile();
    while(file){
      String name = String(file.name());
      if(name.endsWith(".json")) {
        cJSON* obj = cJSON_CreateObject();
        String flowName = name.substring(0, name.length() - 5);
        cJSON_AddStringToObject(obj, "name", flowName.c_str());
        
        File f = LittleFS.open("/flows/" + name, "r");
        cJSON* tmp = cParseFile(f);
        f.close();
        
        bool enabled = false;
        if (tmp) {
          cJSON* nodes = cGet(tmp, "nodes");
          if (nodes && cJSON_IsArray(nodes)) {
            cJSON* n;
            cJSON_ArrayForEach(n, nodes) {
              if (n && strcmp(cStr(n, "type"), "start") == 0) {
                cJSON* cfg = cGet(n, "config");
                if (cfg) enabled = cBool(cfg, "enabled");
                break;
              }
            }
          }
          cJSON_Delete(tmp);
        }
        cJSON_AddBoolToObject(obj, "enabled", enabled);
        cJSON_AddItemToArray(arr, obj);
      }
      file = root.openNextFile();
    }
    String out = cSerialize(arr);
    cJSON_Delete(arr);
    server.send(200, "application/json", out);
  });

  server.on("/api/flow", HTTP_GET, []() {
    String name = server.arg("name");
    if(name == "") name = "default";
    if (loadFlow(name)) {
      String out = cSerialize(flowDoc);
      server.send(200, "application/json", out);
    } else {
      server.send(404, "text/plain", "Not Found");
    }
  });

  server.on("/api/flow", HTTP_POST, []() {
    String name = server.arg("name");
    if(name == "") name = "default";
    if (server.hasArg("plain")) {
      if (flowDoc) cJSON_Delete(flowDoc);
      flowDoc = cJSON_Parse(server.arg("plain").c_str());
      if(!LittleFS.exists("/flows")) LittleFS.mkdir("/flows");
      File f = LittleFS.open("/flows/" + name + ".json", "w");
      String s = cSerialize(flowDoc);
      f.print(s);
      f.close();
      server.send(200, "text/plain", "OK");
    }
  });

  server.on("/api/flow", HTTP_DELETE, []() {
    String name = server.arg("name");
    if(name == "") { server.send(400, "text/plain", "Missing name"); return; }
    LittleFS.remove("/flows/" + name + ".json");
    server.send(200, "text/plain", "OK");
  });

  // --- Tables API ---
  server.on("/api/tables", HTTP_GET, []() {
    if(!LittleFS.exists("/tables")) LittleFS.mkdir("/tables");
    cJSON* arr = cJSON_CreateArray();
    File root = LittleFS.open("/tables");
    File file = root.openNextFile();
    while(file){
      String name = String(file.name());
      if(name.endsWith(".json")) {
        String tableName = name.substring(0, name.length() - 5);
        cJSON* item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "name", tableName.c_str());
        cJSON_AddNumberToObject(item, "size", file.size());
        cJSON_AddItemToArray(arr, item);
      }
      file = root.openNextFile();
    }
    String out = cSerialize(arr);
    cJSON_Delete(arr);
    server.send(200, "application/json", out);
  });

  server.on("/api/table", HTTP_POST, []() {
    String name = server.arg("name");
    if(name == "") { server.send(400, "text/plain", "Missing name"); return; }
    if(!LittleFS.exists("/tables")) LittleFS.mkdir("/tables");
    File f = LittleFS.open("/tables/" + name + ".json", "w");
    if (f) {
      f.print("[]");
      f.close();
      server.send(200, "text/plain", "OK");
    } else {
      server.send(500, "text/plain", "Error creating table");
    }
  });

  server.on("/api/table", HTTP_DELETE, []() {
    String name = server.arg("name");
    if(name == "") { server.send(400, "text/plain", "Missing name"); return; }
    LittleFS.remove("/tables/" + name + ".json");
    server.send(200, "text/plain", "OK");
  });

  server.on("/api/table/data", HTTP_GET, []() {
    String name = server.arg("name");
    if(name == "") { server.send(400, "text/plain", "Missing name"); return; }
    String path = "/tables/" + name + ".json";
    if (!LittleFS.exists(path)) {
      server.send(404, "application/json", "[]");
      return;
    }
    File f = LittleFS.open(path, "r");
    server.streamFile(f, "application/json");
    f.close();
  });

  server.on("/api/run", HTTP_POST, []() {
    String name = server.arg("name");
    if(name == "") name = "default";
    loadFlow(name);
    stopExecution = false;
    server.send(200, "text/plain", "Started");
    startFlowFromTrigger(false, name);
  });

  server.on("/api/flow/status", HTTP_POST, []() {
    String name = server.arg("name");
    bool status = server.arg("enabled") == "true";
    if (loadFlow(name)) {
      cJSON* nodes = cGet(flowDoc, "nodes");
      cJSON* node;
      cJSON_ArrayForEach(node, nodes) {
        if (strcmp(cStr(node, "type"), "start") == 0) {
          cSetBool(cGet(node, "config"), "enabled", status);
          break;
        }
      }
      File f = LittleFS.open("/flows/" + name + ".json", "w");
      String s = cSerialize(flowDoc);
      f.print(s);
      f.close();
      server.send(200, "text/plain", "OK");
    } else {
      server.send(404, "text/plain", "Not Found");
    }
  });

  server.on("/api/stop", HTTP_POST, []() {
    stopExecution = true;
    server.send(200, "text/plain", "Stopping");
  });

  server.on("/api/wifi", HTTP_POST, []() {
    if (server.hasArg("plain")) {
      File f = LittleFS.open("/wifi.json", "w");
      f.print(server.arg("plain"));
      f.close();
      server.send(200, "text/plain", "OK");
      delay(500);
      ESP.restart();
    }
  });

  server.on("/api/status", HTTP_GET, []() {
    cJSON* doc = cJSON_CreateObject();
    cJSON_AddBoolToObject(doc, "apMode", apMode);
    cJSON_AddStringToObject(doc, "ssid", wifi_ssid);
    cJSON_AddStringToObject(doc, "pass", wifi_pass);
    cJSON_AddNumberToObject(doc, "heap", ESP.getFreeHeap());
    cJSON_AddStringToObject(doc, "version", FW_VERSION);
    cJSON_AddBoolToObject(doc, "tunnel", tunnelEnabled);
    String out = cSerialize(doc);
    cJSON_Delete(doc);
    server.send(200, "application/json", out);
  });

  // Tunnel Config Routes
  server.on("/api/tunnel/config", HTTP_GET, []() {
    cJSON* doc = cJSON_CreateObject();
    cJSON_AddBoolToObject(doc, "enabled", tunnelEnabled);
    cJSON_AddStringToObject(doc, "host", tunnelHost.c_str());
    cJSON_AddNumberToObject(doc, "port", tunnelPort);
    cJSON_AddStringToObject(doc, "protocol", tunnelProtocol.c_str());
    String out = cSerialize(doc);
    cJSON_Delete(doc);
    server.send(200, "application/json", out);
  });

  server.on("/api/tunnel/config", HTTP_POST, []() {
    if (server.hasArg("plain")) {
      cJSON* doc = cJSON_Parse(server.arg("plain").c_str());
      if (doc) {
        tunnelEnabled = cBool(doc, "enabled");
        tunnelHost = cStr(doc, "host", "domain.com");
        tunnelPort = cInt(doc, "port", 9000);
        tunnelProtocol = cStr(doc, "protocol", "http");
        
        File f = LittleFS.open("/tunnel.json", "w");
        String s = cSerialize(doc);
        f.print(s);
        f.close();
        cJSON_Delete(doc);
      }
      tunnelClient.stop();
      localClient.stop();
      server.send(200, "text/plain", "OK");
    }
  });

  // OTA Routes
  server.on("/api/ota/config", HTTP_GET, []() {
    cJSON* doc = cJSON_CreateObject();
    cJSON_AddStringToObject(doc, "url", otaUrl.c_str());
    cJSON_AddStringToObject(doc, "version", FW_VERSION);
    String out = cSerialize(doc);
    cJSON_Delete(doc);
    server.send(200, "application/json", out);
  });

  server.on("/api/ota/config", HTTP_POST, []() {
    if (server.hasArg("plain")) {
      cJSON* doc = cJSON_Parse(server.arg("plain").c_str());
      if (doc) {
        otaUrl = cStr(doc, "url");
        cJSON_Delete(doc);
      }
      saveOtaConfig();
      server.send(200, "text/plain", "OK");
    }
  });

  server.on("/api/ota/check", HTTP_GET, []() {
    String result = checkForUpdate();
    server.send(200, "application/json", result);
  });

  server.on("/api/ota/update", HTTP_POST, []() {
    if (!server.hasArg("plain")) {
      server.send(400, "text/plain", "Missing body");
      return;
    }
    cJSON* doc = cJSON_Parse(server.arg("plain").c_str());
    String firmwareUrl = doc ? String(cStr(doc, "url")) : "";
    if (doc) cJSON_Delete(doc);
    
    String result = performOtaUpdate(firmwareUrl);
    server.send(200, "application/json", result);
    
    cJSON* res = cJSON_Parse(result.c_str());
    if (res) {
      if (strcmp(cStr(res, "status"), "ok") == 0) {
        cJSON_Delete(res);
        delay(1000);
        ESP.restart();
      }
      cJSON_Delete(res);
    }
  });

  server.onNotFound([]() {
    String uri = server.uri();
    broadcastEvent("{\"event\":\"log\",\"msg\":\"🔍 Server: Incoming request " + uri + "\"}");
    
    bool isDirectWebhook = uri.startsWith("/webhook/");
    
    if (isDirectWebhook) {
      auto normalize = [](String p) {
        p.trim();
        while(p.startsWith("/")) p = p.substring(1);
        while(p.endsWith("/")) p = p.substring(0, p.length() - 1);
        return p;
      };

      int flowStart = 9;
      String triggerPath = normalize(uri.substring(flowStart));
      broadcastEvent("{\"event\":\"log\",\"msg\":\"🔔 Webhook logic: matching normalized path [" + triggerPath + "]\"}");
      
      File root = LittleFS.open("/flows");
      File file = root.openNextFile();
      int matchCount = 0;
      int flowCount = 0;
      while (file) {
        String flowFilename = String(file.name());
        if (flowFilename.endsWith(".json")) {
           flowCount++;
           String flowName = flowFilename.substring(0, flowFilename.length() - 5);
           
           File f = LittleFS.open("/flows/" + flowFilename, "r");
           cJSON* doc = cParseFile(f);
           f.close();
           if (doc) {
              cJSON* nodes = cGet(doc, "nodes");
              cJSON* n;
              cJSON_ArrayForEach(n, nodes) {
                if (strcmp(cStr(n, "type"), "start") == 0) {
                  cJSON* cfg = cGet(n, "config");
                  if (strcmp(cStr(cfg, "mode"), "webhook") == 0) {
                    String nodePath = normalize(cStr(cfg, "path", ""));
                    String nodeId = cStr(n, "id");
                    if (triggerPath == nodeId || (nodePath.length() > 0 && triggerPath == nodePath) || (nodePath.length() == 0 && triggerPath == "")) {
                      if (cBool(cfg, "enabled")) {
                        broadcastEvent("{\"event\":\"log\",\"msg\":\"🚀 Match! Queuing " + flowName + " (Node: " + nodeId + ")\"}");
                        
                        WebhookRequest req;
                        req.flowName = flowName;
                        req.nodeId = nodeId;
                        req.body = server.hasArg("plain") ? server.arg("plain") : "";
                        webhookQueue.push_back(req);
                        matchCount++;
                      } else {
                        broadcastEvent("{\"event\":\"log\",\"msg\":\"⚠️ " + flowName + " matches but is DISABLED.\"}");
                      }
                    }
                  }
                }
              }
              cJSON_Delete(doc);
           }
        }
        file = root.openNextFile();
      }
      if (matchCount > 0) {
        server.send(200, "text/plain", "OK Triggered " + String(matchCount) + " flows");
      } else {
        broadcastEvent("{\"event\":\"log\",\"msg\":\"❓ No match found for " + triggerPath + " (searched " + String(flowCount) + " files)\"}");
        server.send(404, "text/plain", "Not Found");
      }
      return;
    }
    else if (apMode) {
      server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/", true);
      server.send(302, "text/plain", "");
    } else {
      broadcastEvent("{\"event\":\"log\",\"msg\":\"❓ 404: " + uri + "\"}");
      server.send(404, "text/plain", "Not Found");
    }
  });
}

#endif
