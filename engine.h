#ifndef ENGINE_H
#define ENGINE_H

#include "config.h"

struct WebhookRequest {
  String flowName;
  String nodeId;
  String body;
};

std::vector<WebhookRequest> webhookQueue;

void broadcastEvent(String json) {
  webSocket.broadcastTXT(json);
}

bool cronMatch(const char* cron, struct tm* t) {
  if (!cron || strlen(cron) < 5) return false;
  char buf[64];
  strncpy(buf, cron, 63);
  buf[63] = '\0';
  
  char* fields[5];
  int i = 0;
  char* saveptr1;
  char* p = strtok_r(buf, " ", &saveptr1);
  while (p && i < 5) {
    fields[i++] = p;
    p = strtok_r(NULL, " ", &saveptr1);
  }
  if (i < 5) return false;
  
  auto match = [](const char* f, int val) -> bool {
    if (strcmp(f, "*") == 0) return true;
    if (strchr(f, ',')) {
      char f2[32]; strncpy(f2, f, 31); f2[31] = '\0';
      char* saveptr2;
      char* token = strtok_r(f2, ",", &saveptr2);
      while (token) {
        if (atoi(token) == val) return true;
        token = strtok_r(NULL, ",", &saveptr2);
      }
      return false;
    }
    return (atoi(f) == val);
  };
  
  if (!match(fields[0], t->tm_min)) return false;
  if (!match(fields[1], t->tm_hour)) return false;
  if (!match(fields[2], t->tm_mday)) return false;
  if (!match(fields[3], t->tm_mon + 1)) return false;
  if (!match(fields[4], t->tm_wday)) return false;
  
  return true;
}

String resolveVariables(String input, bool urlEncode = false) {
  bool found = true;
  int passes = 0;
  while (found && passes < 5) {
    found = false;
    passes++;
    int start = input.indexOf("{{");
    while (start != -1) {
      int end = input.indexOf("}}", start);
      if (end == -1) break;
      
      String placeholder = input.substring(start + 2, end);
      int dot = placeholder.indexOf('.');
      if (dot != -1) {
        String nodeId = placeholder.substring(0, dot);
        String path = placeholder.substring(dot + 1);
        
        cJSON* nodeCtx = cGet(contextDoc, nodeId.c_str());
        if (nodeCtx) {
          cJSON* current = nodeCtx;
          int lastDot = 0;
          int nextDot = path.indexOf('.');
          bool fail = false;
          while (nextDot != -1) {
            String part = path.substring(lastDot, nextDot);
            cJSON* child = cGet(current, part.c_str());
            if (child) {
              current = child;
              lastDot = nextDot + 1;
              nextDot = path.indexOf('.', lastDot);
            } else {
              fail = true; break;
            }
          }
          if (!fail) {
            String lastPart = path.substring(lastDot);
            cJSON* val = cGet(current, lastPart.c_str());
            if (val) {
              String value;
              if (cJSON_IsString(val)) value = val->valuestring;
              else if (cJSON_IsNumber(val)) value = String(val->valuedouble);
              else if (cJSON_IsBool(val)) value = cJSON_IsTrue(val) ? "true" : "false";
              
              if (urlEncode) {
                value.replace(" ", "%20");
                value.replace("\n", "%0A");
              }
              input = input.substring(0, start) + value + input.substring(end + 2);
              found = true;
              start = input.indexOf("{{", start);
              if (start == -1) break;
              continue;
            }
          }
        }
      }
      start = input.indexOf("{{", start + 1);
    }
  }
  return input;
}

