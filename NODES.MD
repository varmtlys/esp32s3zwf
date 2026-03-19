# ESP32-S3 Zero WorkFlow Nodes Documentation

This document describes the mechanics and configuration of each supported node type.

---

## 🏗 Common Mechanisms

### Dynamic Variables
Almost all configuration fields support dynamic variables using the syntax `{{node_id.path}}`.
- **node_id**: The unique ID of the node (e.g., `http_1`).
- **path**: The path to the data in the node's context.

**Examples**:
- `{{http_1.status}}` — Get the HTTP response code from `http_1`.
- `{{webhook_1.json.temp}}` — Get the `temp` field from the JSON body of an incoming webhook.
- `{{transform_1.result}}` — Get a calculated value from a transform node.

---

## ◈ Start Node (Trigger)
The flow entry point. A single flow can have multiple start nodes for different triggers.

### Modes:
- **Manual**: Triggered manually via the "Run" button in the Web UI.
- **Schedule**: Automatic execution at specific times.
  - **Type: Period**: Runs every X seconds.
    - `Interval`: Time in seconds.
    - `Immediate`: If true, runs as soon as the device boots/activates.
    - `Anchor`: `Start` (interval from flow start) or `End` (interval from flow finish).
  - **Type: Cron**: Runs according to a standard Cron expression (e.g., `*/5 * * * *` for every 5 minutes).
- **Webhook**: Waits for an external HTTP request.
  - **Path**: Custom relative path (e.g., `my/trigger`). URL becomes `https://yourdomain.com/webhook/my/trigger`.
  - **Enabled**: Global switch to enable/disable the trigger.

---

## ⇄ HTTP Request
Performs an outgoing HTTP request to any external API.

### Configuration:
- **URL**: Web address (supports variables).
- **Method**: `GET`, `POST`, `PUT`, `PATCH`, `DELETE`.
- **Headers**: Key: Value pairs, one per line.
- **Body**: Payload for POST/PUT requests (string or JSON).

### Output Context:
- `status`: HTTP response code (e.g., `200`).
- `data`: Raw response body as a string.
- `json`: Parsed JSON object (if response is valid JSON).

---

## ✉ Telegram
Sends notifications using the Telegram Bot API.

### Configuration:
- **Token**: Your BotFather API token.
- **Chat ID**: Target chat/user ID.
- **Method**:
  - `sendMessage`: Send text.
  - `sendPhoto`: Send an image from a URL.
- **Text/URL/Caption**: Content based on the selected method.

---

## ⎇ Condition (If)
Splits the flow into two branches based on a logical comparison.

### Configuration:
- **Value 1** & **Value 2**: Operands for comparison (usually variables).
- **Operator**: `==` (equal), `>` (greater than), `!=` (not equal).

### Branches:
- **True (1)**: Executed if the condition is met.
- **False (0)**: Executed if the condition is not met.

---

## ⏱ Delay
Pauses the execution of the current flow branch.

### Configuration:
- **MS**: Delay time in milliseconds (1000ms = 1s).

---

## ƒ Transform
Allows basic data manipulation and calculations using a simple script engine.

### Script Syntax:
`SET variable_name = expression`

**Example**:
```text
SET total_temp = {{sensor_1.json.t1}} + {{sensor_1.json.t2}}
SET average = {{total_temp}} / 2
```

**Supported Operations**: `+`, `-`, `*`, `/`.

---

## ⟳ Loop
Repeats a section of the flow for a specified number of times.

### Configuration:
- **Count**: Number of iterations.

### Branches:
- **Body (⟳)**: Triggered on each iteration.
- **Done (🞪)**: Triggered once the loop is finished.

### Output Context:
- `index`: Current iteration number (starts from 1).

---

## ⇱ Read Table
Reads JSON data from a local table stored in LittleFS.

### Configuration:
- **Table**: The name of the table to read (e.g., `logs`).

### Output Context:
- `data`: An array containing all rows from the table.
- `count`: Total number of rows found.

---

## ⇲ Write Table
Appends or overwrites data in a local JSON table.

### Configuration:
- **Table**: The name of the target table.
- **Mode**: `Append` (add as new row) or `Overwrite` (replace entire table).
- **Data**: JSON object representing the row to write (supports variables).

---

## ⊟ Loop Array
Iterates over each item in a data array (usually retrieved from a "Read Table" node).

### Configuration:
- **Array Variable**: The path to an array (e.g., `{{read_table_1.data}}`).

### Branches:
- **Body (⟳)**: Triggered for every item in the array.
- **Done (🞪)**: Triggered after all items have been processed.

### Output Context:
- `item`: The current element of the array.
- `index`: Current index (starts from 0).

---

## ⎋ Break Loop
Immediately terminates the current loop and continues execution from the loop's "Done" port.

### Configuration:
- **Loop ID**: The ID of the loop node you want to break (e.g., `loop_1`).
