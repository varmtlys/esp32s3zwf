> [!CAUTION]
> **Warning**: The code in this branch is currently **UNTESTED** and may be unstable or behave unexpectedly. Use it at your own risk.

# ESP32-S3 Zero WorkFlow

**ESP32-S3 Zero WorkFlow** — a powerful and lightweight workflow automation platform inspired by n8n, but optimized to run directly on ESP32-S3 microcontrollers. It allows you to create, edit, and execute complex automation scenarios (flows) directly on the device through a user-friendly visual web interface.

## ◈ Key Features

- 🖊 **Visual Flow Editor**: Build scenarios by dragging and dropping nodes right in your browser.
- ⚙ **Execution Engine**: High-performance execution engine with support for logic branching and loops.
- ↩ **Webhook Tunneling**: Built-in support for proxying incoming webhooks via an external server (allows operation behind NAT/Firewalls without port forwarding).
- ⏱ **Scheduler**: Execute flows by intervals or using the powerful Cron syntax.
- π **Dynamic Variables**: Pass data between nodes using the `{{nodeId.path}}` syntax (e.g., `{{http_1.json.temp}}`).
- 👁 **Real-time Monitoring**: Monitor execution logs, heap, CPU, and Wi-Fi state in real-time via WebSockets.
- 🜁 **OTA Updates**: Update firmware remotely through the control panel.

## 🖧 Supported Nodes

1. **◈ Start**: Entry point. Supports manual triggers, schedules (Period/Cron), or Webhooks.
2. **⇄ HTTP Request**: Perform GET, POST, PUT, DELETE requests to external APIs.
3. **✉ Telegram**: Send notifications and photos via Telegram Bot API.
4. **⎇ Condition (If)**: Split the flow into `true`/`false` branches.
5. **⏱ Delay**: Pause flow execution.
6. **ƒ Transform**: Scripted data processing and arithmetic.
7. **⟳ Loop**: Repeat a section of the flow N times.
8. **⇱ Read Table**: Read JSON data from local storage.
9. **⇲ Write Table**: Save or append data to local JSON tables.
10. **⊟ Loop Array**: Iterate over data arrays (e.g., from a table).
11. **⎋ Break Loop**: Exit a loop prematurely.

## 🛠 Tech Stack

- **Framework**: Arduino Framework (C++)
- **Storage**: LittleFS (stores flows, configs, and logs)
- **Networking**: Wi-Fi (ST & AP modes), WebServer, WebSockets
- **Data Engine**: cJSON for high-performance JSON manipulation

## ▶ Quick Start

1. Compile and upload the firmware to your ESP32-S3.
2. On first boot, the device creates an access point: `ESP32-WorkFlow-Config` (password: `12345678`).
3. Connect and navigate to `http://192.168.4.1` to configure your Wi-Fi credentials.
4. Once connected, open the device's IP in your browser to access the Flow Editor.

## 🗁 Project Structure

- `esp32n8n.ino` — main loop and system initialization.
- `web_ui.h` — embedded HTML/JS/CSS for the visual editor.
- `engine.h` — core workflow execution logic.
- `web_server.h` — API endpoints for device management.
- `config.h` — global settings and constants.
- `ota_update.h` — over-the-air update logic.

## 🖎 Contributing

Have ideas for new nodes or UI improvements? Pull requests are welcome!