void startFlowFromNodes(std::vector<String> queue, String flowName = "") {
  int safetyCounter = 0;
  cJSON* nodes = cGet(flowDoc, "nodes");
  cJSON* conns = cGet(flowDoc, "connections");
  
  broadcastEvent("{\"event\":\"log\",\"msg\":\"⚙ Engine: Starting execution for " + flowName + " (" + String(queue.size()) + " entry points)\"}");

  while (queue.size() > 0 && safetyCounter < 1000) {
    if (stopExecution) {
      broadcastEvent("{\"event\":\"log\",\"msg\":\"⚠ Engine: Execution STOPPED by user flag.\"}");
      broadcastEvent("{\"event\":\"stopped\"}");
      break;
    }
    String nodeId = queue[0];
    queue.erase(queue.begin());
    safetyCounter++;

    cJSON* node = NULL;
    cJSON* item;
    cJSON_ArrayForEach(item, nodes) {
      if (strcmp(cStr(item, "id"), nodeId.c_str()) == 0) { node = item; break; }
    }
    if (!node) {
      broadcastEvent("{\"event\":\"log\",\"msg\":\"⚠ Engine: Node " + nodeId + " not found in flow definition.\"}");
      continue;
    }

    String type = cStr(node, "type");
    cJSON* config = cGet(node, "config");
    
    broadcastEvent("{\"event\":\"executing\",\"nodeId\":\"" + nodeId + "\"}");
    broadcastEvent("{\"event\":\"log\",\"msg\":\"► Executing: " + nodeId + " [" + type + "]\"}");

    String branchToTake = "out";

    if (type == "http") {
      HTTPClient http;
      String url = resolveVariables(String(cStr(config, "url")), true);
      broadcastEvent("{\"event\":\"log\",\"msg\":\"⇄ HTTP Request to: " + url + "\"}");
      http.begin(url);
      http.setTimeout(10000); 
      String method = cStr(config, "method", "GET");
      
      String headersStr = cStr(config, "headers", "");
      if (headersStr.length() > 0) {
        headersStr = resolveVariables(headersStr);
        int lastIdx = 0;
        while (lastIdx < (int)headersStr.length()) {
          int nextLine = headersStr.indexOf('\n', lastIdx);
          if (nextLine == -1) nextLine = headersStr.length();
          String line = headersStr.substring(lastIdx, nextLine);
          line.trim();
          int colon = line.indexOf(':');
          if (colon != -1) {
             String hKey = line.substring(0, colon); hKey.trim();
             String hVal = line.substring(colon + 1); hVal.trim();
             http.addHeader(hKey, hVal);
          }
          lastIdx = nextLine + 1;
        }
      }

      int code = -1;
      String bodyStr = resolveVariables(String(cStr(config, "body")));
      if (method == "POST") code = http.POST(bodyStr);
      else if (method == "PUT") code = http.PUT(bodyStr);
      else if (method == "PATCH") code = http.PATCH(bodyStr);
      else if (method == "DELETE") code = http.sendRequest("DELETE", bodyStr);
      else code = http.GET();
      
      broadcastEvent("{\"event\":\"log\",\"msg\":\"⇄ HTTP Response Code: " + String(code) + "\"}");

      cJSON* ctx = cGet(contextDoc, nodeId.c_str());
      if (!ctx) { ctx = cJSON_CreateObject(); cJSON_AddItemToObject(contextDoc, nodeId.c_str(), ctx); }
      cSetInt(ctx, "status", code);
      if (code > 0) {
        String payload = http.getString();
        cSet(ctx, "data", payload.c_str());
        cJSON* parsed = cJSON_Parse(payload.c_str());
        if (parsed) {
          cJSON_DeleteItemFromObject(ctx, "json");
          cJSON_AddItemToObject(ctx, "json", parsed);
        }
      }
      http.end();
    } 
    else if (type == "delay") {
      int ms = cInt(config, "ms", 1000);
      if (ms <= 0) ms = 1000;
      broadcastEvent("{\"event\":\"log\",\"msg\":\"⏱ Delaying " + String(ms) + "ms...\"}");
      int elapsed = 0;
      while (elapsed < ms) {
        if (stopExecution) break;
        delay(50);
        yield();
        elapsed += 50;
      }
    } 
    else if (type == "condition") {
      String v1 = resolveVariables(String(cStr(config, "value1")));
      String v2 = resolveVariables(String(cStr(config, "value2")));
      String op = cStr(config, "op", "==");
      bool res = (op == "==") ? (v1 == v2) : (op == ">" ? v1.toFloat() > v2.toFloat() : v1 != v2);
      broadcastEvent("{\"event\":\"log\",\"msg\":\"⎇ Condition: '" + v1 + "' " + op + " '" + v2 + "' -> " + (res ? "TRUE" : "FALSE") + "\"}");
      
      cJSON* ctx = cGet(contextDoc, nodeId.c_str());
      if (!ctx) { ctx = cJSON_CreateObject(); cJSON_AddItemToObject(contextDoc, nodeId.c_str(), ctx); }
      cSetBool(ctx, "result", res);
      branchToTake = res ? "true" : "false";
    }
    else if (type == "telegram") {
      HTTPClient http;
      String token = resolveVariables(String(cStr(config, "token")));
      String method = cStr(config, "method", "sendMessage");
      String url = "https://api.telegram.org/bot" + token + "/" + method;
      broadcastEvent("{\"event\":\"log\",\"msg\":\"✉ Telegram: Sending " + method + "...\"}");
      
      cJSON* bodyDoc = cJSON_CreateObject();
      cJSON_AddStringToObject(bodyDoc, "chat_id", resolveVariables(String(cStr(config, "chat_id"))).c_str());
      
      if (method == "sendMessage") {
        cJSON_AddStringToObject(bodyDoc, "text", resolveVariables(String(cStr(config, "text"))).c_str());
      } else if (method == "sendPhoto") {
        cJSON_AddStringToObject(bodyDoc, "photo", resolveVariables(String(cStr(config, "url"))).c_str());
        cJSON_AddStringToObject(bodyDoc, "caption", resolveVariables(String(cStr(config, "caption"))).c_str());
      }
      
      String body = cSerialize(bodyDoc);
      cJSON_Delete(bodyDoc);
      
      http.begin(url);
      http.addHeader("Content-Type", "application/json");
      int code = http.POST(body);
      broadcastEvent("{\"event\":\"log\",\"msg\":\"✉ Telegram Response: " + String(code) + "\"}");
      
      cJSON* ctx = cGet(contextDoc, nodeId.c_str());
      if (!ctx) { ctx = cJSON_CreateObject(); cJSON_AddItemToObject(contextDoc, nodeId.c_str(), ctx); }
      cSetInt(ctx, "status", code);
      if (code > 0) {
        cSet(ctx, "data", http.getString().c_str());
      }
      http.end();
    }
    else if (type == "transform") {
      String script = cStr(config, "script");
      broadcastEvent("{\"event\":\"log\",\"msg\":\"ƒ Running script...\"}");
      int lastIdx = 0;
      while (lastIdx < (int)script.length()) {
        if (stopExecution) break;
        int nextLine = script.indexOf('\n', lastIdx);
        if (nextLine == -1) nextLine = script.length();
        String line = script.substring(lastIdx, nextLine);
        line.trim();
        if (line.startsWith("SET ")) {
          int eq = line.indexOf('=');
          if (eq != -1) {
            String key = line.substring(4, eq); key.trim();
            String expr = line.substring(eq + 1); expr.trim();
            expr = resolveVariables(expr);
            
            cJSON* ctx = cGet(contextDoc, nodeId.c_str());
            if (!ctx) { ctx = cJSON_CreateObject(); cJSON_AddItemToObject(contextDoc, nodeId.c_str(), ctx); }
            
            if (expr.indexOf('+') != -1) {
              cJSON_DeleteItemFromObject(ctx, key.c_str());
              cJSON_AddNumberToObject(ctx, key.c_str(), expr.substring(0, expr.indexOf('+')).toFloat() + expr.substring(expr.indexOf('+')+1).toFloat());
            } else if (expr.indexOf('-') != -1) {
              cJSON_DeleteItemFromObject(ctx, key.c_str());
              cJSON_AddNumberToObject(ctx, key.c_str(), expr.substring(0, expr.indexOf('-')).toFloat() - expr.substring(expr.indexOf('-')+1).toFloat());
            } else if (expr.indexOf('*') != -1) {
              cJSON_DeleteItemFromObject(ctx, key.c_str());
              cJSON_AddNumberToObject(ctx, key.c_str(), expr.substring(0, expr.indexOf('*')).toFloat() * expr.substring(expr.indexOf('*')+1).toFloat());
            } else if (expr.indexOf('/') != -1) {
              float v2 = expr.substring(expr.indexOf('/')+1).toFloat();
              cJSON_DeleteItemFromObject(ctx, key.c_str());
              cJSON_AddNumberToObject(ctx, key.c_str(), (v2 != 0) ? expr.substring(0, expr.indexOf('/')).toFloat() / v2 : 0);
            } else {
              cSet(ctx, key.c_str(), expr.c_str());
            }
          }
        }
        lastIdx = nextLine + 1;
      }
    }
    else if (type == "loop") {
      int count = cInt(config, "count", 1);
      int current = 0;
      cJSON* ctx = cGet(contextDoc, nodeId.c_str());
      if (ctx) current = cInt(ctx, "index", 0);
      else { ctx = cJSON_CreateObject(); cJSON_AddItemToObject(contextDoc, nodeId.c_str(), ctx); }
      
      if (current < count) {
        cSetInt(ctx, "index", current + 1);
        branchToTake = "body";
        broadcastEvent("{\"event\":\"log\",\"msg\":\"⟳ Loop " + nodeId + ": " + String(current + 1) + "/" + String(count) + "\"}");
      } else {
        cSetInt(ctx, "index", 0);
        branchToTake = "done";
      }
    }

    cJSON* conn;
    int nextCount = 0;
    cJSON_ArrayForEach(conn, conns) {
      if (strcmp(cStr(conn, "from"), nodeId.c_str()) == 0) {
        String fromPort = cStr(conn, "fromPort", "out");
        if (fromPort == branchToTake) {
          queue.push_back(String(cStr(conn, "to")));
          nextCount++;
        }
      }
    }
    if (nextCount > 0) broadcastEvent("{\"event\":\"log\",\"msg\":\"➔ Node " + nodeId + " finished, triggered " + String(nextCount) + " next nodes.\"}");
    else broadcastEvent("{\"event\":\"log\",\"msg\":\"⏹ Node " + nodeId + " finished, no further connections.\"}");
    
    yield();
  }

  unsigned long now = millis() / 1000;
  bool changed = false;
  cJSON* nodesArr = cGet(flowDoc, "nodes");
  cJSON* n;
  cJSON_ArrayForEach(n, nodesArr) {
    if (strcmp(cStr(n, "type"), "start") == 0) {
      cJSON* cfg = cGet(n, "config");
      if (cfg) { cSetInt(cfg, "last_finished", now); changed = true; }
      break;
    }
  }
  
  if (changed && flowName != "") {
    File f = LittleFS.open("/flows/" + flowName + ".json", "w");
    if (f) { String s = cSerialize(flowDoc); f.print(s); f.close(); }
  }

  broadcastEvent("{\"event\":\"finished\"}");
  broadcastEvent("{\"event\":\"log\",\"msg\":\"🗹 Engine: Flow " + flowName + " execution FINISHED.\"}");
}

void startFlowFromTrigger(bool isSchedule = false, String flowName = "") {
  if (contextDoc) cJSON_Delete(contextDoc);
  contextDoc = cJSON_CreateObject();
  
  std::vector<String> queue;
  unsigned long now = millis() / 1000;
  time_t now_t = time(NULL);
  struct tm timeinfo;
  bool timeValid = getLocalTime(&timeinfo);
  bool changed = false;

  cJSON* nodesArr = cGet(flowDoc, "nodes");
  cJSON* node;
  cJSON_ArrayForEach(node, nodesArr) {
    if (strcmp(cStr(node, "type"), "start") != 0) continue;
    cJSON* config = cGet(node, "config");
    String mode = cStr(config, "mode", "manual");
    
    if (isSchedule) {
      if (mode == "schedule" && cBool(config, "enabled")) {
        String schedType = cStr(config, "schedType", "period");
        bool shouldRun = false;

        if (schedType == "cron") {
          if (timeValid) {
            String cronStr = cStr(config, "cron", "* * * * *");
            uint32_t lastRun = cInt(config, "last_run", 0);
            // Check if it matches cron AND hasn't run in this minute already
            if (cronMatch(cronStr.c_str(), &timeinfo)) {
               // lastRun is unix timestamp. Check if same minute.
               if (now_t / 60 > lastRun / 60) {
                 shouldRun = true;
               }
            }
          }
        } else {
          // Period mode
          uint32_t interval = cInt(config, "interval", 60);
          uint32_t lastRun = cInt(config, "last_run", 0);
          uint32_t lastFinished = cInt(config, "last_finished", 0);
          String anchor = cStr(config, "anchor", "start");
          bool immediate = cBool(config, "immediate");

          if (lastRun == 0) {
            if (immediate) shouldRun = true;
            else { cSetInt(config, "last_run", now_t); cSetInt(config, "last_finished", now_t); changed = true; }
          } else {
            uint32_t baseTime = (anchor == "end") ? lastFinished : lastRun;
            if (now_t - baseTime >= interval) shouldRun = true;
          }
        }

        if (shouldRun) {
          cSetInt(config, "last_run", now_t);
          queue.push_back(String(cStr(node, "id")));
          changed = true;
        }
      }
    } else {
      queue.push_back(String(cStr(node, "id")));
      cSetInt(config, "last_run", now_t);
      changed = true;
    }
  }

  if (changed && flowName != "") {
    File f = LittleFS.open("/flows/" + flowName + ".json", "w");
    if (f) { String s = cSerialize(flowDoc); f.print(s); f.close(); }
  }

  if (queue.size() > 0) startFlowFromNodes(queue, flowName);
}

void startFlowFromNode(String nodeId, String flowName = "") {
  std::vector<String> queue;
  queue.push_back(nodeId);
  startFlowFromNodes(queue, flowName);
}

void processWebhookQueue() {
  if (webhookQueue.empty()) return;
  
  WebhookRequest req = webhookQueue[0];
  webhookQueue.erase(webhookQueue.begin());
  
  broadcastEvent("{\"event\":\"log\",\"msg\":\"🗘 Processing background webhook for flow: " + req.flowName + "\"}");

  File f = LittleFS.open("/flows/" + req.flowName + ".json", "r");
  cJSON* doc = cParseFile(f);
  f.close();
  
  if (doc) {
    cJSON* savedFlow = flowDoc;
    flowDoc = doc;
    if (contextDoc) cJSON_Delete(contextDoc);
    contextDoc = cJSON_CreateObject();
    
    cJSON* nodeCtx = cJSON_CreateObject();
    cJSON_AddStringToObject(nodeCtx, "body", req.body.c_str());
    
    // Auto-parse JSON body
    cJSON* parsed = cJSON_Parse(req.body.c_str());
    if (parsed) {
      cJSON_AddItemToObject(nodeCtx, "json", parsed);
    }
    
    cJSON_AddItemToObject(contextDoc, req.nodeId.c_str(), nodeCtx);
    
    stopExecution = false;
    startFlowFromNode(req.nodeId, req.flowName);
    
    cJSON_Delete(flowDoc);
    flowDoc = savedFlow;
  }
}

void checkAndRunSchedules() {
  File root = LittleFS.open("/flows");
  File file = root.openNextFile();
  time_t now_t = time(NULL);
  struct tm timeinfo;
  bool timeValid = getLocalTime(&timeinfo);

  while (file) {
    String name = String(file.name());
    if (name.endsWith(".json")) {
      String flowName = name.substring(0, name.length() - 5);
      File f = LittleFS.open("/flows/" + name, "r");
      cJSON* tmpDoc = cParseFile(f);
      f.close();
      if (!tmpDoc) { file = root.openNextFile(); continue; }

      bool changed = false;
      bool shouldRun = false;
      std::vector<String> queue;

      cJSON* nodesArr = cGet(tmpDoc, "nodes");
      if (nodesArr) {
        cJSON* node;
        cJSON_ArrayForEach(node, nodesArr) {
          if (strcmp(cStr(node, "type"), "start") == 0) {
            cJSON* config = cGet(node, "config");
            if (strcmp(cStr(config, "mode"), "schedule") == 0 && cBool(config, "enabled")) {
               String schedType = cStr(config, "schedType", "period");
               
               if (schedType == "cron") {
                 if (timeValid) {
                   String cronStr = cStr(config, "cron", "* * * * *");
                   uint32_t lastRun = cInt(config, "last_run", 0);
                   if (cronMatch(cronStr.c_str(), &timeinfo)) {
                     if (now_t / 60 > lastRun / 60) {
                       shouldRun = true;
                     }
                   }
                 }
               } else {
                 uint32_t interval = cInt(config, "interval", 60);
                 uint32_t lastRun = cInt(config, "last_run", 0);
                 uint32_t lastFinished = cInt(config, "last_finished", 0);
                 String anchor = cStr(config, "anchor", "start");
                 bool immediate = cBool(config, "immediate");
                 if (lastRun == 0) {
                   if (immediate) shouldRun = true;
                   else { cSetInt(config, "last_run", now_t); cSetInt(config, "last_finished", now_t); changed = true; }
                 } else {
                   uint32_t baseTime = (anchor == "end") ? lastFinished : lastRun;
                   if (now_t - baseTime >= interval) { shouldRun = true; }
                 }
               }

               if (shouldRun) {
                  cSetInt(config, "last_run", now_t);
                  queue.push_back(String(cStr(node, "id")));
                  changed = true;
               }
            }
          }
        }
      }

      if (changed) {
         File fw = LittleFS.open("/flows/" + name, "w");
         if (fw) { String s = cSerialize(tmpDoc); fw.print(s); fw.close(); }
      }
      cJSON_Delete(tmpDoc);

      if (shouldRun && queue.size() > 0) {
         cJSON* savedFlow = flowDoc;
         flowDoc = NULL;
         File fr = LittleFS.open("/flows/" + name, "r");
         flowDoc = cParseFile(fr);
         fr.close();
         if (contextDoc) cJSON_Delete(contextDoc);
         contextDoc = cJSON_CreateObject();
         stopExecution = false;
         startFlowFromNodes(queue, flowName);
         if (flowDoc) cJSON_Delete(flowDoc);
         flowDoc = savedFlow;
      }
    }
    file = root.openNextFile();
  }
  root.close();
}

#endif
